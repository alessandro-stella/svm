#include "../svm_commands.h"
#include "../utils/blob_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

char *unpack_command(const char *hex_hash, size_t *out_original_len) {
  size_t decompressed_size;
  unsigned char *decompressed_blob = read_compressed_blob(hex_hash, &decompressed_size);

  if (decompressed_blob == NULL) {
    fprintf(stderr, "Failed to decompress blob.\n");
    *out_original_len = 0;
    return NULL;
  }

  const unsigned char *size_start = (unsigned char *)strchr((char *)decompressed_blob, ' ');
  if (size_start == NULL) {
    fprintf(stderr, "Error: Malformed decompressed blob header (missing space).\n");
    free(decompressed_blob);
    return NULL;
  }
  size_start++;

  const unsigned char *null_terminator = (unsigned char *)memchr(size_start, '\0', decompressed_size - (size_start - decompressed_blob));
  if (null_terminator == NULL) {
    fprintf(stderr, "Error: Malformed decompressed blob header (missing null terminator).\n");
    free(decompressed_blob);
    return NULL;
  }

  size_t size_len = null_terminator - size_start;

  char size_str[32];
  if (size_len <= 0 || size_len >= sizeof(size_str)) {
    fprintf(stderr, "Error: Invalid size string length.\n");
    free(decompressed_blob);
    return NULL;
  }

  memcpy(size_str, (const char *)size_start, size_len);
  size_str[size_len] = '\0';

  long original_len_long = strtol(size_str, NULL, 10);
  if (original_len_long < 0) {
    fprintf(stderr, "Error: Could not parse original blob size.\n");
    free(decompressed_blob);
    return NULL;
  }
  size_t original_len = (size_t)original_len_long;

  const unsigned char *content_start = null_terminator + 1;
  size_t actual_content_len = decompressed_size - (content_start - decompressed_blob);

  if (actual_content_len != original_len) {
    fprintf(stderr, "Error: Decompressed size mismatch! Header size: %lu, Actual content size: %lu\n", original_len, actual_content_len);
    free(decompressed_blob);
    return NULL;
  }

  *out_original_len = decompressed_size;
  return (char *)decompressed_blob;
}
