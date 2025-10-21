#ifndef BLOB_HANDLER_H
#define BLOB_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

char *create_blob_from_file(const char *filename, size_t *size);
bool create_file(const char *hex_hash, const unsigned char *blob, size_t blob_size);

#endif
