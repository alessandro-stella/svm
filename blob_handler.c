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

char *create_blob_from_file(const char *filename, size_t *blob_size_out) {
  FILE *f = fopen(filename, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  size_t file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char *data = malloc(file_size);
  if (!data) {
    fclose(f);
    return NULL;
  }
  fread(data, 1, file_size, f);
  fclose(f);

  char header[64];
  int header_len = snprintf(header, sizeof(header), "blob %zu", file_size);

  size_t full_size = header_len + 1 + file_size; // +1 for '\0'
  unsigned char *buffer = malloc(full_size);
  if (!buffer) {
    free(data);
    return NULL;
  }

  memcpy(buffer, header, header_len);
  buffer[header_len] = '\0';
  memcpy(buffer + header_len + 1, data, file_size);

  free(data);

  uLongf compressed_len = compressBound(full_size);
  unsigned char *compressed = malloc(compressed_len);
  if (!compressed) {
    free(buffer);
    return NULL;
  }

  if (compress(compressed, &compressed_len, buffer, full_size) != Z_OK) {
    free(buffer);
    free(compressed);
    return NULL;
  }

  free(buffer);

  size_t hash_len;
  unsigned char *hash_bin = create_hash(compressed, compressed_len, &hash_len);
  char *hex_hash = hash_to_hex(hash_bin, hash_len);
  free(hash_bin);

  if (!create_file(hex_hash, compressed, compressed_len)) {
    free(compressed);
    free(hex_hash);
    return NULL;
  }

  free(compressed);
  *blob_size_out = file_size;
  return hex_hash;
}

bool create_file(const char *hex_hash, const unsigned char *content, size_t content_size) {
  struct stat st = {0};
  if (stat(".svm", &st) == -1 || stat(".svm/objects", &st) == -1)
    return false;

  char subdir[256];
  snprintf(subdir, sizeof(subdir), ".svm/objects/%.2s", hex_hash);
  if (stat(subdir, &st) == -1 && mkdir(subdir, 0700) != 0)
    return false;

  size_t filename_len = strlen(subdir) + strlen(hex_hash + 2) + 2;
  char *filename = malloc(filename_len);
  snprintf(filename, filename_len, "%s/%s", subdir, hex_hash + 2);

  FILE *f = fopen(filename, "wb");
  free(filename);
  if (!f)
    return false;

  size_t compressed_len = compressBound(content_size);
  unsigned char *compressed = malloc(compressed_len);
  if (!compressed) {
    fclose(f);
    return false;
  }

  if (compress(compressed, &compressed_len, content, content_size) != Z_OK) {
    free(compressed);
    fclose(f);
    return false;
  }

  fwrite(&content_size, sizeof(size_t), 1, f);
  fwrite(compressed, 1, compressed_len, f);

  fclose(f);
  free(compressed);
  return true;
}

// ============================
// Reading blob
// ============================

unsigned char *read_blob(const char *hex_hash, size_t *out_original_len) {
  char file_path[256];
  snprintf(file_path, sizeof(file_path), ".svm/objects/%.2s/%s", hex_hash, hex_hash + 2);

  FILE *f = fopen(file_path, "rb");
  if (!f)
    return NULL;

  size_t original_len;
  fread(&original_len, sizeof(size_t), 1, f);
  if (out_original_len)
    *out_original_len = original_len;

  fseek(f, 0, SEEK_END);
  size_t blob_size = ftell(f) - sizeof(size_t);
  fseek(f, sizeof(size_t), SEEK_SET);

  unsigned char *compressed_data = malloc(blob_size);
  fread(compressed_data, 1, blob_size, f);
  fclose(f);

  unsigned char *decompressed = malloc(original_len);
  uLongf dest_len = original_len;
  if (uncompress(decompressed, &dest_len, compressed_data, blob_size) != Z_OK) {
    free(decompressed);
    free(compressed_data);
    return NULL;
  }

  free(compressed_data);
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
