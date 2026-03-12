#ifndef LOGINSERVER_STATUS_STATUS_HANDLER_H
#define LOGINSERVER_STATUS_STATUS_HANDLER_H

class QueryClient;
struct ServerConfig;

const char* get_status_string(QueryClient& client, const ServerConfig& config);

#endif // LOGINSERVER_STATUS_STATUS_HANDLER_H
