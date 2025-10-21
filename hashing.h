#ifndef HASHING_H
#define HASHING_H

#include <openssl/evp.h>
#include <stdbool.h>
#include <stddef.h>

unsigned char *create_hash(const unsigned char *data, size_t len, size_t *hash_len);
char *hash_to_hex(const unsigned char *hash, size_t len);

EVP_MD_CTX *hash_init(void);
bool hash_update(EVP_MD_CTX *ctx, const unsigned char *data, size_t len);
unsigned char *hash_finalize(EVP_MD_CTX *ctx, size_t *hash_len);

#endif
