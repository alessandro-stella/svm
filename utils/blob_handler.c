#include "blob_handler.h"
#include "hashing.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

// ===================================================
// Funzione di Salvataggio su Disco (Senza Modifiche)
// ===================================================

bool create_file(const char *hex_hash, const unsigned char *compressed_content, size_t compressed_size, size_t original_uncompressed_size) {
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

  if (fwrite(&original_uncompressed_size, sizeof(size_t), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (fwrite(compressed_content, 1, compressed_size, f) != compressed_size) {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

char *create_object_from_data(const char *type, const unsigned char *content_data, size_t content_size, size_t *out_full_size) {
  char header[64];
  int header_len = snprintf(header, sizeof(header), "%s %zu", type, content_size);

  size_t full_size = header_len + 1 + content_size;
  unsigned char *buffer = malloc(full_size);
  if (!buffer) {
    return NULL;
  }

  memcpy(buffer, header, header_len);
  buffer[header_len] = '\0';
  memcpy(buffer + header_len + 1, content_data, content_size);

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

  if (!create_file(hex_hash, compressed, compressed_len, full_size)) {
    free(compressed);
    free(hex_hash);
    return NULL;
  }

  free(compressed);
  *out_full_size = full_size;
  return hex_hash;
}

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

  size_t full_size;
  char *hex_hash = create_object_from_data("blob", data, file_size, &full_size);

  free(data);

  if (hex_hash) {
    *blob_size_out = file_size;
  }
  return hex_hash;
}

char *create_blob_from_memory(const char *data, size_t data_size) {
  size_t full_size;
  char *hex_hash = create_object_from_data("blob", (const unsigned char *)data, data_size, &full_size);

  return hex_hash;
}
