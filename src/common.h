#ifndef TIBIA_COMMON_H_
#define TIBIA_COMMON_H_ 1

// Common modules
#include "common/types.h"
#include "common/assert.h"
#include "common/string_utils.h"
#include "common/logging.h"
#include "common/byte_order.h"
#include "common/utf8.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"

// Config
#include "config/config.h"

// Crypto
#include "crypto/rsa.h"
#include "crypto/xtea.h"

// Query
#include "query/query_types.h"
#include "query/query_client.h"
#include "query/login_query.h"
#include "query/world_query.h"

// Status
#include "status/xml_builder.h"
#include "status/status_handler.h"

// Login
#include "login/login_handler.h"

// Network
#include "network/connection_types.h"
#include "network/server.h"
#include "network/websocket_acceptor.h"

// Globals
extern ServerConfig g_config;
extern QueryClient* query_client;

#endif //TIBIA_COMMON_H_
