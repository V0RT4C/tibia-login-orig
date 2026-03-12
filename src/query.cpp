#include "common.h"

static QueryClient* g_query_client;

int LoginAccount(int account_id, const char* password, const char* ip_address,
        int max_characters, int* num_characters, CharacterLoginData* characters,
        int* premium_days) {
    uint8 buffer[kb(4)];
    BufferWriter write_buffer = g_query_client->prepare_query(
            QueryType::LoginAccount, buffer, sizeof(buffer));
    write_buffer.write_u32((uint32)account_id);
    write_buffer.write_string(password);
    write_buffer.write_string(ip_address);

    BufferReader read_buffer;
    QueryStatus status = g_query_client->execute_query(true, &write_buffer, &read_buffer);
    int result = (status == QueryStatus::Ok ? 0 : -1);
    if (status == QueryStatus::Ok) {
        *num_characters = read_buffer.read_u8();
        if (*num_characters > max_characters) {
            LOG_ERR("Too many characters");
            return -1;
        }

        for (int i = 0; i < *num_characters; i += 1) {
            read_buffer.read_string(characters[i].name, sizeof(characters[i].name));
            read_buffer.read_string(characters[i].world_name, sizeof(characters[i].world_name));
            characters[i].world_address = read_buffer.read_u32_be();
            characters[i].world_port = read_buffer.read_u16();
        }

        *premium_days = read_buffer.read_u16();
    } else if (status == QueryStatus::Error) {
        int error_code = read_buffer.read_u8();
        if (error_code >= 1 && error_code <= 6) {
            result = error_code;
        } else {
            LOG_ERR("Invalid error code %d", error_code);
        }
    } else {
        LOG_ERR("Request failed");
    }
    return result;
}

int GetWorld(const char* world_name, WorldInfo* out_world) {
    ASSERT(world_name && out_world);
    uint8 buffer[4096];
    BufferReader read_buffer;
    BufferWriter write_buffer = g_query_client->prepare_query(
            QueryType::GetWorlds, buffer, sizeof(buffer));
    QueryStatus status = g_query_client->execute_query(true, &write_buffer, &read_buffer);
    int result = (status == QueryStatus::Ok ? 0 : -1);
    memset(out_world, 0, sizeof(WorldInfo));
    if (status == QueryStatus::Ok) {
        int num_worlds = (int)read_buffer.read_u8();
        for (int i = 0; i < num_worlds; i += 1) {
            WorldInfo world = {};
            read_buffer.read_string(world.name, sizeof(world.name));
            world.type = (int)read_buffer.read_u8();
            world.num_players = (int)read_buffer.read_u16();
            world.max_players = (int)read_buffer.read_u16();
            world.online_peak = (int)read_buffer.read_u16();
            world.online_peak_timestamp = (int)read_buffer.read_u32();
            world.last_startup = (int)read_buffer.read_u32();
            world.last_shutdown = (int)read_buffer.read_u32();

            if (string_empty(world_name)) {
                if (i == 0 || world.num_players > out_world->num_players) {
                    *out_world = world;
                }
            } else if (string_equals_ignore_case(world_name, world.name)) {
                *out_world = world;
                break;
            }
        }
    } else {
        LOG_ERR("Request failed");
    }
    return result;
}

bool InitQuery(void) {
    ASSERT(g_query_client == nullptr);
    g_query_client = new QueryClient();
    if (!g_query_client->connect(g_config.query_manager_host,
            g_config.query_manager_port, g_config.query_manager_password)) {
        LOG_ERR("Failed to connect to query manager");
        return false;
    }
    return true;
}

void ExitQuery(void) {
    if (g_query_client != nullptr) {
        delete g_query_client;
        g_query_client = nullptr;
    }
}
