// hash-table.h
// kpadron.github@gmail.com
// Kristian Padron
// hash table module
#pragma once
#include <stdlib.h>
#include <stdint.h>

#define HASH_BLOCK_SIZE 32
#define HASH_GROWTH_FACTOR 2
#define HASH_MAX_ALPHA 64

// Object representing a hash table entry
typedef struct
{
    void* key;
    void* data;
} entry_t;

// Object representing a hash table bucket chain
typedef struct
{
    uint32_t count;
    uint32_t size;
    entry_t* chain;
} bucket_t;

// Object representing a hash table
typedef struct
{
    uint64_t entries;
    uint32_t size;
    bucket_t* buckets;

    size_t (*keysize)(void*);
    int (*keycmp)(void*, void*);
    uint32_t (*keyhash)(void*, size_t);
    uint32_t (*hashmap)(uint32_t, uint32_t);
} hash_t;

// Initalize a hash table object
extern void hash_init(hash_t* table, uint32_t size, size_t (*keysize)(void*), int (*keycmp)(void*, void*), uint32_t (*keyhash)(void*, size_t), uint32_t (*hashmap)(uint32_t, uint32_t));

// Cleanup and deallocate a hash table object
extern void hash_free(hash_t* table, void (*keyfree)(void*), void (*datafree)(void*));

// Insert new entry into hash table using specified key O(1)
extern void hash_insert(hash_t* table, void* key, void* data);

// Return data of the entry with specified key O(1)
extern void* hash_search(hash_t* table, void* key);

// Remove entry with specified key returning data O(1)
extern void* hash_remove(hash_t* table, void* key);

// Print table statistics
extern void hash_print_stats(hash_t* table);

// Debug print used to visualize hash table
extern void hash_print_debug(hash_t* table);