#include "query/world_query.h"
#include "common/logging.h"
#include "common/assert.h"
#include "common/string_utils.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"
#include "common/types.h"
#include <string.h>

int get_world(QueryClient& client, const char* world_name, WorldInfo* out_world) {
    ASSERT(world_name && out_world);
    uint8 buffer[4096];
    BufferReader read_buffer;
    BufferWriter write_buffer = client.prepare_query(
            QueryType::GetWorlds, buffer, sizeof(buffer));
    QueryStatus status = client.execute_query(true, &write_buffer, &read_buffer);
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
