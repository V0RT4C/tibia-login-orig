#ifndef LOGINSERVER_CRYPTO_XTEA_H
#define LOGINSERVER_CRYPTO_XTEA_H

#include "../common/types.h"

void xtea_encrypt(const uint32* key, uint8* data, int size);
void xtea_decrypt(const uint32* key, uint8* data, int size);

#endif // LOGINSERVER_CRYPTO_XTEA_H
