#include "../utils/blob_handler.h"
#include "../utils/hash_table.h"
#include "../utils/hashing.h"
#include "../utils/queue.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *read_until_char(FILE *fp, int delimiter) {
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
    buffer[length] = '\0';
    return buffer;
  }

  final_buffer[length] = '\0';
  return final_buffer;
}

int get_depth(char *path) {
  int depth = 0;
  char *c = path;

  if (strncmp(path, "./", 2) == 0) {
    c += 2;
  } else if (*path == '/') {
    c++;
  } else if (strchr(path, '/') == NULL) {
    return 0;
  }

  while ((c = strchr(c, '/')) != NULL) {
    depth++;
    c++;
  }

  return depth;
}

typedef struct {
  FILE *prep;
} ProcessData;

void process_path_callback(const char *path, void *data) {
  ProcessData *pdata = (ProcessData *)data;
  FILE *prep = pdata->prep;

  FILE *fd = fopen(path, "rb");
  if (!fd) {
    return;
  }

  fseek(fd, 0, SEEK_END);
  long file_size = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  if (file_size < 0) {
    printf("Error while preparing \"%s\"\n", path);
    fclose(fd);
    return;
  }

  size_t size = (size_t)file_size;

  unsigned char *file_content = (unsigned char *)malloc(size > 0 ? size : 1);
  if (!file_content) {
    printf("Error while preparing \"%s\"\n", path);
    fclose(fd);
    return;
  }

  size_t read_bytes = 0;
  if (size > 0) {
    read_bytes = fread(file_content, 1, size, fd);
  }
  fclose(fd);

  if (read_bytes != size && size > 0) {
    fprintf(stderr, "Error while retrieving current dist\n");
    free(file_content);
    return;
  }

  size_t header_len_str = (size_t)snprintf(NULL, 0, "blob %lu", (long unsigned int)size);
  size_t total_in_len = header_len_str + 1 + size;

  unsigned char *final_content = (unsigned char *)malloc(total_in_len);
  if (final_content == NULL) {
    free(file_content);
    printf("Error during blob creation\n");
    return;
  }

  snprintf((char *)final_content, total_in_len, "blob %lu", (long unsigned int)size);
  final_content[header_len_str] = '\0';
  memcpy(final_content + header_len_str + 1, file_content, size);
  free(file_content);

  size_t hash_len;
  unsigned char *hash = create_hash(final_content, total_in_len, &hash_len);
  char *hex_hash = hash_to_hex(hash, hash_len);

  printf("Added \"%s\" to prep - size: %lu bytes\n", path, size);

  if (!blob_exists(hex_hash)) {
    create_blob(final_content, total_in_len, hex_hash);
  } else {
    printf("Blob already exists!\n");
  }

  fprintf(prep, "%s %s\n", hex_hash, path);

  free(final_content);
  free(hash);
  free(hex_hash);
}

char prepare_all(char *path) {
  FILE *prep = fopen(".svm/prep", "w");
  if (prep == NULL) {
    printf("Couldn't find \"prep\" inside .svm\n");
    return 'e';
  }

  Queue *head = init_queue();
  if (head == NULL) {
    printf("Error while preparing project for distribution\n");
    fclose(prep);
    return 'e';
  }

  traverse(head, path, 0);

  Queue *ref = head;
  ProcessData data = {.prep = prep};

  while (ref != NULL) {
    ht_iterate_all(ref->path_table, process_path_callback, &data);
    ref = ref->next;
  }

  fclose(prep);
  free_queue(head);
  return 'a';
}

typedef struct {
  char *hash;
  char *path;
} PrepEntry;

PrepEntry *load_prep_file(FILE *prep, size_t *count) {
  *count = 0;
  size_t capacity = 10;
  PrepEntry *entries = (PrepEntry *)malloc(capacity * sizeof(PrepEntry));
  if (entries == NULL) {
    perror("malloc failed for PrepEntry");
    return NULL;
  }

  char *hash_data;
  char *path_data;

  while ((hash_data = read_until_char(prep, ' ')) != NULL) {
    path_data = read_until_char(prep, '\n');

    if (path_data == NULL) {
      free(hash_data);
      break;
    }

    if (*count >= capacity) {
      capacity *= 2;
      PrepEntry *temp = (PrepEntry *)realloc(entries, capacity * sizeof(PrepEntry));
      if (temp == NULL) {

        free(hash_data);
        free(path_data);
        return entries;
      }
      entries = temp;
    }

    entries[*count].hash = hash_data;
    entries[*count].path = path_data;
    (*count)++;
  }

  return entries;
}

size_t find_path_index(PrepEntry *entries, size_t count, const char *path) {
  for (size_t i = 0; i < count; i++) {
    if (entries[i].path != NULL && strcmp(entries[i].path, path) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

char prepare_file(char *path) {

  FILE *prep_r = fopen(".svm/prep", "r");
  if (prep_r == NULL) {

    FILE *prep_w_init = fopen(".svm/prep", "w");
    if (prep_w_init == NULL) {
      printf("Error: Could not open or create \".svm/prep\".\n");
      return 'e';
    }
    fclose(prep_w_init);
    prep_r = fopen(".svm/prep", "r");
    if (prep_r == NULL) {
      printf("Error: Could not reopen \".svm/prep\" for reading.\n");
      return 'e';
    }
  }

  size_t entry_count;
  PrepEntry *entries = load_prep_file(prep_r, &entry_count);
  fclose(prep_r);

  size_t size = 0;
  unsigned char *file_content = NULL;
  unsigned char *final_content = NULL;
  char *new_hex_hash = NULL;
  size_t total_in_len = 0;
  bool success = false;

  if (access(path, F_OK) == -1) {
    printf("Error: File \"%s\" not found or inaccessible.\n", path);

    for (size_t i = 0; i < entry_count; i++) {
      free(entries[i].hash);
      free(entries[i].path);
    }
    free(entries);
    return 'e';
  }

  do {
    FILE *fd = fopen(path, "rb");
    if (!fd)
      break;

    fseek(fd, 0, SEEK_END);
    long file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    if (file_size < 0) {
      fclose(fd);
      break;
    }
    size = (size_t)file_size;

    file_content = (unsigned char *)malloc(size > 0 ? size : 1);
    if (!file_content) {
      fclose(fd);
      break;
    }

    size_t read_bytes = 0;
    if (size > 0)
      read_bytes = fread(file_content, 1, size, fd);
    fclose(fd);

    if (read_bytes != size && size > 0) {
      free(file_content);
      break;
    }

    size_t header_len_str = (size_t)snprintf(NULL, 0, "blob %lu", (long unsigned int)size);
    total_in_len = header_len_str + 1 + size;

    final_content = (unsigned char *)malloc(total_in_len);
    if (final_content == NULL) {
      free(file_content);
      break;
    }

    snprintf((char *)final_content, total_in_len, "blob %lu", (long unsigned int)size);
    final_content[header_len_str] = '\0';
    memcpy(final_content + header_len_str + 1, file_content, size);
    free(file_content);

    size_t hash_len;
    unsigned char *hash = create_hash(final_content, total_in_len, &hash_len);
    new_hex_hash = hash_to_hex(hash, hash_len);
    free(hash);

    if (new_hex_hash == NULL)
      break;
    success = true;
  } while (0);

  if (!success) {
    printf("Error: Failed to hash file \"%s\".\n", path);
    if (file_content)
      free(file_content);
    if (final_content)
      free(final_content);
    for (size_t i = 0; i < entry_count; i++) {
      free(entries[i].hash);
      free(entries[i].path);
    }
    free(entries);
    return 'e';
  }

  size_t tracked_index = find_path_index(entries, entry_count, path);
  bool file_is_new = (tracked_index == (size_t)-1);
  bool hash_changed = false;

  if (file_is_new) {
    printf("File \"%s\" is untracked. Adding new entry with hash %s.\n", path, new_hex_hash);

    PrepEntry *temp = (PrepEntry *)realloc(entries, (entry_count + 1) * sizeof(PrepEntry));
    if (temp == NULL) {
      printf("Error realloc for new entry.\n");
      free(final_content);
      free(new_hex_hash);
      for (size_t i = 0; i < entry_count; i++) {
        free(entries[i].hash);
        free(entries[i].path);
      }
      free(entries);
      return 'e';
    }
    entries = temp;

    entries[entry_count].hash = new_hex_hash;
    entries[entry_count].path = strdup(path);
    entry_count++;
    hash_changed = true;

  } else {
    char *old_hash = entries[tracked_index].hash;
    if (strcmp(old_hash, new_hex_hash) != 0) {
      printf("File \"%s\" is tracked but changed. Updating hash to %s.\n", path, new_hex_hash);
      free(old_hash);
      entries[tracked_index].hash = new_hex_hash;
      hash_changed = true;
    } else {
      printf("File \"%s\" is already tracked and unchanged.\n", path);
      free(new_hex_hash);
      free(final_content);
    }
  }

  if (hash_changed) {
    char *current_hash_ptr = file_is_new ? entries[entry_count - 1].hash : entries[tracked_index].hash;

    if (!blob_exists(current_hash_ptr)) {
      if (create_blob(final_content, total_in_len, current_hash_ptr) == 0) {
        printf("Blob created successfully for hash %s.\n", current_hash_ptr);
      } else {
        printf("Error creating blob for hash %s.\n", current_hash_ptr);
      }
    } else {
      printf("Blob already exists for hash %s.\n", current_hash_ptr);
    }
    free(final_content);
  }

  FILE *prep_w = fopen(".svm/prep", "w");
  if (prep_w == NULL) {
    printf("Error: Couldn't open \".svm/prep\" for writing.\n");
    for (size_t i = 0; i < entry_count; i++) {
      free(entries[i].hash);
      free(entries[i].path);
    }
    free(entries);
    return 'e';
  }

  Queue *head = init_queue();
  if (head == NULL) {
    fclose(prep_w);
    for (size_t i = 0; i < entry_count; i++) {
      free(entries[i].hash);
      free(entries[i].path);
    }
    free(entries);
    return 'e';
  }

  for (size_t i = 0; i < entry_count; i++) {
    if (entries[i].path) {
      int depth = get_depth(entries[i].path);
      add_to_queue(head, strdup(entries[i].path), depth);
    }
  }

  Queue *q_ref = head;
  while (q_ref != NULL) {
    for (size_t i = 0; i < q_ref->path_table->size; i++) {
      HNode *current = q_ref->path_table->buckets[i];
      while (current != NULL) {
        size_t entry_idx = find_path_index(entries, entry_count, current->key);
        if (entry_idx != (size_t)-1) {
          fprintf(prep_w, "%s %s\n", entries[entry_idx].hash, current->key);
        }
        current = current->next;
      }
    }
    q_ref = q_ref->next;
  }

  fclose(prep_w);
  free_queue(head);

  for (size_t i = 0; i < entry_count; i++) {
    free(entries[i].hash);
    free(entries[i].path);
  }
  free(entries);

  printf("Successfully prepared file \"%s\" and updated .svm/prep file.\n", path);
  return 'a';
}
