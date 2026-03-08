#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

struct HashMapBucket {
    int occupied;
    char data[];
};

struct HashMap {
    void* buckets;
    size_t bucket_count;
    size_t count;
    size_t key_size;
    size_t value_size;
    size_t bucket_size;
    HashFunc hash_func;
    EqualFunc equal_func;
};

static const size_t DEFAULT_BUCKET_COUNT = 64;
static const float LOAD_FACTOR_THRESHOLD = 0.75f;

static size_t default_hash(const void* key, size_t key_size) {
    const unsigned char* bytes = (const unsigned char*)key;
    size_t hash = 5381;
    for (size_t i = 0; i < key_size; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }
    return hash;
}

static bool default_equal(const void* a, const void* b, size_t key_size) {
    return memcmp(a, b, key_size) == 0;
}

static char* bucket_key(char* bucket) {
    return bucket + sizeof(struct HashMapBucket);
}

static char* bucket_value(char* bucket, size_t key_size) {
    return bucket + sizeof(struct HashMapBucket) + key_size;
}

HashMap* hashmap_create(size_t key_size, size_t value_size) {
    if (key_size == 0 || value_size == 0) return NULL;
    
    HashMap* map = (HashMap*)malloc(sizeof(HashMap));
    if (!map) return NULL;
    
    size_t bucket_size = sizeof(struct HashMapBucket) + key_size + value_size;
    
    map->key_size = key_size;
    map->value_size = value_size;
    map->bucket_size = bucket_size;
    map->bucket_count = DEFAULT_BUCKET_COUNT;
    map->count = 0;
    map->buckets = calloc(DEFAULT_BUCKET_COUNT, bucket_size);
    
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    map->hash_func = default_hash;
    map->equal_func = default_equal;
    
    return map;
}

void hashmap_destroy(HashMap* map) {
    if (!map) return;
    free(map->buckets);
    free(map);
}

void hashmap_set_hash_func(HashMap* map, HashFunc func) {
    if (map && func) map->hash_func = func;
}

void hashmap_set_equal_func(HashMap* map, EqualFunc func) {
    if (map && func) map->equal_func = func;
}

static void rehash(HashMap* map) {
    size_t new_bucket_count = map->bucket_count * 2;
    void* new_buckets = calloc(new_bucket_count, map->bucket_size);
    if (!new_buckets) return;
    
    for (size_t i = 0; i < map->bucket_count; i++) {
        char* old_bucket = (char*)map->buckets + (i * map->bucket_size);
        struct HashMapBucket* old_entry = (struct HashMapBucket*)old_bucket;
        
        if (old_entry->occupied) {
            size_t hash = map->hash_func(bucket_key(old_bucket), map->key_size);
            size_t idx = hash % new_bucket_count;
            
            char* new_bucket = (char*)new_buckets + (idx * map->bucket_size);
            memcpy(new_bucket, old_bucket, map->bucket_size);
        }
    }
    
    free(map->buckets);
    map->buckets = new_buckets;
    map->bucket_count = new_bucket_count;
}

void hashmap_set(HashMap* map, const void* key, const void* value) {
    if (!map || !key || !value) return;
    
    if ((float)map->count / (float)map->bucket_count > LOAD_FACTOR_THRESHOLD) {
        rehash(map);
    }
    
    size_t hash = map->hash_func(key, map->key_size);
    size_t idx = hash % map->bucket_count;
    
    for (size_t i = 0; i < map->bucket_count; i++) {
        size_t probe = (idx + i) % map->bucket_count;
        char* bucket = (char*)map->buckets + (probe * map->bucket_size);
        struct HashMapBucket* entry = (struct HashMapBucket*)bucket;
        
        if (!entry->occupied) {
            entry->occupied = 1;
            memcpy(bucket_key(bucket), key, map->key_size);
            memcpy(bucket_value(bucket, map->key_size), value, map->value_size);
            map->count++;
            return;
        }
        
        if (map->equal_func(bucket_key(bucket), key, map->key_size)) {
            memcpy(bucket_value(bucket, map->key_size), value, map->value_size);
            return;
        }
    }
}

void* hashmap_get(HashMap* map, const void* key) {
    if (!map || !key) return NULL;
    
    size_t hash = map->hash_func(key, map->key_size);
    size_t idx = hash % map->bucket_count;
    
    for (size_t i = 0; i < map->bucket_count; i++) {
        size_t probe = (idx + i) % map->bucket_count;
        char* bucket = (char*)map->buckets + (probe * map->bucket_size);
        struct HashMapBucket* entry = (struct HashMapBucket*)bucket;
        
        if (!entry->occupied) return NULL;
        
        if (map->equal_func(bucket_key(bucket), key, map->key_size)) {
            return bucket_value(bucket, map->key_size);
        }
    }
    
    return NULL;
}

bool hashmap_contains(HashMap* map, const void* key) {
    return hashmap_get(map, key) != NULL;
}

void hashmap_remove(HashMap* map, const void* key) {
    if (!map || !key) return;
    
    size_t hash = map->hash_func(key, map->key_size);
    size_t idx = hash % map->bucket_count;
    
    for (size_t i = 0; i < map->bucket_count; i++) {
        size_t probe = (idx + i) % map->bucket_count;
        char* bucket = (char*)map->buckets + (probe * map->bucket_size);
        struct HashMapBucket* entry = (struct HashMapBucket*)bucket;
        
        if (!entry->occupied) continue;
        
        if (map->equal_func(bucket_key(bucket), key, map->key_size)) {
            entry->occupied = 0;
            map->count--;
            return;
        }
    }
}

void hashmap_clear(HashMap* map) {
    if (!map) return;
    memset(map->buckets, 0, map->bucket_count * map->bucket_size);
    map->count = 0;
}

size_t hashmap_count(const HashMap* map) {
    return map ? map->count : 0;
}
