#ifndef QUEUE_H
#define QUEUE_H

#include "hash_table.h"

typedef struct weighted_queue {
  int depth;
  struct weighted_queue *next;
  HashTable *path_table;
} Queue;

void print_queue(Queue *head);

void free_queue(Queue *head);

void add_to_queue(Queue *head, char *path, int depth);

char *create_dynamic_path(const char *parent_dir, const char *name);

void traverse(Queue *head, char *fn, int depth);

Queue *init_queue();

#endif
