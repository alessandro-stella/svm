#include "../svm_commands.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *local_unpack(char *hash, size_t *content_len) {
  char subdir[256];
  snprintf(subdir, sizeof(subdir), ".svm/objects/%.2s", hash);

  char blob_path[512];
  snprintf(blob_path, sizeof(blob_path), "%s/%s", subdir, hash + 2);

  struct stat st_unpack;
  if (stat(blob_path, &st_unpack) == -1) {
    printf("Blob not found (hash: %s)\n", hash);
    return NULL;
  }

  size_t original_len;
  char *decompressed = unpack_command(hash, &original_len);

  if (!decompressed) {
    printf("Failed to decompress blob.\n");
    return NULL;
  }

  if (strncmp(decompressed, "blob ", 5) != 0) {
    fprintf(stderr, "Malformed decompressed header (not starting with 'blob ').\n");
    free(decompressed);
    return NULL;
  }

  char *header_end = (char *)memchr(decompressed + 5, '\0', original_len - 5);

  if (header_end == NULL) {
    fprintf(stderr, "Malformed decompressed header (missing null byte separator).\n");
    free(decompressed);
    return NULL;
  }

  char *content_start = header_end + 1;
  *content_len = original_len - (content_start - decompressed) + 1;

  char *content_only = (char *)malloc(*content_len);
  if (!content_only) {
    perror("malloc");
    free(decompressed);
    *content_len = 0;
    return NULL;
  }

  memcpy(content_only, content_start, *content_len);
  content_only[*content_len] = '\0';

  free(decompressed);
  return content_only;
}

char *get_tree(char *hash) {
  size_t content_len;
  char *content = local_unpack(hash, &content_len);

  if (content_len < 0) {
    printf("Error during unpack!\n");

    if (content != NULL) {
      free(content);
    }

    return NULL;
  }

  return content;
}

bool restore_file_from_blob(char *hash, char *path) {
  size_t content_len;
  char *content_data = local_unpack(hash, &content_len);

  if (content_data == NULL) {
    fprintf(stderr, "Failed to retrieve content for hash %s.\n", hash);
    return false;
  }

  FILE *fd = fopen(path, "wb");
  if (fd == NULL) {
    perror("Error opening file for writing");
    free(content_data);
    return false;
  }

  size_t written = fwrite(content_data, 1, content_len, fd);

  fclose(fd);
  free(content_data);

  if (written != content_len) {
    fprintf(stderr, "Error writing file %s: expected %zu bytes, wrote %zu bytes.\n", path, content_len, written);
    return false;
  }

  printf("Restored file: %s (size: %zu bytes)\n", path, content_len);

  return true;
}
