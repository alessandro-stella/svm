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
  char *hash;
  char *path;
} Entry;

typedef struct {
  FILE *prep;
  HashTable *written_dirs;
  Entry *entries;
  size_t entry_count;
  const char *updated_path;
  const char *updated_hash;
} ProcessData;

static char *read_until_char(FILE *fp, int delimiter) {
  size_t buffer_size = 16;
  size_t length = 0;
  char *buffer = (char *)malloc(buffer_size);
  int c;

  if (buffer == NULL) {
    perror("Errore malloc");
    return NULL;
  }

  while ((c = fgetc(fp)) != EOF) {
    if (c == delimiter) {
      break;
    }

    if (c == '\n') {
      ungetc(c, fp); // Lascia '\n' per la prossima chiamata
      break;
    }

    buffer[length++] = (char)c;

    if (length >= buffer_size) {
      buffer_size *= 2;
      char *temp = (char *)realloc(buffer, buffer_size);

      if (temp == NULL) {
        free(buffer);
        perror("Errore realloc");
        return NULL;
      }
      buffer = temp;
    }
  }

  if (length == 0 && c == EOF) {
    free(buffer);
    return NULL;
  }

  char *final_buffer = (char *)realloc(buffer, length + 1);

  if (final_buffer == NULL) {
    free(buffer);
    perror("Errore realloc finale");
    return NULL;
  }

  final_buffer[length] = '\0';
  return final_buffer;
}

static char *trim_whitespace(char *str) {
  if (!str) {
    return NULL;
  }

  char *end;
  char *start = str;

  while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
    start++;
  }

  if (*start == '\0') {
    // Se la stringa è vuota o composta solo da spazi
    if (start != str) {
      memmove(str, start, 1);
    }
    return str;
  }

  end = str + strlen(str) - 1;
  while (end >= start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
    end--;
  }

  *(end + 1) = '\0';

  if (start != str) {
    memmove(str, start, strlen(start) + 1);
  }

  return str;
}

static size_t find_entry(Entry *entries, size_t count, const char *path) {
  for (size_t i = 0; i < count; i++) {
    if (entries[i].path != NULL && strcmp(entries[i].path, path) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

static void free_entries(Entry *entries, size_t count) {
  if (entries == NULL)
    return;
  for (size_t i = 0; i < count; i++) {
    free(entries[i].hash);
    free(entries[i].path);
  }
  free(entries);
}

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

  if (existing_hash != NULL && strcmp(existing_hash, "X") != 0) {
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

  unsigned char *file_content = (unsigned char *)malloc(size > 0 ? size : 1);
  if (!file_content) {
    fclose(fd);
    return;
  }
  size_t read_bytes = 0;
  if (size > 0)
    read_bytes = fread(file_content, 1, size, fd);
  fclose(fd);
  if (read_bytes != size && size > 0) {
    free(file_content);
    return;
  }

  size_t header_len_str = (size_t)snprintf(NULL, 0, "blob %lu", (long unsigned int)size);
  size_t total_in_len = header_len_str + 1 + size;
  unsigned char *final_content = (unsigned char *)malloc(total_in_len);
  if (final_content == NULL) {
    free(file_content);
    return;
  }

  snprintf((char *)final_content, total_in_len, "blob %lu", (long unsigned int)size);
  final_content[header_len_str] = '\0';
  memcpy(final_content + header_len_str + 1, file_content, size);
  free(file_content);

  size_t hash_len;
  unsigned char *hash = create_hash(final_content, total_in_len, &hash_len);
  char *hex_hash = hash_to_hex(hash, hash_len);

  if (!blob_exists(hex_hash)) {
    create_blob(final_content, total_in_len, hex_hash);
  }

  fprintf(prep, "%s %s\n", hex_hash, path);
  printf("Added \"%s\" to prep - size: %lu bytes\n", path, size);

  free(final_content);
  free(hash);
  free(hex_hash);
}

// Recursive function for DFX index
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
      dir_path_for_prep = realloc(dir_path_for_prep, strlen(dir_path_for_prep) + 2);
      strcat(dir_path_for_prep, "/");
    }
  }

  // Write directory if not already written
  if (!ht_lookup(pdata->written_dirs, dir_path_for_prep)) {

    fprintf(pdata->prep, "X %s\n", dir_path_for_prep);
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

    if (pdata->entries) {
      size_t idx = find_entry(pdata->entries, pdata->entry_count, file_paths[i]);

      // Get updated hash if not existing
      if (pdata->updated_hash != NULL && strcmp(file_paths[i], pdata->updated_path) == 0) {
        hash_to_write = pdata->updated_hash;
      }
      // Get old hash
      else if (idx != (size_t)-1) {
        hash_to_write = pdata->entries[idx].hash;
      }
    }

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

char prepare_all(char *path) {
  if (access(".svm", F_OK) != 0) {
    if (mkdir(".svm", 0755) != 0) {
      perror("Error creating .svm directory");
      return 'e';
    }
  }

  if (access(".svm/objects", F_OK) != 0) {
    if (mkdir(".svm/objects", 0755) != 0) {
      perror("Error creating .svm/objects directory");
      return 'e';
    }
  }

  FILE *prep_w = fopen(".svm/prep", "w");
  if (prep_w == NULL) {
    printf("Error: Couldn't open \".svm/prep\" for writing.\n");
    return 'e';
  }

  ProcessData pdata = {prep_w, ht_create(), NULL, 0, ".", NULL};
  if (pdata.written_dirs == NULL) {
    fclose(prep_w);
    return 'e';
  }

  dfs_traverse_and_process(path, &pdata);

  fclose(prep_w);
  ht_free(pdata.written_dirs);

  return 's';
}

char *prepare_file(char *path) {
  FILE *prep_r = fopen(".svm/prep", "r");
  if (prep_r == NULL) {
    printf("Warning: .svm/prep not found. Running prepare_all.\n");
    if (prepare_all(".") == 's') {
      return prepare_file(path);
    }
    return NULL;
  }

  size_t entry_count = 0;
  size_t entry_capacity = 16;
  Entry *entries = (Entry *)malloc(entry_capacity * sizeof(Entry));
  if (!entries) {
    perror("malloc failed");
    fclose(prep_r);
    return NULL;
  }

  while (1) {
    char *hash_str = read_until_char(prep_r, ' ');
    if (hash_str == NULL)
      break;

    char *path_str = read_until_char(prep_r, '\n');
    if (path_str == NULL) {
      free(hash_str);
      break;
    }

    char *trimmed_path = strdup(path_str);
    trim_whitespace(trimmed_path);

    if (entry_count >= entry_capacity) {
      entry_capacity *= 2;
      entries = (Entry *)realloc(entries, entry_capacity * sizeof(Entry));
      if (!entries) {
        free(hash_str);
        free(path_str);
        break;
      }
    }

    entries[entry_count].hash = hash_str;
    entries[entry_count].path = trimmed_path;
    free(path_str);

    entry_count++;
  }
  fclose(prep_r);

  struct stat st;
  if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
    printf("Error: \"%s\" not found or is a directory.\n", path);
    free_entries(entries, entry_count);
    return NULL;
  }

  FILE *fd = fopen(path, "rb");
  if (!fd) {
    printf("Error: Could not open file \"%s\".\n", path);
    free_entries(entries, entry_count);
    return NULL;
  }

  long file_size = st.st_size;
  size_t size = (size_t)file_size;

  unsigned char *file_content = (unsigned char *)malloc(size > 0 ? size : 1);
  if (!file_content) {
    fclose(fd);
    free_entries(entries, entry_count);
    return NULL;
  }
  size_t read_bytes = 0;
  if (size > 0)
    read_bytes = fread(file_content, 1, size, fd);
  fclose(fd);
  if (read_bytes != size && size > 0) {
    free(file_content);
    free_entries(entries, entry_count);
    return NULL;
  }

  size_t header_len_str = (size_t)snprintf(NULL, 0, "blob %lu", (long unsigned int)size);
  size_t total_in_len = header_len_str + 1 + size;
  unsigned char *final_content = (unsigned char *)malloc(total_in_len);
  if (final_content == NULL) {
    free(file_content);
    free_entries(entries, entry_count);
    return NULL;
  }

  snprintf((char *)final_content, total_in_len, "blob %lu", (long unsigned int)size);
  final_content[header_len_str] = '\0';
  memcpy(final_content + header_len_str + 1, file_content, size);
  free(file_content);

  size_t hash_len;
  unsigned char *hash = create_hash(final_content, total_in_len, &hash_len);
  char *new_hex_hash = hash_to_hex(hash, hash_len);

  if (!blob_exists(new_hex_hash)) {
    create_blob(final_content, total_in_len, new_hex_hash);
  }

  free(final_content);
  free(hash);

  FILE *prep_w = fopen(".svm/prep", "w");
  if (prep_w == NULL) {
    free_entries(entries, entry_count);
    free(new_hex_hash);
    return NULL;
  }

  ProcessData pdata = {prep_w, ht_create(), entries, entry_count, path, new_hex_hash};
  if (pdata.written_dirs == NULL) {
    fclose(prep_w);
    free_entries(entries, entry_count);
    free(new_hex_hash);
    return NULL;
  }

  dfs_traverse_and_process(".", &pdata);

  fclose(prep_w);
  ht_free(pdata.written_dirs);

  free_entries(entries, entry_count);

  return new_hex_hash;
}
