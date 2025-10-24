#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

typedef struct HNode {
  char *key;
  struct HNode *next;
} HNode;

typedef struct HashTable {
  HNode **buckets;
  size_t size;
  size_t count;
} HashTable;

HashTable *ht_create();
void ht_free(HashTable *ht);
void ht_insert(HashTable *ht, const char *key);
int ht_lookup(HashTable *ht, const char *key);
size_t hash_function(const char *key, size_t size);
size_t get_next_prime(size_t n);
void ht_iterate_all(HashTable *ht, void (*callback)(const char *path, void *data), void *data);

#endif
