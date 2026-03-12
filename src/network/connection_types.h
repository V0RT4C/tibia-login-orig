#ifndef LOGINSERVER_NETWORK_CONNECTION_TYPES_H
#define LOGINSERVER_NETWORK_CONNECTION_TYPES_H

#include "common/types.h"

enum class ConnectionState : int {
    Free       = 0,
    Reading    = 1,
    Processing = 2,
    Writing    = 3,
};

struct Connection {
    ConnectionState state;
    int socket;
    int ip_address;
    int start_time;
    int rw_size;
    int rw_position;
    uint32 random_seed;
    uint32 xtea[4];
    char remote_address[32];
    uint8 buffer[kb(2)];
};

struct StatusRecord {
    int ip_address;
    int timestamp;
};

#endif // LOGINSERVER_NETWORK_CONNECTION_TYPES_H
