#include "common.h"

#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

static void DumpOpenSSLErrors(const char *Where, const char *What){
	LOG_ERR("OpenSSL error(s) while executing %s at %s:", What, Where);
	ERR_print_errors_cb(
		[](const char *str, size_t len, void *u) -> int {
			(void)u;

			// NOTE(fusion): These error strings already have trailing newlines,
			// for whatever reason.
			if(len > 0 && str[len - 1] == '\n'){
				len -= 1;
			}

			if(len > 0){
				LOG_ERR("> %*s", (int)len, str);
			}

			return 1;
		}, NULL);
}

RSAKey *RSALoadPEM(const char *FileName){
	FILE *File = fopen(FileName, "rb");
	if(File == NULL){
		LOG_ERR("Failed to open \"%s\"", FileName);
		return NULL;
	}

	RSA *Key = PEM_read_RSAPrivateKey(File, NULL, NULL, NULL);
	fclose(File);
	if(Key == NULL){
		LOG_ERR("Failed to read key from \"%s\"", FileName);
		DumpOpenSSLErrors("RSALoadPem", "PEM_read_PrivateKey");
	}

	return (RSAKey*)Key;
}

void RSAFree(RSAKey *Key){
	RSA_free((RSA*)Key);
}

bool RSADecrypt(RSAKey *Key, uint8 *Data, int Size){
	ASSERT(Data != NULL && Size > 0);
	if(Key == NULL){
		LOG_ERR("Key not initialized");
		return false;
	}

	if(Size != RSA_size((RSA*)Key)){
		LOG_ERR("Invalid data size %d (expected %d)", Size, RSA_size((RSA*)Key));
		return false;
	}

	if(RSA_private_decrypt(Size, Data, Data, (RSA*)Key, RSA_NO_PADDING) == -1){
		DumpOpenSSLErrors("RSADecrypt", "RSA_private_decrypt");
		return false;
	}

	return true;
}

