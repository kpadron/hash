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
#include "hash-table.h"

double wtime(void);
uint32_t rand32(void);
uint64_t rand64(void);

typedef struct
{
    char* key;
    void* data;
    uint8_t flag;
} pair_t;

typedef struct
{
    uint64_t count;
    uint64_t size;
    pair_t* array;
} pairlist_t;

void pairlist_insert(pairlist_t* list, char* key, void* data)
{
    if (!list) return;

    if (list->count == list->size || !list->array)
    {
        list->size <<= 1;
        list->array = (pair_t*) realloc(list->array, list->size * sizeof(pair_t));
    }

    list->array[list->count].key = key;
    list->array[list->count].data = data;
    list->array[list->count++].flag = 0;
}

static inline size_t keysize(const void* key)
{
    return strlen((const char*) key);
}

static inline int keycmp(const void* a, const void* b)
{
    return strcmp((const char*) a, (const char*) b);
}

int main(int argc, char** argv)
{
    char* tests[] = { "hash_insert", "hash_search", "hash_remove" };

    double test_duration = 5;

    if (argc > 1)
    {
        test_duration = atof(argv[1]);
    }

    hash_t table;

    pairlist_t list;
    list.count = 0;
    list.size = 10000;
    list.array = NULL;

    hash_init(&table, 10, keysize, keycmp, hash_xxhash, NULL);

    FILE* dict = fopen("words.txt", "r");

    for (size_t i = 0; i < 3; i++)
    {
        double test_start = 0;
        double test_time = 0;
        size_t test_cycles = 0;

        do
        {
            if (!strcmp(tests[i], "hash_insert"))
            {
                char str[256];
                if (!fgets(str, 255, dict)) break;
                str[strlen(str) - 1] = '\0';

                if (!strcmp(str, "Z"))
                {
                    break;
                }

                void* d = strdup(str);
                char* k = d;

                pairlist_insert(&list, k, d);

                test_start = wtime();
                hash_insert(&table, d, d);
                test_time += wtime() - test_start;
            }
            else if (!strcmp(tests[i], "hash_search"))
            {
                uint64_t index = rand64()%list.count;
                char* k = list.array[index].key;
                void* d = list.array[index].data;

                test_start = wtime();
                void* hd = hash_search(&table, k);
                test_time += wtime() - test_start;

                if (hd != d)
                {
                    assert(hd == d);
                }
            }
            else if (!strcmp(tests[i], "hash_remove"))
            {
                uint64_t index = rand64()%list.count;
                char* k = list.array[index].key;
                void* d = list.array[index].data;
                uint8_t f = list.array[index].flag;

                test_start = wtime();
                void* hd = hash_remove(&table, k);
                test_time += wtime() - test_start;

                if (!f)
                {
                    list.array[index].flag = 1;
                    assert(hd == d);
                }
                else
                {
                    assert(hd == NULL);
                }
            }

            test_cycles++;

        } while (test_time < test_duration);

        printf("%s: %zu iterations over %.2f s -> %.4f ns per operation\n", tests[i], test_cycles, test_time, test_time * 1E9 / test_cycles);
        hash_print_stats(&table);
        printf("\n");
    }

    hash_free(&table, NULL, NULL);

    return 0;
}


// walltime of the computer in seconds (useful for performance analysis)
double wtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1E9;
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
