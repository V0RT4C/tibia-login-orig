#ifndef LOGINSERVER_QUERY_TYPES_H
#define LOGINSERVER_QUERY_TYPES_H

#include "common/types.h"

enum class ApplicationType : uint8 {
    Game  = 1,
    Login = 2,
    Web   = 3,
};

enum class QueryStatus : int {
    Ok     = 0,
    Error  = 1,
    Failed = 3,
};

enum class QueryType : uint8 {
    Login        = 0,
    LoginAccount = 11,
    GetWorlds    = 150,
};

struct CharacterLoginData {
    char name[30];
    char world_name[30];
    int world_address;
    int world_port;
};

struct WorldInfo {
    char name[30];
    int type;
    int num_players;
    int max_players;
    int online_peak;
    int online_peak_timestamp;
    int last_startup;
    int last_shutdown;
};

#endif // LOGINSERVER_QUERY_TYPES_H
