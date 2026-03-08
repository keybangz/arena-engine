#ifndef ARENA_HASHMAP_H
#define ARENA_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct HashMap HashMap;

typedef size_t (*HashFunc)(const void* key, size_t key_size);
typedef bool (*EqualFunc)(const void* a, const void* b, size_t key_size);

HashMap* hashmap_create(size_t key_size, size_t value_size);
void hashmap_destroy(HashMap* map);

void hashmap_set(HashMap* map, const void* key, const void* value);
void* hashmap_get(HashMap* map, const void* key);
bool hashmap_contains(HashMap* map, const void* key);
void hashmap_remove(HashMap* map, const void* key);

void hashmap_clear(HashMap* map);
size_t hashmap_count(const HashMap* map);

void hashmap_set_hash_func(HashMap* map, HashFunc func);
void hashmap_set_equal_func(HashMap* map, EqualFunc func);

#endif
