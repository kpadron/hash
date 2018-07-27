// hash_test.c
// kpadron.github@gmail.com
// Kristian Padron
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

double wtime(void);
void* recalloc(void* p, size_t old_size, size_t new_size);
double hr_bytes(size_t bytes, char* symbol);
double hr_seconds(double seconds, char* symbol);
uint32_t rand32(void);
uint64_t rand64(void);

typedef struct node
{
    void* key;
    struct node* next;
} node_t;

typedef struct
{
    uint32_t hash;
    uint64_t count;
    node_t* list;
} pair_t;

typedef struct
{
    uint64_t count;
    uint64_t size;
    pair_t* array;
} pairlist_t;

// Return index to the position of entry with specified key O(log N)
uint64_t pairlist_bsearch(pairlist_t* list, uint32_t hash)
{
    pair_t* sorted = list->array;

    uint64_t l = 0;
    uint64_t u = list->count;

    // Perform a binary search by comparing to middle element
    while (l < u)
    {
        // Update pivot index to be middle of the range
        uint64_t p = l + ((u - l) >> 1);

        // Use lower half as new range
        if (hash < sorted[p].hash) u = p;
        // Use upper half as new range
        else if (hash > sorted[p].hash) l = p + 1;
        // Return matching pivot
        else return p;
    }

    return l;
}

// Insert new entry into bucket in sorted order O(N)
void pairlist_binsert(pairlist_t* list, uint32_t hash, void* key)
{
    // Expand bucket memory if necessary
    if (list->count == list->size || !list->array)
    {
        list->size <<= 1;
        list->array = (pair_t*) realloc(list->array, list->size * sizeof(pair_t));
    }

    // Determine position to insert at O(log N)
    uint64_t index = pairlist_bsearch(list, hash);
    pair_t* array = list->array;

    // Add new node to list for hash
    // node_t* node = (node_t*) malloc(sizeof(node_t));
    // node->key = key;
    // node->next = array[index].list;

    // Update existing entry (duplicates not allowed!)
    if (index < list->count && hash == array[index].hash)
    {
        // array[index].list = node;
        array[index].count++;
    }
    // Insert and shift entries into place O(N)
    else if (index <= list->count)
    {
        memmove(&array[index + 1], &array[index], (list->count++ - index) * sizeof(pair_t));
        array[index].hash = hash;
        array[index].count = 1;
        // array[index].list = node;
    }
}

void pairlist_stats(pairlist_t* list)
{
    if (!list) return;

    uint64_t collisions = 0;

    for (uint64_t i = 0; i < list->count; i++)
    {
        collisions += list->array[i].count - 1;
    }

    printf("collisions: %zu\n", (size_t) collisions);
}

static inline size_t _filesize(char* path)
{
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fclose(f);
    return size;
}


int main(int argc, char** argv)
{
    char* tests[] = { "hash_fnv1a", "hash_oaat", "hash_murmur3", "hash_xxhash" };

    pairlist_t list;
    list.count = 0;
    list.size = 1024;
    list.array = NULL;

    FILE* dict = fopen("words.txt", "r");
    if (!dict)
    {
        perror("fopen");
        exit(1);
    }

    // Fill dictionary with words
    char** words = (char**) malloc(466544 * sizeof(char*));
    for (size_t i = 0; i < 466544; i++)
    {
        char str[256];
        fgets(str, 255, dict);
        str[strlen(str) - 1] = '\0';
        words[i] = strdup(str);
    }

    // Hash files from command line
    for (int c = 1; c < argc; c++)
    {
        for (size_t i = 0; i < 4; i++)
        {
            double test_start = 0;
            double test_time = 0;
            size_t bytes = _filesize(argv[c]);

            uint32_t (*hash)(void* key, size_t length);

            if (!strcmp(tests[i], "hash_fnv1a"))
                hash = hash_fnv1a;
            else if (!strcmp(tests[i], "hash_oaat"))
                hash = hash_oaat;
            else if (!strcmp(tests[i], "hash_murmur3"))
                hash = hash_murmur3;
            else if (!strcmp(tests[i], "hash_xxhash"))
                hash = hash_xxhash;
            else
                hash = NULL;


            test_start = wtime();
            uint32_t h = hash_file(argv[c], hash);
            test_time += wtime() - test_start;

            char symbol[8];
            printf("%s [%s]: 0x%zx\n", argv[c], tests[i], (size_t) h);
            printf("%zu bytes over %.2f %ss -> %.1f %sB/s\n", bytes, hr_seconds(test_time, symbol), symbol, hr_bytes(bytes / test_time, symbol + 2), symbol + 2);
            printf("\n");
        }
    }

    if (argc < 2)
    {
        for (size_t i = 0; i < 4; i++)
        {
            double test_duration = 5;
            double test_start = 0;
            double test_time = 0;
            size_t test_cycles = 0;
            size_t j = 0;
            size_t bytes = 0;

            uint32_t (*hash)(void* key, size_t length);

            if (!strcmp(tests[i], "hash_fnv1a"))
                hash = hash_fnv1a;
            else if (!strcmp(tests[i], "hash_oaat"))
                hash = hash_oaat;
            else if (!strcmp(tests[i], "hash_murmur3"))
                hash = hash_murmur3;
            else if (!strcmp(tests[i], "hash_xxhash"))
                hash = hash_xxhash;
            else
                hash = NULL;

            do
            {
                // Generate hash
                test_start = wtime();
                uint32_t h = hash(words[j], strlen(words[j]));
                test_time += wtime() - test_start;

                bytes += strlen(words[j]);

                // Add hash and key to list
                pairlist_binsert(&list, h, words[j++]);

                // Ran out of words
                if (j == 466544) break;

                test_cycles++;

            } while (test_time < test_duration);

            char symbol[8];
            printf("%s: %zu iterations over %.2f %ss -> %.4f ns per operation\n", tests[i], test_cycles, hr_seconds(test_time, symbol), symbol, test_time * 1E9 / test_cycles);
            printf("%zu bytes over %.2f s -> %.1f %sB/s\n", bytes, test_time, hr_bytes(bytes / test_time, symbol), symbol);
            pairlist_stats(&list);
            printf("\n");

            free(list.array);
            list.array = NULL;
            list.count = 0;
        }
    }

    return 0;
}


// walltime of the computer in seconds (useful for performance analysis)
double wtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1E9;
}

// realloc except new memory area is zeroed
void* recalloc(void* p, size_t old_size, size_t new_size)
{
    p = realloc(p, new_size);

    if (new_size > old_size)
    {
        memset((char*)p + old_size, 0, new_size - old_size);
    }

    return p;
}

// Print human readable form of bytes based on powers of 2
double hr_bytes(size_t bytes, char* symbol)
{
    char symbols[7][3] = { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei" };
    unsigned char i = 0;
    size_t unit = 1 << 10;

    for (; i < 7 && bytes >= unit; i++, unit <<= 10);
    strcpy(symbol, symbols[i]);

    return (double) bytes / (unit >> 10);
}

// Print human readable form of seconds based on powers of 10
double hr_seconds(double seconds, char* symbol)
{
    char symbols[7][2] = { "", "m", "u", "n", "p", "f", "a" };
    unsigned char i = 0;
    double unit = 1;

    for (; i < 7 && seconds * unit < 1; i++, unit *= 1000);
    strcpy(symbol, symbols[i]);

    return seconds * unit;
}

// Return 32-bit random number
uint32_t rand32(void)
{
    return (rand() ^ (rand() << 15) ^ (rand() << 30));
}


// Return 64-bit random number
uint64_t rand64(void)
{
    return (((uint64_t) rand32()) << 32) | rand32();
}
