#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

HT* ht_create(size_t size) {
    HT *ht = malloc(sizeof(HT));

    ht->size = size;
    ht->buckets = calloc(size, sizeof(HTEntry*));
    
    return ht;
}

void ht_put(HT *ht, const char *key, const void *val, size_t val_size) {
    u64 h = hash_str(key);
    size_t idx = h % ht->size;

    HTEntry *e = ht->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            free(e->val);

            void *new_data = malloc(val_size);

            if (new_data) {
                e->val = new_data;
                e->val_size = val_size;
                memcpy(e->val, val, val_size);
            }


            return;
        }
        e = e->nxt;
    }

    HTEntry *new = malloc(sizeof(HTEntry));
    new->key = strdup(key);
    new->val = malloc(val_size);
    new->val_size = val_size;
    memcpy(new->val, val, val_size);

    new->nxt = ht->buckets[idx];
    ht->buckets[idx] = new;
}

const void* ht_get(HT *ht, const char *key) {
    u64 h = hash_str(key);
    size_t idx = h % ht->size;

    HTEntry *e = ht->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) return e->val;
        e = e->nxt;
    }

    return NULL;
}

bool ht_contains(HT *ht, const char *key) {
    u64 h = hash_str(key);
    size_t idx = h % ht->size;

    HTEntry *e = ht->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) return true;
        e = e->nxt;
    }

    return false;
}

void ht_free(HT *ht) {
    for (size_t i = 0; i < ht->size; i++) {
        HTEntry *e = ht->buckets[i];
        while (e) {
            HTEntry *cur = e;
            e = cur->nxt;

            free(cur->key);
            free(cur->val);
            free(cur);
        }
    }
    free(ht->buckets);
    free(ht);
}

u64 hash_str(const char *str) {
    u64 hash = 5381;
    int c;

    while ((c = *str++)) hash = ((hash << 5) + hash) + c;

    return hash;
}