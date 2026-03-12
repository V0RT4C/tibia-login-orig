#ifndef LOGINSERVER_QUERY_WORLD_QUERY_H
#define LOGINSERVER_QUERY_WORLD_QUERY_H

#include "query/query_client.h"
#include "query/query_types.h"

int get_world(QueryClient& client, const char* world_name, WorldInfo* out_world);

#endif // LOGINSERVER_QUERY_WORLD_QUERY_H
