#include "../utils/blob_handler.h"
#include "../utils/hash_table.h"
#include "../utils/hashing.h"

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
  FILE *prep;
  HashTable *written_dirs;
} ProcessData;

static char *create_dynamic_path(const char *parent_dir, const char *name) {
  size_t len = strlen(parent_dir) + 1 + strlen(name) + 1;
  char *new_path = (char *)malloc(len);

  if (new_path == NULL) {
    perror("malloc failed");
    return NULL;
  }

  if (strcmp(parent_dir, ".") == 0) {
    snprintf(new_path, len, "./%s", name);
  } else {
    snprintf(new_path, len, "%s/%s", parent_dir, name);
  }
  return new_path;
}

static void write_file_entry(FILE *prep, const char *path, const char *existing_hash) {
  if (existing_hash != NULL && strcmp(existing_hash, "tree") != 0) {
    fprintf(prep, "%s %s\n", existing_hash, path);
    return;
  }

  struct stat st;
  if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
    return;
  }

  FILE *fd = fopen(path, "rb");
  if (!fd) {
    return;
  }

  long file_size = st.st_size;
  size_t size = (size_t)file_size;

  unsigned char *file_content = NULL;
  size_t read_bytes = 0;

  if (size > 0) {
    file_content = (unsigned char *)malloc(size);
    if (!file_content) {
      fclose(fd);
      return;
    }
    read_bytes = fread(file_content, 1, size, fd);
  } else {
    file_content = (unsigned char *)malloc(1);
    if (!file_content) {
      fclose(fd);
      return;
    }
  }

  fclose(fd);

  if (read_bytes != size && size > 0) {
    free(file_content);
    return;
  }

  size_t header_len_str = (size_t)snprintf(NULL, 0, "blob %lu", (long unsigned int)size);

  size_t total_in_len = header_len_str + 1 + size;

  unsigned char *hash_input_content = (unsigned char *)malloc(total_in_len);
  if (hash_input_content == NULL) {
    free(file_content);
    return;
  }

  snprintf((char *)hash_input_content, total_in_len, "blob %lu", (long unsigned int)size);

  hash_input_content[header_len_str] = '\0';

  if (size > 0) {
    memcpy(hash_input_content + header_len_str + 1, file_content, size);
  }

  free(file_content);

  size_t hash_len;
  unsigned char *hash = create_hash(hash_input_content, total_in_len, &hash_len);
  char *hex_hash = hash_to_hex(hash, hash_len);

  if (!blob_exists(hex_hash)) {
    create_blob(hash_input_content, total_in_len, hex_hash);
  }

  fprintf(prep, "%s %s\n", hex_hash, path);
  printf("Added \"%s\" to prep (compressed) - original size: %lu bytes\n", path, size);

  free(hash_input_content);
  free(hash);
  free(hex_hash);
}

// Recursive function for DFS indexing
static void dfs_traverse_and_process(const char *fn, void *data) {
  ProcessData *pdata = (ProcessData *)data;
  DIR *dir;
  struct dirent *entry;

  // Prepare dir path with '/'
  char *dir_path_for_prep;
  if (strcmp(fn, ".") == 0) {
    dir_path_for_prep = strdup("./");
  } else {
    dir_path_for_prep = strdup(fn);
    if (dir_path_for_prep[strlen(dir_path_for_prep) - 1] != '/') {
      dir_path_for_prep = (char *)realloc(dir_path_for_prep, strlen(dir_path_for_prep) + 2);
      strcat(dir_path_for_prep, "/");
    }
  }

  // Write directory if not already written
  if (!ht_lookup(pdata->written_dirs, dir_path_for_prep)) {
    fprintf(pdata->prep, "tree %s\n", dir_path_for_prep);
    ht_insert(pdata->written_dirs, dir_path_for_prep);
  }

  free(dir_path_for_prep);

  if ((dir = opendir(fn)) == NULL) {
    return;
  }

  char **file_paths = NULL;
  size_t file_count = 0;
  char **dir_paths = NULL;
  size_t dir_count = 0;

  while ((entry = readdir(dir)) != NULL) {
    // TODO: Controllare perchè non legge cartelle interne con .
    if (entry->d_name[0] == '.')
      continue;

    char *current_path = create_dynamic_path(fn, entry->d_name);
    if (current_path == NULL)
      continue;

    struct stat st;
    if (lstat(current_path, &st) != 0) {
      free(current_path);
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      dir_paths = (char **)realloc(dir_paths, (dir_count + 1) * sizeof(char *));
      dir_paths[dir_count++] = current_path;
    } else if (S_ISREG(st.st_mode)) {
      file_paths = (char **)realloc(file_paths, (file_count + 1) * sizeof(char *));
      file_paths[file_count++] = current_path;
    } else {
      free(current_path);
    }
  }
  closedir(dir);

  // Process file in current directory
  for (size_t i = 0; i < file_count; i++) {
    const char *hash_to_write = NULL;

    write_file_entry(pdata->prep, file_paths[i], hash_to_write);
    free(file_paths[i]);
  }
  free(file_paths);

  // Process subdirs
  for (size_t i = 0; i < dir_count; i++) {
    dfs_traverse_and_process(dir_paths[i], data);
    free(dir_paths[i]);
  }
  free(dir_paths);
}

bool prep_command(char *path) {
  if (access(".svm", F_OK) != 0) {
    if (mkdir(".svm", 0755) != 0) {
      perror("Error creating .svm directory");
      return false;
    }
  }

  if (access(".svm/objects", F_OK) != 0) {
    if (mkdir(".svm/objects", 0755) != 0) {
      perror("Error creating .svm/objects directory");
      return false;
    }
  }

  FILE *prep_w = fopen(".svm/prep", "w");
  if (prep_w == NULL) {
    printf("Error: Couldn't open \".svm/prep\" for writing.\n");
    return false;
  }

  ProcessData pdata = {prep_w, ht_create()};
  if (pdata.written_dirs == NULL) {
    fclose(prep_w);
    return false;
  }

  dfs_traverse_and_process(path, &pdata);

  fclose(prep_w);
  ht_free(pdata.written_dirs);

  return true;
}
