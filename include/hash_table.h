#ifndef HASH_TABLE_C
#define HASH_TABLE_C

#include <stdbool.h>
#include <stddef.h>

#include "types.h"

typedef struct _HTEntry {
    char *key;
    void *val;
    size_t val_size;
    struct _HTEntry *nxt;
} HTEntry;

typedef struct {
    HTEntry **buckets;
    size_t size;
} HT;

HT* ht_create(size_t size);

void ht_put(HT *ht, const char *key, const void *val, size_t val_size);
const void* ht_get(HT *ht, const char *key);
bool ht_contains(HT *ht, const char *key);

void ht_free(HT *ht);

u64 hash_str(const char *str);

#endif