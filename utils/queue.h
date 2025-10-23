#ifndef QUEUE_H
#define QUEUE_H

typedef struct weighted_queue {
  int depth;
  struct weighted_queue *next;
  struct node *path_head;
} Queue;

typedef struct node {
  char *path;
  struct node *next;
} Node;

void print_queue(Queue *head);

void free_queue(Queue *head);

void add_to_queue(Queue *head, char *path, int depth);

char *create_dynamic_path(const char *parent_dir, const char *name);

Queue *traverse(Queue *head, char *fn, int depth);

Queue *init_queue();

#endif
