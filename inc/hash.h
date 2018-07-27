// hash.h
// kpadron.github@gmail.com
// Kristian Padron
// hash function module
#pragma once
#include <stdlib.h>
#include <stdint.h>

// Compute 32-bit FNV1A hash
extern uint32_t hash_fnv1a(void* key, size_t length);

// Compute 32-bit jenkins one-at-a-time hash
extern uint32_t hash_oaat(void* key, size_t length);

// Compute 32-bit murmur3 hash with constant seed
extern uint32_t hash_murmur3(void* key, size_t length);

// Compute 32-bit murmur3 hash with seed
extern uint32_t hash_murmur3s(void* key, size_t length, uint32_t seed);

// Compute 32-bit xxHash with constant seed
extern uint32_t hash_xxhash(void* key, size_t length);

// Compute 32-bit xxHash with seed
extern uint32_t hash_xxhashs(void* key, size_t length, uint32_t seed);
