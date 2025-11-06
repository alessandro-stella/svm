#ifndef BLOB_HANDLER_H
#define BLOB_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

int blob_exists(const char *hex_hash);
int create_blob(const unsigned char *data_in, size_t len_in, const char *hex_hash);
unsigned char *read_compressed_blob(const char *hex_hash, size_t *out_blob_size);

#endif
