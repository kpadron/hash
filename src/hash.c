// hash.c
// kpadron.github@gmail.com
// Kristian Padron
// implementation for hash table object
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

#define HASH_SEED 0x7FFF2FF8

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static inline int _lsb(void)
{
    const union { uint32_t u; uint8_t c[4]; } one = { 1 };
    return one.c[0];
}

static inline uint32_t _rotl32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

static inline uint64_t _rotl64(uint64_t x, int8_t r)
{
    return (x << r) | (x >> (64 - r));
}

static inline uint32_t _swap32(uint32_t x)
{
    return ((x << 24) & 0xFF000000) |
           ((x <<  8) & 0x00FF0000) |
           ((x >>  8) & 0x0000FF00) |
           ((x >> 24) & 0x000000FF);
}

static inline uint64_t _swap64(uint64_t x)
{
    return ((x << 56) & 0xFF00000000000000ULL) |
           ((x << 40) & 0x00FF000000000000ULL) |
           ((x << 24) & 0x0000FF0000000000ULL) |
           ((x <<  8) & 0x000000FF00000000ULL) |
           ((x >>  8) & 0x00000000FF000000ULL) |
           ((x >> 24) & 0x0000000000FF0000ULL) |
           ((x >> 40) & 0x000000000000FF00ULL) |
           ((x >> 56) & 0x00000000000000FFULL);
}

static inline uint32_t _read32(void* p)
{
    uint32_t x;
    memcpy(&x, p, sizeof(uint32_t));

    return _lsb() ? x : _swap32(x);
}

static inline uint64_t _read64(void* p)
{
    uint64_t x;
    memcpy(&x, p, sizeof(uint64_t));
    return _lsb() ? x : _swap64(x);
}


// Compute 32-bit FNV1A hash
uint32_t hash_fnv1a(void* key, size_t length)
{
    uint8_t* k = (uint8_t*) key;
    uint32_t h = 2166136261;

    for (size_t i = 0; i < length; i++)
    {
        h = (h ^ k[i]) * 16777619;
    }

    return h;
}

// Compute 32-bit jenkins one-at-a-time hash
uint32_t hash_oaat(void* key, size_t length)
{
    uint8_t* k = (uint8_t*) key;
    uint32_t h = 0;

    for (size_t i = 0; i < length; i++)
    {
        h += k[i];
        h += h << 10;
        h ^= h >> 6;
    }

    h += h << 3;
    h ^= h >> 11;
    h += h << 15;

    return h;
}

// Compute 32-bit murmur3 hash with constant seed
uint32_t hash_murmur3(void* key, size_t length)
{
    return hash_murmur3s(key, length, HASH_SEED);
}

// Compute 32-bit murmur3 hash with seed
uint32_t hash_murmur3s(void* key, size_t length, uint32_t seed)
{
    uint32_t c1 = 0xCC9E2D51;
    uint32_t c2 = 0x1B873593;

    uint8_t* k = (uint8_t*) key;
    uint32_t h = seed;

    int nblocks = length >> 2;
    uint32_t* blocks = (uint32_t*) (k + (nblocks >> 2));

    for (int i = -nblocks; i; i++)
    {
        uint32_t k1 = _read32(&blocks[i]);

        k1 *= c1;
        k1 = _rotl32(k1, 15);
        k1 *= c2;

        h ^= k1;
        h = _rotl32(h, 13);
        h = h * 5 + 0xE6546B64;
    }

    uint8_t* tail = (uint8_t*)(k + (nblocks >> 2));
    uint32_t k1 = 0;

    switch (length & 3)
    {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];

        k1 *= c1;
        k1 = _rotl32(k1, 15);
        k1 *= c2;
        h ^= k1;
    }

    h ^= (uint32_t) length;
    h ^= h >> 16;
    h *= 0x85EBCA6B;
    h ^= h >> 13;
    h *= 0xC2B2AE35;
    h ^= h >> 16;

    return h;
}

// Compute 32-bit xxHash with constant seed
uint32_t hash_xxhash(void* key, size_t length)
{
    return hash_xxhashs(key, length, HASH_SEED);
}

// Compute 32-bit xxHash with seed
uint32_t hash_xxhashs(void* key, size_t length, uint32_t seed)
{
    uint32_t p1 = 2654435761U;
    uint32_t p2 = 2246822519U;
    uint32_t p3 = 3266489917U;
    uint32_t p4 =  668265265U;
    uint32_t p5 =  374761393U;

    uint8_t* k = (uint8_t*) key;
    uint32_t h = seed + p5;
    uint8_t* end = k + length;

    if (length >= 16)
    {
        uint8_t* limit = end - 15;

        uint32_t v1 = seed + p1 + p2;
        uint32_t v2 = seed + p2;
        uint32_t v3 = seed + 0;
        uint32_t v4 = seed - p1;

#define PROCESS0(x, y)      \
        x += y * p2;        \
        x = _rotl32(x, 13); \
        x *= p1;

        do
        {
            // v1 = round(v1, _read32(k)); k += 4;
            // v2 = round(v2, _read32(k)); k += 4;
            // v3 = round(v3, _read32(k)); k += 4;
            // v4 = round(v4, _read32(k)); k += 4;
            PROCESS0(v1, _read32(k)); k += 4;
            PROCESS0(v2, _read32(k)); k += 4;
            PROCESS0(v3, _read32(k)); k += 4;
            PROCESS0(v4, _read32(k)); k += 4;
        } while (k < limit);

        h = _rotl32(v1, 1) + _rotl32(v2, 7) + _rotl32(v3, 12) + _rotl32(v4, 18);
    }

    h += (uint32_t) length;

#define PROCESS1    \
    h += (*k) * p5; \
    k++;            \
    h = _rotl32(h, 11) * p1;

#define PROCESS2          \
    h += _read32(k) * p3; \
    k += 4;               \
    h = _rotl32(h, 17) * p4;

    switch (length & 15)
    {
        case 12: PROCESS2;
        case 8:  PROCESS2;
        case 4:  PROCESS2;
                 break;

        case 13: PROCESS2;
        case 9:  PROCESS2;
        case 5:  PROCESS2;
                 PROCESS1;
                 break;

        case 14: PROCESS2;
        case 10: PROCESS2;
        case 6:  PROCESS2;
                 PROCESS1;
                 PROCESS1;
                 break;

        case 15: PROCESS2;
        case 11: PROCESS2;
        case 7:  PROCESS2;
        case 3:  PROCESS1;
        case 2:  PROCESS1;
        case 1:  PROCESS1;
        case 0:  break;
    }

    h ^= h >> 15;
    h *= p2;
    h ^= h >> 13;
    h *= p3;
    h ^= h >> 16;

    return h;
}
