#include "../svm_commands.h"
#include "../utils/blob_handler.h"

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

char *add_command(const char *c) {
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

        char *tree_hash = add_command(inner_dir);
        free(inner_dir);

        if (tree_hash == NULL) {
          return NULL;
        }

        fprintf(tmp, "tree %s %s\n", tree_hash, dir->d_name);
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

      fprintf(tmp, "blob %s %s\n", hash, dir->d_name);
      free(hash);
    }

    closedir(d);
  }

  fseek(tmp, 0, SEEK_END);
  size_t tree_content_size = ftell(tmp);
  fseek(tmp, 0, SEEK_SET);

  unsigned char *tree_content_data = malloc(tree_content_size);
  if (!tree_content_data) {
    fclose(tmp);
    return NULL;
  }
  fread(tree_content_data, 1, tree_content_size, tmp);
  fclose(tmp);

  size_t full_size_ignored;
  char *hex_hash = create_object_from_data("tree", tree_content_data, tree_content_size, &full_size_ignored);

  free(tree_content_data);
  return hex_hash;
}
