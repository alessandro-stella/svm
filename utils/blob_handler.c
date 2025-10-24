#include "blob_handler.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

#define BUF_SIZE 16384

int blob_exists(const char *hex_hash) {
  size_t path_size = strlen(".svm/objects/") + strlen(hex_hash) + 1;
  char *path = malloc(path_size);

  snprintf(path, path_size, ".svm/objects/%.2s/%s", hex_hash, hex_hash + 2);

  struct stat st;

  if (stat(path, &st) == 0) {
    free(path);
    return 1;
  }

  free(path);
  return 0;
}

size_t get_digit_count(size_t number) {
  if (number == 0) {
    return 1;
  }

  size_t count = 0;
  size_t temp = number;

  while (temp > 0) {
    temp /= 10;
    count++;
  }
  return count;
}

int create_directory(const char *path) {
  if (mkdir(path, 0755) != 0) {
    if (errno != EEXIST) {
      perror("Error creating directory");
      return -1;
    }
  }
  return 0;
}

int create_blob(const void *data_in, size_t len_in, const char *hex_hash) {
  size_t dir_size = strlen(".svm/objects/") + 3;
  char *dir_path = malloc(dir_size);

  snprintf(dir_path, dir_size, ".svm/objects/%.2s", hex_hash);

  if (create_directory(dir_path) != 0) {
    free(dir_path);
    return Z_ERRNO;
  }

  size_t path_size = dir_size + strlen(hex_hash) - 1;
  char *path = malloc(path_size);

  snprintf(path, path_size, "%s/%s", dir_path, hex_hash + 2);

  FILE *file_out = fopen(path, "wb");

  free(path);
  free(dir_path);

  if (file_out == NULL) {
    perror("Error while creating blob file");
    return Z_ERRNO;
  }

  int ret;
  z_stream strm;
  unsigned char out_buf[BUF_SIZE];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) {
    fclose(file_out);
    return ret;
  }

  strm.avail_in = len_in;
  strm.next_in = (Bytef *)data_in;

  do {
    strm.avail_out = BUF_SIZE;
    strm.next_out = out_buf;

    int flush = (strm.avail_in == 0) ? Z_FINISH : Z_NO_FLUSH;
    ret = deflate(&strm, flush);

    if (ret == Z_STREAM_ERROR) {
      (void)deflateEnd(&strm);
      fclose(file_out);
      return ret;
    }

    unsigned int have = BUF_SIZE - strm.avail_out;

    if (fwrite(out_buf, 1, have, file_out) != have || ferror(file_out)) {
      (void)deflateEnd(&strm);
      fclose(file_out);
      return Z_ERRNO;
    }

  } while (ret != Z_STREAM_END);

  (void)deflateEnd(&strm);
  fclose(file_out);

  return Z_OK;
}
