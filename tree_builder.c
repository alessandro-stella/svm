#include "tree_builder.h"
#include "blob_handler.h"
#include "hashing.h"

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *join_path(const char *base, const char *subdir) {
  if (!base)
    base = "";
  if (!subdir)
    subdir = "";

  int need_slash = (base[strlen(base) - 1] != '/');

  size_t len = strlen(base) + strlen(subdir) + (need_slash ? 1 : 0) + 1;

  char *path = malloc(len);
  if (!path) {
    perror("malloc");
    return NULL;
  }

  strcpy(path, base);
  if (need_slash)
    strcat(path, "/");
  strcat(path, subdir);

  return path;
}

char *create_tree(const char *c, size_t *tree_size) {
  FILE *tmp = tmpfile();

  if (!tmp) {
    printf("Error during tmp creation for dir %s", c);
    return NULL;
  }

  DIR *d;
  struct dirent *dir;

  d = opendir(c);

  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (dir->d_type == DT_DIR) {
        if (dir->d_name[0] == '.') {
          continue;
        }

        char *inner_dir = join_path(c, dir->d_name);
        if (!inner_dir) {
          fclose(tmp);
          return NULL;
        }

        size_t tree_original_size;
        char *tree_hash = create_tree(inner_dir, &tree_original_size);
        free(inner_dir);

        if (tree_hash == NULL) {
          free(tree_hash);
          return NULL;
        }

        fprintf(tmp, "tree %s %lu %s\n", tree_hash, tree_original_size, dir->d_name);
        free(tree_hash);
        continue;
      }

      size_t file_size;
      char *file_path = join_path(c, dir->d_name);
      char *hash = create_blob_from_file(file_path, &file_size);
      free(file_path);

      if (hash == NULL) {
        printf("Error during blob creation of %s\n", dir->d_name);
        continue;
      }

      printf("\nBlob for %s (original size: %lu) created: %s", dir->d_name, file_size, hash);
      printf("\n");

      fprintf(tmp, "blob %s %lu %s\n", hash, file_size, dir->d_name);
      free(hash);
    }

    closedir(d);
  }

  fseek(tmp, 0, SEEK_SET);

  fseek(tmp, 0, SEEK_END);
  *tree_size = ftell(tmp);
  fseek(tmp, 0, SEEK_SET);

  unsigned char *data = malloc(*tree_size);
  fread(data, 1, *tree_size, tmp);

  size_t hash_len;
  unsigned char *hash = create_hash(data, *tree_size, &hash_len);
  char *hex_hash = hash_to_hex(hash, hash_len);

  create_file(hex_hash, data, *tree_size);

  fclose(tmp);
  return hex_hash;
}
