#ifndef BLOB_HANDLER_H
#define BLOB_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

int blob_exists(const char *hex_hash);

size_t get_digit_count(size_t number);

int create_blob(const void *data_in, size_t len_in, const char *hex_hash);

bool create_file(const char *hex_hash, const unsigned char *compressed_content, size_t compressed_size, size_t original_uncompressed_size);

#endif
