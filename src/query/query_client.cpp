#include "query/query_client.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/logging.h"
#include "common/string_utils.h"
#include "common/byte_order.h"

static bool ResolveHostName(const char* host_name, in_addr_t* out_addr) {
    ASSERT(host_name != NULL && out_addr != NULL);
    addrinfo* result = NULL;
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err_code = getaddrinfo(host_name, NULL, &hints, &result);
    if (err_code != 0) {
        LOG_ERR("Failed to resolve hostname \"%s\": %s", host_name, gai_strerror(err_code));
        return false;
    }

    bool resolved = false;
    for (addrinfo* addr_info = result;
            addr_info != NULL;
            addr_info = addr_info->ai_next) {
        if (addr_info->ai_family == AF_INET && addr_info->ai_socktype == SOCK_STREAM) {
            ASSERT(addr_info->ai_addrlen == sizeof(sockaddr_in));
            *out_addr = ((sockaddr_in*)addr_info->ai_addr)->sin_addr.s_addr;
            resolved = true;
            break;
        }
    }
    freeaddrinfo(result);
    return resolved;
}

static bool WriteExact(int fd, const uint8* buffer, int size) {
    int bytes_to_write = size;
    const uint8* write_ptr = buffer;
    while (bytes_to_write > 0) {
        int ret = (int)write(fd, write_ptr, bytes_to_write);
        if (ret == -1) {
            return false;
        }
        bytes_to_write -= ret;
        write_ptr += ret;
    }
    return true;
}

static bool ReadExact(int fd, uint8* buffer, int size) {
    int bytes_to_read = size;
    uint8* read_ptr = buffer;
    while (bytes_to_read > 0) {
        int ret = (int)read(fd, read_ptr, bytes_to_read);
        if (ret == -1 || ret == 0) {
            return false;
        }
        bytes_to_read -= ret;
        read_ptr += ret;
    }
    return true;
}

QueryClient::QueryClient() : socket_(-1), port_(0) {
    host_[0] = '\0';
    password_[0] = '\0';
}

QueryClient::~QueryClient() {
    disconnect();
}

bool QueryClient::connect(const char* host, int port, const char* password) {
    if (socket_ != -1) {
        LOG_ERR("Already connected");
        return false;
    }

    // Store connection params for auto-reconnect
    strncpy(host_, host, sizeof(host_) - 1);
    host_[sizeof(host_) - 1] = '\0';
    port_ = port;
    strncpy(password_, password, sizeof(password_) - 1);
    password_[sizeof(password_) - 1] = '\0';

    in_addr_t addr;
    if (!ResolveHostName(host_, &addr)) {
        LOG_ERR("Failed to resolve query manager's host name \"%s\"", host_);
        return false;
    }

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == -1) {
        LOG_ERR("Failed to create socket: (%d) %s", errno, strerror(errno));
        return false;
    }

    sockaddr_in query_manager_address = {};
    query_manager_address.sin_family = AF_INET;
    query_manager_address.sin_port = htons((uint16)port_);
    query_manager_address.sin_addr.s_addr = addr;
    if (::connect(socket_, (sockaddr*)&query_manager_address, sizeof(query_manager_address)) == -1) {
        LOG_ERR("Failed to connect: (%d) %s", errno, strerror(errno));
        disconnect();
        return false;
    }

    uint8 login_buffer[1024];
    BufferWriter write_buffer = prepare_query(QueryType::Login, login_buffer, sizeof(login_buffer));
    write_buffer.write_u8(static_cast<uint8>(ApplicationType::Login));
    write_buffer.write_string(password_);
    QueryStatus status = execute_query(false, &write_buffer, NULL);
    if (status != QueryStatus::Ok) {
        LOG_ERR("Failed to login to query manager (%d)", (int)status);
        disconnect();
        return false;
    }

    return true;
}

void QueryClient::disconnect() {
    if (socket_ != -1) {
        close(socket_);
        socket_ = -1;
    }
}

bool QueryClient::is_connected() const {
    return socket_ != -1;
}

BufferWriter QueryClient::prepare_query(QueryType type, uint8* buffer, int buffer_size) {
    BufferWriter write_buffer(buffer, buffer_size);
    write_buffer.write_u16(0); // Request Size
    write_buffer.write_u8(static_cast<uint8>(type));
    return write_buffer;
}

QueryStatus QueryClient::execute_query(bool auto_reconnect,
        BufferWriter* write_buffer, BufferReader* out_read_buffer) {
    // IMPORTANT(fusion): This is similar to the Go version where there is no
    // connection buffer, and the response is read into the same buffer used
    // by `write_buffer`. This helps prevent allocating and moving data around
    // when reconnecting in the middle of a query.
    ASSERT(write_buffer != NULL && write_buffer->position > 2);

    int request_size = write_buffer->position - 2;
    if (request_size < 0xFFFF) {
        write_buffer->rewrite_u16(0, (uint16)request_size);
    } else {
        write_buffer->rewrite_u16(0, 0xFFFF);
        write_buffer->insert_u32(2, (uint32)request_size);
    }

    if (write_buffer->overflowed()) {
        LOG_ERR("Write buffer overflowed when writing request");
        return QueryStatus::Failed;
    }

    const int max_attempts = 2;
    uint8* buffer = write_buffer->buffer;
    int buffer_size = write_buffer->size;
    int write_size = write_buffer->position;
    for (int attempt = 1; true; attempt += 1) {
        if (!is_connected() && (!auto_reconnect || !this->connect(host_, port_, password_))) {
            return QueryStatus::Failed;
        }

        if (!WriteExact(socket_, buffer, write_size)) {
            disconnect();
            if (attempt >= max_attempts) {
                LOG_ERR("Failed to write request");
                return QueryStatus::Failed;
            }
            continue;
        }

        uint8 help[4];
        if (!ReadExact(socket_, help, 2)) {
            disconnect();
            if (attempt >= max_attempts) {
                LOG_ERR("Failed to read response size");
                return QueryStatus::Failed;
            }
            continue;
        }

        int response_size = read_u16_le(help);
        if (response_size == 0xFFFF) {
            if (!ReadExact(socket_, help, 4)) {
                disconnect();
                LOG_ERR("Failed to read response extended size");
                return QueryStatus::Failed;
            }
            response_size = read_u32_le(help);
        }

        if (response_size <= 0 || response_size > buffer_size) {
            disconnect();
            LOG_ERR("Invalid response size %d (buffer_size: %d)",
                    response_size, buffer_size);
            return QueryStatus::Failed;
        }

        if (!ReadExact(socket_, buffer, response_size)) {
            disconnect();
            LOG_ERR("Failed to read response");
            return QueryStatus::Failed;
        }

        BufferReader read_buffer(buffer, response_size);
        int status = read_buffer.read_u8();
        if (out_read_buffer) {
            *out_read_buffer = read_buffer;
        }
        return static_cast<QueryStatus>(status);
    }
}
