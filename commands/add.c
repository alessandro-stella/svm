#include "../svm_commands.h"
#include "../utils/blob_handler.h"
#include "../utils/hashing.h"

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} dynamic_string_t;

void init_dynamic_string(dynamic_string_t *ds) {
  ds->data = NULL;
  ds->size = 0;
  ds->capacity = 0;
}

void free_dynamic_string(dynamic_string_t *ds) {
  if (ds->data != NULL) {
    free(ds->data);
  }
  ds->data = NULL;
  ds->size = 0;
  ds->capacity = 0;
}

int append_dynamic_string(dynamic_string_t *ds, const char *s) {
  size_t s_len = strlen(s);
  size_t required_capacity = ds->size + s_len + 1;

  if (required_capacity > ds->capacity) {
    size_t new_capacity = ds->capacity > 0 ? ds->capacity : 1;
    while (new_capacity < required_capacity) {
      new_capacity *= 2;
    }

    char *new_data = realloc(ds->data, new_capacity);
    if (new_data == NULL) {
      return -1;
    }

    ds->data = new_data;
    ds->capacity = new_capacity;
  }

  memcpy(ds->data + ds->size, s, s_len);

  ds->size += s_len;
  ds->data[ds->size] = '\0';

  return 0;
}

size_t count_lines(FILE *fd) {
  int c;
  size_t lines = 0;

  while ((c = fgetc(fd)) != EOF) {
    if (c == '\n') {
      lines++;
    }
  }

  if (ftell(fd) > 0 && c != '\n') {
    lines++;
  }

  rewind(fd);
  return lines;
}

void freeLines(char **lines, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (lines[i] != NULL) {
      free(lines[i]);
    }
  }
  free(lines);
}

void split_line2(const char *input_line, char **first_half, char **second_half) {
  const char *delimiter = " ";
  char *context;

  char *line_copy = strdup(input_line);
  if (line_copy == NULL) {
    *first_half = NULL;
    *second_half = NULL;
    return;
  }

  *first_half = strtok_r(line_copy, delimiter, &context);
  *second_half = strtok_r(NULL, delimiter, &context);

  if (*second_half != NULL)
    (*second_half)[strcspn(*second_half, "\n\r")] = '\0';
}

char *create_tree(char **lines, size_t lines_len, size_t start_index, size_t *end_index_out) {
  dynamic_string_t content;
  init_dynamic_string(&content);

  char *type_start = NULL, *path_start = NULL;

  if (start_index >= lines_len || lines[start_index] == NULL) {
    *end_index_out = start_index;
    return NULL;
  }

  char *line_copy_start = strdup(lines[start_index]);
  if (line_copy_start == NULL)
    return NULL;
  split_line2(line_copy_start, &type_start, &path_start);

  size_t path_len = strlen(path_start);
  if (path_len > 0 && path_start[path_len - 1] == '/') {
    path_start[path_len - 1] = '\0';
    path_len--;
  }

  size_t path_prefix_len = strlen(path_start) + 1;
  char *path_prefix = (char *)malloc(path_prefix_len + 1);
  if (path_prefix == NULL) {
    free(line_copy_start);
    return NULL;
  }
  snprintf(path_prefix, path_prefix_len + 1, "%s/", path_start);

  free(line_copy_start);

  size_t i = start_index + 1;

  while (i < lines_len) {
    char *entry_type = NULL, *entry_path = NULL;

    char *line_copy = strdup(lines[i]);
    if (line_copy == NULL) {
      i++;
      continue;
    }

    char *ptr_to_free = line_copy;
    split_line2(line_copy, &entry_type, &entry_path);

    if (entry_path == NULL || strncmp(entry_path, path_prefix, strlen(path_prefix)) != 0) {
      free(ptr_to_free);
      break;
    }

    char *entry_name = entry_path + strlen(path_prefix);

    if (strcmp(entry_type, "tree") == 0) {
      size_t recursion_end_index = i;
      char *new_tree_hash = create_tree(lines, lines_len, i, &recursion_end_index);

      if (new_tree_hash == NULL) {
        free(ptr_to_free);
        free(path_prefix);
        free_dynamic_string(&content);
        *end_index_out = recursion_end_index;
        return NULL;
      }

      size_t new_line_len = strlen("tree ") + strlen(new_tree_hash) + strlen(entry_name) + 4;
      char *new_line = (char *)malloc(new_line_len);
      snprintf(new_line, new_line_len, "tree %s %s\n", new_tree_hash, entry_name);

      append_dynamic_string(&content, new_line);

      free(new_line);
      free(new_tree_hash);

      i = recursion_end_index;
    } else {
      size_t blob_line_len = strlen(entry_type) + strlen(entry_name) + strlen("blob") + 4;
      char *blob_line = (char *)malloc(blob_line_len);

      snprintf(blob_line, blob_line_len, "blob %s %s\n", entry_type, entry_name);

      append_dynamic_string(&content, blob_line);
      free(blob_line);

      i++;
    }

    free(ptr_to_free);
  }

  free(path_prefix);

  *end_index_out = i;

  size_t header_len_str = (size_t)snprintf(NULL, 0, "blob %lu", (long unsigned int)content.size);
  size_t total_in_len = header_len_str + 1 + content.size;

  unsigned char *final_content = (unsigned char *)malloc(total_in_len);
  if (final_content == NULL) {
    free_dynamic_string(&content);
    return NULL;
  }

  snprintf((char *)final_content, total_in_len, "blob %lu", (long unsigned int)content.size);
  memcpy(final_content + header_len_str + 1, content.data, content.size);

  free_dynamic_string(&content);

  size_t hash_len;
  unsigned char *hash = create_hash(final_content, total_in_len, &hash_len);
  char *hex_hash = hash_to_hex(hash, hash_len);

  if (create_blob(final_content, total_in_len, hex_hash) != Z_OK) {
    free(hash);
    free(hex_hash);
    free(final_content);
    return NULL;
  }

  free(hash);
  free(final_content);

  return hex_hash;
}

char *add_command(char *msg) {
  FILE *current_dist_fd = fopen(".svm/current_dist", "r");
  FILE *prep_fd = fopen(".svm/prep", "r");

  if (current_dist_fd == NULL || prep_fd == NULL) {
    fprintf(stderr, "Error: couldn't read distribution files\n");
    if (current_dist_fd)
      fclose(current_dist_fd);
    if (prep_fd)
      fclose(prep_fd);
    return NULL;
  }

  fseek(current_dist_fd, 0, SEEK_END);
  size_t content_size = ftell(current_dist_fd);
  rewind(current_dist_fd);

  char *current_dist = (char *)malloc(content_size + 1);

  if (current_dist == NULL) {
    fprintf(stderr, "Allocation error for current_dist\n");
    fclose(current_dist_fd);
    fclose(prep_fd);
    return NULL;
  }

  if (content_size > 0) {
    size_t read_bytes = fread(current_dist, 1, content_size, current_dist_fd);
    if (read_bytes != content_size) {
    }
  }
  current_dist[content_size] = '\0';

  size_t dist_len = strlen(current_dist);
  while (dist_len > 0 && (current_dist[dist_len - 1] == '\n' || current_dist[dist_len - 1] == ' ' || current_dist[dist_len - 1] == '\r')) {
    current_dist[--dist_len] = '\0';
  }

  size_t lines_len_initial = count_lines(prep_fd);
  size_t lines_len = lines_len_initial;
  char **lines = (char **)malloc(sizeof(char *) * lines_len_initial);

  if (lines == NULL) {
    printf("Error while reading lines\n");
    free(current_dist);
    fclose(current_dist_fd);
    fclose(prep_fd);
    return NULL;
  }

  char *line_ptr = NULL;
  size_t len = 0;
  for (size_t i = 0; i < lines_len_initial; i++) {
    ssize_t read = getline(&line_ptr, &len, prep_fd);

    if (read == -1) {
      lines_len = i;
      break;
    }

    lines[i] = strdup(line_ptr);
    if (lines[i] == NULL) {
      lines_len = i;
      break;
    }
  }

  if (line_ptr != NULL) {
    free(line_ptr);
  }

  fclose(prep_fd);

  size_t lines_checked = 0;
  char *tree_hash = create_tree(lines, lines_len, 0, &lines_checked);

  if (tree_hash != NULL) {
    char dist_path[PATH_MAX];

    if (snprintf(dist_path, sizeof(dist_path), ".svm/dists/%s", current_dist) >= sizeof(dist_path)) {
      fprintf(stderr, "Error: Distribution path too long.\n");
    } else {
      FILE *dist_fd = fopen(dist_path, "a");
      if (dist_fd == NULL) {
        perror("Error opening distribution history file for writing");
      } else {
        fprintf(dist_fd, "%s %s\n", tree_hash, msg);
        printf("New tree hash %s added to distribution '%s' with message: %s\n", tree_hash, current_dist, msg);
        fclose(dist_fd);
      }
    }
  } else {
    fprintf(stderr, "Error: Failed to create tree.\n");
  }

  freeLines(lines, lines_len);
  free(current_dist);
  fclose(current_dist_fd);

  return tree_hash;
}
