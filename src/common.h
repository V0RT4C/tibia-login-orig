#ifndef TIBIA_COMMON_H_
#define TIBIA_COMMON_H_ 1

#include "common/types.h"
#include "common/assert.h"
#include "common/string_utils.h"
#include "common/logging.h"

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config/config.h"

extern ServerConfig g_config;

#include "common/byte_order.h"
#include "common/utf8.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"

#include "crypto/rsa.h"
#include "crypto/xtea.h"

#include "query/query_types.h"
#include "query/query_client.h"

#include "query/login_query.h"
#include "query/world_query.h"

extern QueryClient* query_client;

#include "status/xml_builder.h"
#include "status/status_handler.h"

#include "login/login_handler.h"

#include "network/connection_types.h"
#include "network/server.h"

#endif //TIBIA_COMMON_H_
