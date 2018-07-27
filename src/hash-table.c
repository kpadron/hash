// hash-table.c
// kpadron.github@gmail.com
// Kristian Padron
// implementation for hash table object
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "hash.h"
#include "hash-table.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))


// Compute the next highest power of 2
static inline uint32_t _up2(uint32_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x + (x == 0);
}

// Maps a number to the range [0, n)
static inline uint32_t _mod(uint32_t x, uint32_t n)
{
    return x % n;
}

// Maps a number to the range [0, n) fast if n is a power of 2
static inline uint32_t _map2(uint32_t x, uint32_t n)
{
    return x & (n - 1);
}

// Maps a number to the range [0, n) faster than modulo division
static inline uint32_t _map32(uint32_t x, uint32_t n)
{
    return ((uint64_t) x * (uint64_t) n) >> 32;
}


// Perform one-at-a-time hash on input bytes
static inline uint32_t _hash_oaat(void* key, size_t length)
{
    uint8_t* p = (uint8_t*) key;
    register uint32_t h = 0;

    for (size_t i = 0; i < length; i++)
    {
        h += p[i];
        h += h << 10;
        h ^= h >> 6;
    }

    h += h << 3;
    h ^= h >> 11;
    h += h << 15;

    return h;
}


// Perform FNV-1a hash on input bytes
static inline uint32_t _hash_fnv1a(void* key, size_t length)
{
    uint8_t* p = (uint8_t*) key;
    register uint32_t h = 2166136261;

    for (size_t i = 0; i < length; i++)
    {
        h = (h ^ p[i]) * 16777619;
    }

    return h;
}


// Iniatialize bucket object
static void _bucket_init(bucket_t* bucket)
{
    bucket->count = 0;
    bucket->size = 0;
    bucket->chain = NULL;
}


// Return index to the position of entry with specified key O(log N)
static uint32_t _bucket_index_bsearch(bucket_t* bucket, int (*keycmp)(void*, void*), void* key)
{
    entry_t* sorted = bucket->chain;

    uint32_t l = 0;
    uint32_t u = bucket->count;

    // Perform a binary search by comparing to middle element
    while (l < u)
    {
        // Update pivot index to be middle of the range
        uint32_t p = l + ((u - l) >> 1);

        // Compare key to pivot
        int comparison = keycmp(key, sorted[p].key);

        // Use lower half as new range
        if (comparison < 0) u = p;
        // Use upper half as new range
        else if (comparison > 0) l = p + 1;
        // Return matching pivot
        else return p;
    }

    return l;
}


// Return data of entry with specified key O(log N)
static void* _bucket_bsearch(bucket_t* bucket, int (*keycmp)(void*, void*), void* key)
{
    void* data = NULL;

    // Find matching entry O(log N)
    uint32_t index = _bucket_index_bsearch(bucket, keycmp, key);
    entry_t* chain = bucket->chain;

    // Return matching entry
    if (index < bucket->count && !keycmp(key, chain[index].key))
    {
        data = chain[index].data;
    }

    return data;
}

// Insert new entry into bucket in sorted order O(N)
static void _bucket_binsert(bucket_t* bucket, int (*keycmp)(void*, void*), void* key, void* data)
{
    // Expand bucket memory if necessary
    if (bucket->count == bucket->size)
    {
        bucket->size += HASH_BLOCK_SIZE;
        bucket->chain = (entry_t*) realloc(bucket->chain, bucket->size * sizeof(entry_t));
    }

    // Determine position to insert at O(log N)
    uint32_t index = _bucket_index_bsearch(bucket, keycmp, key);
    entry_t* chain = bucket->chain;

    // Update existing entry (duplicates not allowed!)
    if (index < bucket->count && !keycmp(key, chain[index].key))
    {
        chain[index].data = data;
    }
    // Insert and shift entries into place O(N)
    else if (index <= bucket->count)
    {
        memmove(&chain[index + 1], &chain[index], (bucket->count++ - index) * sizeof(entry_t));
        chain[index].key = key;
        chain[index].data = data;
    }
}

static void* _bucket_bremove(bucket_t* bucket, int (*keycmp)(void*, void*), void* key)
{
    void* data = NULL;

    // Find matching entry O(log N)
    uint32_t index = _bucket_index_bsearch(bucket, keycmp, key);
    entry_t* chain = bucket->chain;

    // Shift entries into place O(N)
    if (index < bucket->count && !keycmp(key, chain[index].key))
    {
        data = chain[index].data;
        memmove(&chain[index], &chain[index + 1], (--bucket->count - index) * sizeof(entry_t));
    }

    return data;
}


// Create new larger hash table
static void _hash_rehash(hash_t* table, uint32_t size)
{
    if (!table || size == table->size) return;

    bucket_t* old_buckets = table->buckets;
    uint32_t old_size = table->size;

    hash_init(table, size, table->keysize, table->keyhash, table->keycmp);

    for (uint32_t i = 0; i < old_size; i++)
    {
        bucket_t* bucket = &old_buckets[i];

        for (uint32_t j = 0; j < bucket->count; j++)
        {
            entry_t* entry = &bucket->chain[j];

            hash_insert(table, entry->key, entry->data);
        }

        free(bucket->chain);
    }

    free(old_buckets);
}


// Initialize a hash table object
void hash_init(hash_t* table, uint32_t size, size_t (*keysize)(void*), int (*keycmp)(void*, void*), uint32_t (*keyhash)(void*, size_t), uint32_t (*hashmap)(uint32_t, uint32_t))
{
    if (!table) return;

    // Initialize table functions
    table->keysize = keysize;
    table->keycmp = keycmp;
    table->keyhash = keyhash ? keyhash : hash_fnv1a;
    table->hashmap = hashmap ? hashmap : _map2;

    // Initialize size and allocate buckets
    table->entries = 0;
    table->size = table->hashmap == _map2 ? _up2(size) : size;
    table->buckets = (bucket_t*) malloc(table->size * sizeof(bucket_t));

    // Initialize buckets
    for (uint32_t i = 0; i < table->size; i++)
    {
        _bucket_init(&table->buckets[i]);
    }
}


// Cleanup and deallocate a hash table object
void hash_free(hash_t* table, void (*keyfree)(void*), void(*datafree)(void*))
{
    if (!table) return;

    for (uint32_t i = 0; i < table->size; i++)
    {
        bucket_t* bucket = &table->buckets[i];

        // Free key and data if free functions are provided
        if (keyfree || datafree)
        {
            for (uint32_t j = 0; j < bucket->count; j++)
            {
                entry_t* entry = &bucket->chain[j];

                if (keyfree) keyfree(entry->key);
                if (datafree) datafree(entry->data);
            }
        }

        free(bucket->chain);
    }

    free(table->buckets);
}


// Insert new entry into hash table using specified key O(1)
void hash_insert(hash_t* table, void* key, void* data)
{
    if (!table) return;

    // Resize table if necessary
    if (table->entries / table->size >= HASH_MAX_ALPHA) _hash_rehash(table, MAX(table->size * HASH_GROWTH_FACTOR, 8));

    // Determine which bucket to process
    uint32_t hash = table->keyhash(key, table->keysize(key));
    uint32_t index = _map32(hash, table->size);

    // Insert into bucket
    _bucket_binsert(&table->buckets[index], table->keycmp, key, data);
    table->entries++;
}


// Return data of the entry with specified key O(1)
void* hash_search(hash_t* table, void* key)
{
    if (!table) return NULL;

    // Determine which bucket to process
    uint32_t hash = _hash_oaat(key, table->keysize(key));
    uint32_t index = _map32(hash, table->size);

    // Search bucket for key
    void* data = _bucket_bsearch(&table->buckets[index], table->keycmp, key);

    return data;
}


// Remove entry with specified key returning data O(1)
void* hash_remove(hash_t* table, void* key)
{
    if (!table || !table->entries) return NULL;

    // Determine which bucket to process
    uint32_t hash = _hash_oaat(key, table->keysize(key));
    uint32_t index = _map32(hash, table->size);

    // Remove entry from bucket
    void* data = _bucket_bremove(&table->buckets[index], table->keycmp, key);
    if (data) table->entries--;

    // Resize table if necessary
    if (table->entries / table->size < HASH_MAX_ALPHA / 4) _hash_rehash(table, MAX(table->size / 2, 8));

    return data;
}


// Print table statistics
void hash_print_stats(hash_t* table)
{
    if (!table) return;

    uint32_t max = 0;
    uint32_t min = UINT32_MAX;
    uint32_t avg = 0;

    for (uint32_t i = 0; i < table->size; i++)
    {
        bucket_t* bucket = &table->buckets[i];

        if (bucket->count < min) min = bucket->count;
        if (bucket->count > max) max = bucket->count;
        avg += bucket->count;
    }

    printf("entries: %zu, size: %zu, alpha %.2f\n", (size_t) table->entries, (size_t) table->size, (float) table->entries / table->size);
    printf("min-depth: %zu, avg-depth: %.0f, max-depth: %zu\n", (size_t) min, (float) avg / table->size, (size_t) max);
    printf("overhead in bytes: %zu\n", (size_t) avg * sizeof(entry_t) + table->size * sizeof(bucket_t) + sizeof(hash_t));
}


// Debug print used to visualize hash table
void hash_print_debug(hash_t* table)
{
    if (!table) return;

    for (uint32_t i = 0; i < table->size; i++)
    {
        bucket_t* bucket = &table->buckets[i];

        printf("[%zu] ", (size_t) i);
        for (uint32_t j = 0; j < bucket->count; j++) printf("*");
        printf("\n");
    }
}
