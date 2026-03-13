#include "network/websocket_acceptor.h"
#include "network/connection_types.h"
#include "common.h"
#include "login/login_handler.h"

#include <App.h>
#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/syscall.h>

struct WsPerSocketData {
    uint64_t RequestId;
};

using WsSocket = uWS::WebSocket<false, true, WsPerSocketData>;

// Accessed ONLY from the event loop thread — no synchronization needed.
static std::unordered_map<uint64_t, WsSocket *> PendingRequests;
static uint64_t NextRequestId = 0;

static us_listen_socket_t *ListenSocket = nullptr;
static std::thread WsThread;
static std::atomic<uWS::Loop *> EventLoop{nullptr};
static uWS::App *AppInstance = nullptr;
static std::unique_ptr<RsaKey> WsRsaKey;

// RSA_private_decrypt (legacy OpenSSL API) modifies internal blinding state
// and is NOT thread-safe for concurrent calls on the same RSA* handle.
static std::mutex RsaDecryptMutex;

// Shutdown flag: set before event loop is torn down. Worker threads check
// this before calling loop->defer() to avoid use-after-free.
static std::atomic<bool> ShuttingDown{false};

// Active worker count: incremented before spawning a worker thread,
// decremented when the worker exits. stop_websocket_acceptor waits for
// this to reach zero before destroying WsRsaKey and returning.
static std::atomic<int> ActiveWorkers{0};

// Parse a dotted-quad IPv4 string (e.g. "127.0.0.1") into a host-byte-order
// uint32 matching the loginserver's ip_address format: (a<<24|b<<16|c<<8|d).
// Returns 0 on failure (including IPv6 addresses).
static int parse_ipv4_address(const char *addr_str) {
    struct in_addr addr;
    if (inet_pton(AF_INET, addr_str, &addr) == 1) {
        return (int)ntohl(addr.s_addr);
    }
    return 0;
}

// Helper: defer a lambda to the event loop, but only if not shutting down.
// Returns false if the defer was skipped (shutdown in progress).
template <typename F>
static bool safe_defer(uWS::Loop *loop, F &&fn) {
    if (ShuttingDown.load(std::memory_order_acquire)) return false;
    loop->defer(std::forward<F>(fn));
    return true;
}

static void process_ws_login(uint64_t request_id, uWS::Loop *loop,
        std::string message, std::string remote_ip_str) {
    // Ensure ActiveWorkers is decremented when this function exits,
    // regardless of which path is taken.
    struct WorkerGuard {
        ~WorkerGuard() { ActiveWorkers.fetch_sub(1, std::memory_order_release); }
    } guard;

    // Thread-safe seed for rand_r (no global PRNG state).
    unsigned int seed = (unsigned int)(
            (uint64_t)syscall(SYS_gettid) ^ (uint64_t)time(nullptr));

    Connection conn = {};
    conn.state = ConnectionState::Processing;
    conn.socket = -1;
    conn.ip_address = parse_ipv4_address(remote_ip_str.c_str());
    conn.rw_size = (int)message.size();
    conn.rw_position = 0;
    conn.random_seed = (uint32)rand_r(&seed);
    string_buf_copy(conn.remote_address, remote_ip_str.c_str());
    memcpy(conn.buffer, message.data(), message.size());

    // Each worker thread gets its own QueryClient (thread safety).
    QueryClient client;
    if (!client.connect(g_config.query_manager_host,
            g_config.query_manager_port, g_config.query_manager_password)) {
        LOG_ERR("WebSocket: Failed to connect to query manager for request %lu",
                (unsigned long)request_id);
        safe_defer(loop, [request_id]() {
            auto it = PendingRequests.find(request_id);
            if (it != PendingRequests.end()) {
                it->second->close();
            }
        });
        return;
    }

    // RSA decryption is NOT thread-safe with the legacy OpenSSL API.
    // Lock around process_login_request which calls RsaKey::decrypt internally.
    // The lock is held for the entire request (~1-10ms including query manager I/O),
    // which serializes WS login requests. This is acceptable for a login server.
    {
        std::lock_guard<std::mutex> lock(RsaDecryptMutex);
        process_login_request(&conn, *WsRsaKey, client, g_config);
    }

    // After process_login_request returns:
    // - state == Writing: response is in conn.buffer[0..conn.rw_size]
    // - state != Writing: error path called close_connection (no-op for socket==-1)
    if (conn.state == ConnectionState::Writing && conn.rw_size > 0) {
        std::string response((char *)conn.buffer, conn.rw_size);
        safe_defer(loop, [request_id, response = std::move(response)]() {
            auto it = PendingRequests.find(request_id);
            if (it != PendingRequests.end()) {
                it->second->send(response, uWS::OpCode::BINARY);
                it->second->close();
            }
        });
    } else {
        safe_defer(loop, [request_id]() {
            auto it = PendingRequests.find(request_id);
            if (it != PendingRequests.end()) {
                it->second->close();
            }
        });
    }
}

static void websocket_thread_main() {
    uWS::App app;
    AppInstance = &app;
    EventLoop.store(uWS::Loop::get(), std::memory_order_release);

    app.ws<WsPerSocketData>("/*", {
        .compression = uWS::DISABLED,
        .maxPayloadLength = kb(2),
        .idleTimeout = (unsigned short)g_config.connection_timeout,
        .maxBackpressure = kb(4),

        .open = [](auto *ws) {
            auto *data = ws->getUserData();
            data->RequestId = ++NextRequestId;
            PendingRequests[data->RequestId] = ws;
        },

        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
            if (opCode != uWS::OpCode::BINARY) return;
            if (message.empty() || message.size() > (size_t)kb(2)) {
                ws->close();
                return;
            }

            auto *data = ws->getUserData();
            uint64_t request_id = data->RequestId;
            uWS::Loop *loop = uWS::Loop::get();

            // Extract client IP (uWS returns text like "127.0.0.1" or "::ffff:127.0.0.1")
            std::string remote_ip(ws->getRemoteAddressAsText());

            // Strip IPv4-mapped IPv6 prefix if present
            const char *mapped_prefix = "::ffff:";
            if (remote_ip.compare(0, 7, mapped_prefix) == 0) {
                remote_ip = remote_ip.substr(7);
            }

            uint8_t command = (uint8_t)message[0];
            if (command == 1) {
                // Login request — spawn worker thread for blocking query manager I/O.
                // Increment before spawning so stop_websocket_acceptor can wait
                // for all workers to finish before destroying shared resources.
                ActiveWorkers.fetch_add(1, std::memory_order_release);
                std::string msg_copy(message);
                std::thread(process_ws_login, request_id, loop,
                        std::move(msg_copy), std::move(remote_ip)).detach();
            } else {
                // Status (255) and unknown commands — not supported over WebSocket.
                LOG_WARN("WebSocket: Unsupported command %d from %s",
                        command, remote_ip.c_str());
                ws->close();
            }
        },

        .close = [](auto *ws, int /*code*/, std::string_view /*message*/) {
            auto *data = ws->getUserData();
            PendingRequests.erase(data->RequestId);
        },
    });

    app.listen(g_config.websocket_address, g_config.websocket_port,
            [](auto *listen_socket) {
        if (listen_socket) {
            ListenSocket = listen_socket;
            LOG("WebSocket listening on %s:%d",
                    g_config.websocket_address, g_config.websocket_port);
        } else {
            LOG_ERR("WebSocket: Failed to listen on %s:%d",
                    g_config.websocket_address, g_config.websocket_port);
        }
    });

    app.run();
    AppInstance = nullptr;
    EventLoop.store(nullptr, std::memory_order_release);
}

bool start_websocket_acceptor() {
    WsRsaKey = RsaKey::from_pem_file("tibia.pem");
    if (!WsRsaKey) {
        LOG_ERR("WebSocket: Failed to load RSA key");
        return false;
    }

    ShuttingDown.store(false, std::memory_order_release);
    WsThread = std::thread(websocket_thread_main);
    return true;
}

void stop_websocket_acceptor() {
    // Signal worker threads to stop deferring work to the event loop.
    ShuttingDown.store(true, std::memory_order_release);

    uWS::Loop *loop = EventLoop.load(std::memory_order_acquire);
    if (loop) {
        loop->defer([]() {
            if (ListenSocket) {
                us_listen_socket_close(0, ListenSocket);
                ListenSocket = nullptr;
            }
            if (AppInstance) {
                AppInstance->close();
            }
        });
    }

    if (WsThread.joinable()) {
        WsThread.join();
    }

    // Wait for all in-flight worker threads to finish before destroying
    // WsRsaKey. Workers decrement ActiveWorkers on exit via RAII guard.
    while (ActiveWorkers.load(std::memory_order_acquire) > 0) {
        usleep(10000); // 10ms poll
    }

    WsRsaKey.reset();
}
