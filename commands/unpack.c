#include "../svm_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

char *unpack_command(const char *hex_hash, size_t *out_original_len) {
  char file_path[256];
  snprintf(file_path, sizeof(file_path), ".svm/objects/%.2s/%s", hex_hash, hex_hash + 2);

  FILE *f = fopen(file_path, "rb");
  if (!f)
    return NULL;

  size_t original_len;

  if (fread(&original_len, sizeof(size_t), 1, f) != 1) {
    fclose(f);
    return NULL;
  }
  if (out_original_len)
    *out_original_len = original_len;

  fseek(f, 0, SEEK_END);
  size_t blob_size = ftell(f) - sizeof(size_t);
  fseek(f, sizeof(size_t), SEEK_SET);

  unsigned char *compressed_data = malloc(blob_size);
  if (!compressed_data) {
    fclose(f);
    return NULL;
  }
  if (fread(compressed_data, 1, blob_size, f) != blob_size) {
    free(compressed_data);
    fclose(f);
    return NULL;
  }
  fclose(f);

  char *decompressed = malloc(original_len + 1);
  if (!decompressed) {
    free(compressed_data);
    return NULL;
  }

  uLongf dest_len = (uLongf)original_len;

  if (uncompress((Bytef *)decompressed, &dest_len, compressed_data, blob_size) != Z_OK) {
    free(decompressed);
    free(compressed_data);
    return NULL;
  }

  free(compressed_data);

  decompressed[dest_len] = '\0';

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
