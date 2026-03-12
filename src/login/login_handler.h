#ifndef LOGINSERVER_LOGIN_LOGIN_HANDLER_H
#define LOGINSERVER_LOGIN_LOGIN_HANDLER_H

struct TConnection;
class RsaKey;
class QueryClient;
struct ServerConfig;

void process_login_request(TConnection* connection, RsaKey& rsa_key,
        QueryClient& client, const ServerConfig& config);

#endif // LOGINSERVER_LOGIN_LOGIN_HANDLER_H
