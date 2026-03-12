#include "rsa.h"
#include "../common/logging.h"
#include "../common/assert.h"

// Suppress deprecation warnings for OpenSSL legacy RSA API
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

RsaKey::RsaKey() : key_handle(nullptr) {}

RsaKey::~RsaKey() {
    if (key_handle) {
        RSA_free(static_cast<RSA*>(key_handle));
        key_handle = nullptr;
    }
}

RsaKey::RsaKey(RsaKey&& other) noexcept : key_handle(other.key_handle) {
    other.key_handle = nullptr;
}

RsaKey& RsaKey::operator=(RsaKey&& other) noexcept {
    if (this != &other) {
        if (key_handle) {
            RSA_free(static_cast<RSA*>(key_handle));
        }
        key_handle = other.key_handle;
        other.key_handle = nullptr;
    }
    return *this;
}

void RsaKey::dump_openssl_errors(const char* where, const char* what) {
    LOG_ERR("OpenSSL error(s) while executing %s at %s:", what, where);
    ERR_print_errors_cb(
        [](const char* str, size_t len, void*) -> int {
            if (len > 0 && str[len - 1] == '\n') {
                len -= 1;
            }
            if (len > 0) {
                LOG_ERR("> %.*s", static_cast<int>(len), str);
            }
            return 1;
        }, nullptr);
}

std::unique_ptr<RsaKey> RsaKey::from_pem_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == nullptr) {
        LOG_ERR("Failed to open \"%s\"", filename);
        return nullptr;
    }

    RSA* rsa = PEM_read_RSAPrivateKey(file, nullptr, nullptr, nullptr);
    fclose(file);
    if (rsa == nullptr) {
        LOG_ERR("Failed to read key from \"%s\"", filename);
        dump_openssl_errors("from_pem_file", "PEM_read_RSAPrivateKey");
        return nullptr;
    }

    auto key = std::unique_ptr<RsaKey>(new RsaKey());
    key->key_handle = rsa;
    return key;
}

bool RsaKey::decrypt(uint8* data, int size) {
    ASSERT(data != nullptr && size > 0);
    auto* rsa = static_cast<RSA*>(key_handle);
    if (rsa == nullptr) {
        LOG_ERR("Key not initialized");
        return false;
    }

    if (size != RSA_size(rsa)) {
        LOG_ERR("Invalid data size %d (expected %d)", size, RSA_size(rsa));
        return false;
    }

    if (RSA_private_decrypt(size, data, data, rsa, RSA_NO_PADDING) == -1) {
        dump_openssl_errors("decrypt", "RSA_private_decrypt");
        return false;
    }

    return true;
}

#pragma GCC diagnostic pop
