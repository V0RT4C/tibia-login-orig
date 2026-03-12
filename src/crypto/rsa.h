#ifndef LOGINSERVER_CRYPTO_RSA_H
#define LOGINSERVER_CRYPTO_RSA_H

#include "../common/types.h"
#include <memory>

class RsaKey {
public:
    ~RsaKey();

    RsaKey(const RsaKey&) = delete;
    RsaKey& operator=(const RsaKey&) = delete;
    RsaKey(RsaKey&& other) noexcept;
    RsaKey& operator=(RsaKey&& other) noexcept;

    static std::unique_ptr<RsaKey> from_pem_file(const char* filename);
    bool decrypt(uint8* data, int size);

private:
    RsaKey();
    void* key_handle; // opaque RSA* pointer
    static void dump_openssl_errors(const char* where, const char* what);
};

#endif // LOGINSERVER_CRYPTO_RSA_H
