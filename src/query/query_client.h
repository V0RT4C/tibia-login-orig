#ifndef LOGINSERVER_QUERY_CLIENT_H
#define LOGINSERVER_QUERY_CLIENT_H

#include "common/types.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"
#include "query/query_types.h"

class QueryClient {
public:
    QueryClient();
    ~QueryClient();

    // Non-copyable, non-movable
    QueryClient(const QueryClient&) = delete;
    QueryClient& operator=(const QueryClient&) = delete;

    bool connect(const char* host, int port, const char* password);
    void disconnect();
    bool is_connected() const;

    BufferWriter prepare_query(QueryType type, uint8* buffer, int buffer_size);
    QueryStatus execute_query(bool auto_reconnect,
            BufferWriter* write_buffer, BufferReader* out_read_buffer);

private:
    int socket_;
    // Store connection params for auto-reconnect
    char host_[256];
    int port_;
    char password_[256];
};

#endif // LOGINSERVER_QUERY_CLIENT_H
