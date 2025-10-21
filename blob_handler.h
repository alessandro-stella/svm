#ifndef BLOB_HANDLER_H
#define BLOB_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

char *create_blob_from_file(const char *filename, size_t *size);
char *create_blob(const char *data, size_t *size);
unsigned char *create_hash(const unsigned char *blob, size_t len, size_t *hash_len);
char *hash_to_hex(const unsigned char *hash, size_t len);
bool create_file(const char *hex_hash, const unsigned char *blob, size_t blob_size);
unsigned char *read_blob(const char *hex_hash, size_t original_len);
unsigned char *decompress_file(const unsigned char *blob, size_t blob_size, size_t original_len);

#endif
