#include "blob_handler.h"
#include "constants.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

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

int create_blob(const unsigned char *data_in, size_t len_in, const char *hex_hash) {
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

    ret = deflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_ERROR) {
      (void)deflateEnd(&strm);
      fclose(file_out);
      return ret;
    }

    size_t have = BUF_SIZE - strm.avail_out;
    if (fwrite(out_buf, 1, have, file_out) != have || ferror(file_out)) {
      (void)deflateEnd(&strm);
      fclose(file_out);
      return Z_ERRNO;
    }
  } while (strm.avail_out == 0);

  if (ret != Z_STREAM_END) {
    (void)deflateEnd(&strm);
    fclose(file_out);
    return Z_STREAM_ERROR;
  }

  (void)deflateEnd(&strm);
  if (fclose(file_out) != 0) {
    return Z_ERRNO;
  }

  return Z_OK;
}

unsigned char *read_compressed_blob(const char *hex_hash, size_t *out_blob_size) {
  size_t path_size = strlen(".svm/objects/") + strlen(hex_hash) + 2;
  char *path = (char *)malloc(path_size);

  if (path == NULL) {
    *out_blob_size = 0;
    return NULL;
  }

  snprintf(path, path_size, ".svm/objects/%.2s/%s", hex_hash, hex_hash + 2);

  FILE *fd = fopen(path, "rb");
  free(path);

  if (fd == NULL) {
    fprintf(stderr, "Errore: Blob non trovato per hash %s\n", hex_hash);
    *out_blob_size = 0;
    return NULL;
  }

  struct stat st;
  if (fstat(fileno(fd), &st) != 0) {
    perror("Errore fstat durante la lettura del blob");
    fclose(fd);
    *out_blob_size = 0;
    return NULL;
  }
  size_t compressed_size = st.st_size;

  unsigned char *compressed_data = (unsigned char *)malloc(compressed_size);
  if (compressed_data == NULL) {
    fclose(fd);
    *out_blob_size = 0;
    return NULL;
  }

  if (fread(compressed_data, 1, compressed_size, fd) != compressed_size) {
    fprintf(stderr, "Errore durante la lettura del blob compresso.\n");
    free(compressed_data);
    fclose(fd);
    *out_blob_size = 0;
    return NULL;
  }
  fclose(fd);

  int ret;
  z_stream strm;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;

  ret = inflateInit2(&strm, -MAX_WBITS);
  if (ret != Z_OK) {
    free(compressed_data);
    *out_blob_size = 0;
    return NULL;
  }

  strm.avail_in = compressed_size;
  strm.next_in = compressed_data;

  size_t decompressed_capacity = INITIAL_DECOMPRESSED_SIZE;
  unsigned char *decompressed_data = (unsigned char *)malloc(decompressed_capacity);

  if (decompressed_data == NULL) {
    (void)inflateEnd(&strm);
    free(compressed_data);
    *out_blob_size = 0;
    return NULL;
  }

  size_t current_decompressed_size = 0;

  do {
    size_t remaining_capacity = decompressed_capacity - current_decompressed_size;

    if (remaining_capacity < BUF_SIZE && strm.avail_in > 0) {
      decompressed_capacity *= 2;
      remaining_capacity = decompressed_capacity - current_decompressed_size; // Aggiorna lo spazio

      unsigned char *temp = (unsigned char *)realloc(decompressed_data, decompressed_capacity);
      if (temp == NULL) {
        (void)inflateEnd(&strm);
        free(compressed_data);
        free(decompressed_data);
        *out_blob_size = 0;
        return NULL;
      }
      decompressed_data = temp;
    }

    strm.avail_out = (uInt)remaining_capacity;
    strm.next_out = decompressed_data + current_decompressed_size;

    ret = inflate(&strm, Z_NO_FLUSH);

    if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
      fprintf(stderr, "Decompression error: %s\n", strm.msg ? strm.msg : "Unknown zlib error");
      (void)inflateEnd(&strm);
      free(compressed_data);
      free(decompressed_data);
      *out_blob_size = 0;
      return NULL;
    }

    size_t data_produced = remaining_capacity - strm.avail_out;
    current_decompressed_size += data_produced;

  } while (ret != Z_STREAM_END);

  (void)inflateEnd(&strm);
  free(compressed_data);

  if (current_decompressed_size > 0 && current_decompressed_size < decompressed_capacity) {
    unsigned char *temp = (unsigned char *)realloc(decompressed_data, current_decompressed_size);
    if (temp != NULL) {
      decompressed_data = temp;
    }
  } else if (current_decompressed_size == 0) {
    free(decompressed_data);
    decompressed_data = NULL;
  }

  *out_blob_size = current_decompressed_size;
  return decompressed_data;
}
