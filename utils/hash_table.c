#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SIZE 101
#define MAX_LOAD_FACTOR 0.75

static size_t get_next_prime(size_t n) {
  if (n < 2)
    return 2;
  size_t next = n % 2 == 0 ? n + 1 : n + 2;
  while (1) {
    size_t i;
    for (i = 3; i * i <= next; i += 2) {
      if (next % i == 0)
        break;
    }
    if (i * i > next)
      return next;
    next += 2;
  }
}

static size_t hash_function(const char *key, size_t size) {
  unsigned long hash = 5381;
  int c;

  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }

  return hash % size;
}

static int ht_resize(HashTable *ht) {
  size_t old_size = ht->size;
  size_t new_size = get_next_prime(old_size * 2);

  HNode **new_buckets = (HNode **)calloc(new_size, sizeof(HNode *));
  if (new_buckets == NULL) {
    return 0;
  }

  HNode **old_buckets = ht->buckets;

  ht->buckets = new_buckets;
  ht->size = new_size;
  ht->count = 0;

  for (size_t i = 0; i < old_size; i++) {
    HNode *current = old_buckets[i];
    while (current != NULL) {
      HNode *next_hnode = current->next;

      size_t index = hash_function(current->key, ht->size);
      current->next = ht->buckets[index];
      ht->buckets[index] = current;
      ht->count++;

      current = next_hnode;
    }
  }

  free(old_buckets);
  return 1;
}

HashTable *ht_create() {
  HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
  if (ht == NULL) {
    return NULL;
  }

  ht->size = INITIAL_SIZE;
  ht->count = 0;
  ht->buckets = (HNode **)calloc(ht->size, sizeof(HNode *));

  if (ht->buckets == NULL) {
    free(ht);
    return NULL;
  }

  return ht;
}

void ht_free(HashTable *ht) {
  if (ht == NULL)
    return;

  for (size_t i = 0; i < ht->size; i++) {
    HNode *current = ht->buckets[i];
    while (current != NULL) {
      HNode *next_hnode = current->next;
      free(current->key);
      free(current);
      current = next_hnode;
    }
  }

  free(ht->buckets);
  free(ht);
}

int ht_lookup(HashTable *ht, const char *key) {
  size_t index = hash_function(key, ht->size);
  HNode *current = ht->buckets[index];

  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      return 1;
    }
    current = current->next;
  }

  return 0;
}

void ht_insert(HashTable *ht, const char *key) {

  if ((double)ht->count / ht->size >= MAX_LOAD_FACTOR) {
    if (!ht_resize(ht)) {
      return;
    }
  }

  if (ht_lookup(ht, key)) {
    return;
  }

  size_t index = hash_function(key, ht->size);
  HNode *new_node;

  new_node = (HNode *)malloc(sizeof(HNode));
  if (new_node == NULL)
    return;

  new_node->key = strdup(key);
  new_node->next = ht->buckets[index];
  ht->buckets[index] = new_node;

  ht->count++;
}
