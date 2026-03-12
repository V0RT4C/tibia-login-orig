#ifndef LOGINSERVER_NETWORK_SERVER_H
#define LOGINSERVER_NETWORK_SERVER_H

struct Connection;
struct ServerConfig;

void close_connection(Connection* connection);
void process_connections();
bool init_connections();
void exit_connections();

#endif // LOGINSERVER_NETWORK_SERVER_H
