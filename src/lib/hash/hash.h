#ifndef HASH_H
#define HASH_H                                                                                                                                   

#include "../list/list.h"

#define INITIAL_BUCKET_COUNT 16

typedef int (*HashFunc)(const void* key, int bucket_count);

typedef struct {
    void* key;
    void* value;
} HashEntry;

typedef struct HashTable HashTable;

HashTable* HASH_create(int bucket_count, HashFunc hash_func, CompareFunc cmp, FreeFunc free_key, FreeFunc free_value,
                       PrintFunc print_key, PrintFunc print_value);

void HASH_free(HashTable* ht);

void HASH_insert(HashTable* ht, const void* key, void* value);

void* HASH_get(const HashTable* ht, const void* key);

int HASH_delete(HashTable* ht, const void* key);

int HASH_contains(HashTable* ht, const void* key);

void HASH_print(const HashTable* ht);

int HASH_size(const HashTable* ht);

int HASH_load_factor_exceeded(const HashTable* ht);

void HASH_resize(HashTable* ht);
//ht is owner of list values
List* HASH_keys(const HashTable* ht);
// returns list without comparator and free function ht is owner of list values
List* HASH_values(const HashTable* ht);
#endif
