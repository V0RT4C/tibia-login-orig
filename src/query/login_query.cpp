#include "query/login_query.h"
#include "common/logging.h"
#include "common/assert.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"
#include "common/types.h"

int login_account(QueryClient& client, int account_id, const char* password,
        const char* ip_address, int max_characters, int* num_characters,
        CharacterLoginData* characters, int* premium_days) {
    uint8 buffer[kb(4)];
    BufferWriter write_buffer = client.prepare_query(
            QueryType::LoginAccount, buffer, sizeof(buffer));
    write_buffer.write_u32((uint32)account_id);
    write_buffer.write_string(password);
    write_buffer.write_string(ip_address);

    BufferReader read_buffer;
    QueryStatus status = client.execute_query(true, &write_buffer, &read_buffer);
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

int resolve_email(QueryClient& client, const char* email, int* account_id) {
    uint8 buffer[kb(1)];
    BufferWriter write_buffer = client.prepare_query(
            QueryType::ResolveEmail, buffer, sizeof(buffer));
    write_buffer.write_string(email);

    BufferReader read_buffer;
    QueryStatus status = client.execute_query(true, &write_buffer, &read_buffer);
    if (status == QueryStatus::Ok) {
        *account_id = (int)read_buffer.read_u32();
        return 0;
    } else if (status == QueryStatus::Error) {
        return read_buffer.read_u8(); // 1 = email not found
    }
    return -1;
}
