#include "status/status_handler.h"
#include "status/xml_builder.h"
#include "query/query_client.h"
#include "query/world_query.h"
#include "query/query_types.h"
#include "config/config.h"
#include "common/logging.h"
#include "common/types.h"
#include <time.h>

static int g_last_status_refresh;
static char g_status_string[kb(2)];

const char* get_status_string(QueryClient& client, const ServerConfig& config) {
    int time_now = (int)time(NULL);
    if((time_now - g_last_status_refresh) >= config.min_status_interval) {
        const char* world_name = "";
        int uptime = 0;
        int num_players = 0;
        int max_players = 0;
        int online_peak = 0;

        WorldInfo world = {};
        if(get_world(client, config.status_world, &world) == 0) {
            world_name = world.name;
            if(world.last_startup != 0 && world.last_startup > world.last_shutdown) {
                uptime = time_now - world.last_startup;
            }
            num_players = world.num_players;
            max_players = world.max_players;
            online_peak = world.online_peak;

            // IMPORTANT(fusion): This could be a common behaviour but, on OTSERVLIST,
            // the server will show as OFFLINE if the the online peak is less than
            // the number of online players. This shouldn't usually be a problem since
            // the online character list and online peak are updated together in the
            // same CREATE_PLAYERLIST query, but is something to keep in mind.
            if(online_peak < num_players) {
                online_peak = num_players;
            }
        } else {
            LOG_ERR("Failed to query world data...");
        }

        // NOTE(fusion): Skip line with MOTD hash.
        const char* motd = config.motd;
        while(motd[0]) {
            if(motd[0] == '\n') {
                motd += 1;
                break;
            }
            motd += 1;
        }

        XmlBuffer buffer = {};
        buffer.data = g_status_string;
        buffer.size = sizeof(g_status_string);
        xml_append_string(&buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        xml_append_string(&buffer, "<tsqp version=\"1.0\">");
        xml_append_format(&buffer,
                "<serverinfo servername=\"%s\" uptime=\"%d\" url=\"%s\""
                    " location=\"%s\" server=\"%s\" version=\"%s\""
                    " client=\"%s\"/>",
                world_name, uptime, config.url, config.location,
                config.server_type, config.server_version,
                config.client_version);
        xml_append_format(&buffer,
                "<players online=\"%d\" max=\"%d\" peak=\"%d\"/>",
                num_players, max_players, online_peak);
        xml_append_format(&buffer, "<motd>%s</motd>", motd);
        xml_append_string(&buffer, "</tsqp>");
        xml_null_terminate(&buffer);

        g_last_status_refresh = time_now;
    }

    return g_status_string;
}
