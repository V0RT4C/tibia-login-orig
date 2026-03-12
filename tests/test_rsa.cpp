#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "../src/crypto/rsa.h"

#include <cstdio>
#include <cstring>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

static const char* TEST_PEM_PATH = "/tmp/test_rsa_key.pem";

// Generate a 1024-bit RSA key, write private key to PEM file.
// Returns the RSA* for use in public-key encryption (caller must RSA_free).
static RSA* generate_test_key() {
    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();
    BN_set_word(e, RSA_F4);
    RSA_generate_key_ex(rsa, 1024, e, nullptr);
    BN_free(e);

    FILE* f = fopen(TEST_PEM_PATH, "wb");
    PEM_write_RSAPrivateKey(f, rsa, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(f);

    return rsa;
}

TEST_CASE("from_pem_file with non-existent file returns nullptr") {
    auto key = RsaKey::from_pem_file("/tmp/nonexistent_rsa_key_99999.pem");
    CHECK(key == nullptr);
}

TEST_CASE("from_pem_file loads a valid PEM key") {
    RSA* rsa = generate_test_key();
    RSA_free(rsa);

    auto key = RsaKey::from_pem_file(TEST_PEM_PATH);
    CHECK(key != nullptr);

    std::remove(TEST_PEM_PATH);
}

TEST_CASE("encrypt-decrypt roundtrip") {
    RSA* rsa = generate_test_key();

    // Build plaintext: first byte must be 0 (protocol requirement)
    uint8 plaintext[128] = {};
    for (int i = 1; i < 128; i++) {
        plaintext[i] = static_cast<uint8>(i);
    }

    uint8 ciphertext[128];
    int ret = RSA_public_encrypt(128, plaintext, ciphertext, rsa, RSA_NO_PADDING);
    REQUIRE(ret == 128);
    RSA_free(rsa);

    // Decrypt with RsaKey
    auto key = RsaKey::from_pem_file(TEST_PEM_PATH);
    REQUIRE(key != nullptr);
    CHECK(key->decrypt(ciphertext, 128));
    CHECK(std::memcmp(ciphertext, plaintext, 128) == 0);

    std::remove(TEST_PEM_PATH);
}

TEST_CASE("decrypt with wrong data size fails") {
    RSA* rsa = generate_test_key();
    RSA_free(rsa);

    auto key = RsaKey::from_pem_file(TEST_PEM_PATH);
    REQUIRE(key != nullptr);

    uint8 data[64] = {};
    CHECK_FALSE(key->decrypt(data, 64));

    std::remove(TEST_PEM_PATH);
}

TEST_CASE("move constructor transfers ownership") {
    RSA* rsa = generate_test_key();

    uint8 plaintext[128] = {};
    plaintext[1] = 42;
    uint8 ciphertext[128];
    RSA_public_encrypt(128, plaintext, ciphertext, rsa, RSA_NO_PADDING);
    RSA_free(rsa);

    auto key1 = RsaKey::from_pem_file(TEST_PEM_PATH);
    REQUIRE(key1 != nullptr);

    // Move into key2
    RsaKey key2 = std::move(*key1);

    CHECK(key2.decrypt(ciphertext, 128));
    CHECK(ciphertext[0] == 0);
    CHECK(ciphertext[1] == 42);

    std::remove(TEST_PEM_PATH);
}

TEST_CASE("move assignment transfers ownership") {
    RSA* rsa = generate_test_key();

    uint8 plaintext[128] = {};
    plaintext[1] = 99;
    uint8 ciphertext[128];
    RSA_public_encrypt(128, plaintext, ciphertext, rsa, RSA_NO_PADDING);
    RSA_free(rsa);

    auto key1 = RsaKey::from_pem_file(TEST_PEM_PATH);
    REQUIRE(key1 != nullptr);

    // Move-assign into key2
    auto key2 = RsaKey::from_pem_file(TEST_PEM_PATH);
    REQUIRE(key2 != nullptr);
    *key2 = std::move(*key1);

    CHECK(key2->decrypt(ciphertext, 128));
    CHECK(ciphertext[0] == 0);
    CHECK(ciphertext[1] == 99);

    std::remove(TEST_PEM_PATH);
}

#pragma GCC diagnostic pop
