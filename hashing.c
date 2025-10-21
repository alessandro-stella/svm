#include "hashing.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char *create_hash(const unsigned char *data, size_t len, size_t *hash_len) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return NULL;

  const EVP_MD *md = EVP_sha256();
  if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  if (EVP_DigestUpdate(ctx, data, len) != 1) {
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  unsigned char *hash = malloc(EVP_MD_size(md));
  if (!hash) {
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  unsigned int out_len;
  if (EVP_DigestFinal_ex(ctx, hash, &out_len) != 1) {
    free(hash);
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  EVP_MD_CTX_free(ctx);
  *hash_len = out_len;
  return hash;
}

char *hash_to_hex(const unsigned char *hash, size_t len) {
  char *hex = malloc(len * 2 + 1);
  if (!hex)
    return NULL;

  for (size_t i = 0; i < len; i++) {
    sprintf(hex + i * 2, "%02x", hash[i]);
  }
  hex[len * 2] = '\0';
  return hex;
}

// ============================
// Incremental hash
// ============================
EVP_MD_CTX *hash_init(void) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return NULL;
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
    EVP_MD_CTX_free(ctx);
    return NULL;
  }
  return ctx;
}

bool hash_update(EVP_MD_CTX *ctx, const unsigned char *data, size_t len) {
  if (!ctx || !data)
    return false;
  return EVP_DigestUpdate(ctx, data, len) == 1;
}

unsigned char *hash_finalize(EVP_MD_CTX *ctx, size_t *hash_len) {
  if (!ctx || !hash_len)
    return NULL;

  const EVP_MD *md = EVP_sha256();
  unsigned char *hash = malloc(EVP_MD_size(md));
  if (!hash)
    return NULL;

  unsigned int out_len;
  if (EVP_DigestFinal_ex(ctx, hash, &out_len) != 1) {
    free(hash);
    return NULL;
  }

  EVP_MD_CTX_free(ctx);
  *hash_len = out_len;
  return hash;
}
