#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

typedef struct HNode {
  char *key;
  struct HNode *next;
} HNode;

typedef struct {
  size_t size;
  size_t count;
  HNode **buckets;
} HashTable;

HashTable *ht_create();

void ht_free(HashTable *ht);

void ht_insert(HashTable *ht, const char *key);

int ht_lookup(HashTable *ht, const char *key);

#endif
