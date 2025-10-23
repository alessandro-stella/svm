#include "queue.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void print_queue(Queue *head) {
  Queue *ref = head;

  while (ref != NULL) {
    printf("\nDepth: %d\n", ref->depth);

    Node *path_ref = ref->path_head;

    while (path_ref != NULL) {
      printf("%s\n", path_ref->path);
      path_ref = path_ref->next;
    }

    ref = ref->next;
  }
}

void free_queue(Queue *head) {
  Queue *q_current = head;

  while (q_current != NULL) {
    Node *n_current = q_current->path_head;

    while (n_current != NULL) {
      Node *n_next = n_current->next;
      free(n_current->path);
      free(n_current);
      n_current = n_next;
    }

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
    ref->next = malloc(sizeof(Queue));

    ref = ref->next;
    ref->next = next_ref;

    ref->depth = depth;
    ref->path_head = malloc(sizeof(Node));

    ref->path_head->path = path;
    ref->path_head->next = NULL;
  } else {
    if (ref->path_head == NULL) {
      ref->path_head = malloc(sizeof(Node));
      ref->path_head->path = path;
      ref->path_head->next = NULL;
    } else {
      Node *n = ref->path_head;

      while (n->next)
        n = n->next;

      n->next = malloc(sizeof(Node));
      n->next->path = path;
      n->next->next = NULL;
    }
  }
}

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

Queue *traverse(Queue *head, char *fn, int depth) {
  DIR *dir;
  struct dirent *entry;
  struct stat info;

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

  return head;
}

Queue *init_queue() {
  Queue *head = malloc(sizeof(Queue));
  head->depth = 0;
  head->path_head = NULL;
  head->next = NULL;

  return head;
}
