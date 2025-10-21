#include "blob_handler.h"
#include "hashing.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

// ============================
// Creating blob
// ============================

char *create_blob_from_file(const char *filename, size_t *size) {
  FILE *f = fopen(filename, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char *blob = malloc(*size);
  if (!blob) {
    fclose(f);
    return NULL;
  }

  size_t read_bytes = fread(blob, 1, *size, f);
  fclose(f);

  if (read_bytes != *size) {
    free(blob);
    return NULL;
  }

  size_t header_len = snprintf(NULL, 0, "%zu", read_bytes) + 1;
  unsigned char *blob_data_for_hash = malloc(header_len + read_bytes);
  if (!blob_data_for_hash) {
    free(blob);
    return NULL;
  }

  sprintf((char *)blob_data_for_hash, "%zu", read_bytes);
  memcpy(blob_data_for_hash + header_len, blob, read_bytes);

  size_t hash_len;
  unsigned char *hash = create_hash(blob_data_for_hash, header_len + read_bytes, &hash_len);
  char *hex_hash = hash_to_hex(hash, hash_len);

  free(hash);
  free(blob_data_for_hash);

  if (!create_file(hex_hash, blob, *size)) {
    free(blob);
    free(hex_hash);
    return NULL;
  }

  free(blob);

  return hex_hash;
}

bool create_file(const char *hex_hash, const unsigned char *content, size_t content_size) {
  struct stat st = {0};

  if (stat(".svm", &st) == -1) {
    printf("svm not initialized\n");
    return false;
  }
  if (stat(".svm/objects", &st) == -1) {
    printf("objects directory missing\n");
    return false;
  }

  char subdir[256];
  snprintf(subdir, sizeof(subdir), ".svm/objects/%.2s", hex_hash);
  if (stat(subdir, &st) == -1) {
    if (mkdir(subdir, 0700) != 0) {
      perror("mkdir");
      return false;
    }
  }

  size_t filename_len = strlen(subdir) + strlen(hex_hash + 2) + 2;
  char *filename = malloc(filename_len);
  if (!filename)
    return false;

  snprintf(filename, filename_len, "%s/%s", subdir, hex_hash + 2);

  FILE *f = fopen(filename, "wb");
  free(filename);
  if (!f) {
    perror("fopen");
    return false;
  }

  size_t compressed_len = compressBound(content_size);
  unsigned char *compressed = malloc(compressed_len);
  if (!compressed) {
    fclose(f);
    return false;
  }

  if (compress(compressed, &compressed_len, content, content_size) != Z_OK) {
    printf("Compression failed\n");
    free(compressed);
    fclose(f);
    return false;
  }

  fwrite(compressed, 1, compressed_len, f);
  fclose(f);
  free(compressed);

  return true;
}

// ============================
// Reading blob
// ============================

unsigned char *read_blob(const char *hex_hash, size_t original_len) {
  char file_path[256];
  snprintf(file_path, sizeof(file_path), ".svm/objects/%.2s/%s", hex_hash, hex_hash + 2);

  FILE *f = fopen(file_path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  size_t blob_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char *blob_content = malloc(blob_size);
  if (!blob_content) {
    fclose(f);
    return NULL;
  }

  size_t read_bytes = fread(blob_content, 1, blob_size, f);
  fclose(f);

  if (read_bytes != blob_size) {
    free(blob_content);
    return NULL;
  }

  unsigned char *decompressed = decompress_file(blob_content, blob_size, original_len);

  if (decompressed == NULL) {
    free(blob_content);
    return NULL;
  }

  free(blob_content);
  return decompressed;
}

unsigned char *decompress_file(const unsigned char *blob, size_t blob_size, size_t original_len) {
  unsigned char *decompressed = malloc(original_len);
  if (!decompressed)
    return NULL;

  uLongf dest_len = (uLongf)original_len;

  if (uncompress(decompressed, &dest_len, blob, blob_size) != Z_OK) {
    printf("Decompression failed\n");
    free(decompressed);
    return NULL;
  }

  return decompressed;
}
