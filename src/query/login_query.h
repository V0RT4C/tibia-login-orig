#ifndef LOGINSERVER_QUERY_LOGIN_QUERY_H
#define LOGINSERVER_QUERY_LOGIN_QUERY_H

#include "query/query_client.h"
#include "query/query_types.h"

int login_account(QueryClient& client, int account_id, const char* password,
        const char* ip_address, int max_characters, int* num_characters,
        CharacterLoginData* characters, int* premium_days);

#endif // LOGINSERVER_QUERY_LOGIN_QUERY_H
