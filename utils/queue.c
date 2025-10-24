#include "queue.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *create_dynamic_path(const char *parent_dir, const char *name) {
  size_t len = strlen(parent_dir) + 1 + strlen(name) + 1;
  char *new_path = (char *)malloc(len);

  if (new_path == NULL) {
    perror("malloc failed");
    return NULL;
  }

  snprintf(new_path, len, "%s/%s", parent_dir, name);
  return new_path;
}

void print_queue(Queue *head) {
  Queue *ref = head;

  while (ref != NULL) {
    printf("\nDepth: %d (Total Files: %zu)\n", ref->depth, ref->path_table->count);

    for (size_t i = 0; i < ref->path_table->size; i++) {
      HNode *path_ref = ref->path_table->buckets[i];
      while (path_ref != NULL) {
        printf("%s\n", path_ref->key);
        path_ref = path_ref->next;
      }
    }
    ref = ref->next;
  }
}

void free_queue(Queue *head) {
  Queue *q_current = head;

  while (q_current != NULL) {
    ht_free(q_current->path_table);

    Queue *q_next = q_current->next;
    free(q_current);
    q_current = q_next;
  }
}

void add_to_queue(Queue *head, char *path, int depth) {
  Queue *ref = head;

  while (ref->next != NULL && ref->next->depth <= depth) {
    ref = ref->next;
  }

  if (ref->depth != depth) {
    Queue *next_ref = ref->next;
    Queue *new_queue = malloc(sizeof(Queue));
    if (new_queue == NULL) {
      perror("malloc failed");
      return;
    }

    new_queue->next = next_ref;
    new_queue->depth = depth;
    new_queue->path_table = ht_create();

    ref->next = new_queue;
    ref = new_queue;
  }

  ht_insert(ref->path_table, path);
  free(path);
}

void traverse(Queue *head, char *fn, int depth) {
  DIR *dir;
  struct dirent *entry;

  if ((dir = opendir(fn)) == NULL)
    perror("opendir() error");
  else {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] == '.') {
        continue;
      }

      char *current_path = create_dynamic_path(fn, entry->d_name);
      if (current_path == NULL)
        continue;

      if (entry->d_type == DT_DIR) {
        traverse(head, current_path, depth + 1);
        free(current_path);
      } else {
        add_to_queue(head, current_path, depth);
      }
    }

    closedir(dir);
  }
}

Queue *init_queue() {
  Queue *head = malloc(sizeof(Queue));
  if (head == NULL) {
    perror("malloc failed");
    return NULL;
  }
  head->depth = 0;
  head->path_table = ht_create();
  head->next = NULL;

  return head;
}
