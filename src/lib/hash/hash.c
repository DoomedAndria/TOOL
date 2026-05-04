#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

typedef struct HashTable {
    List** buckets;
    int bucket_count;
    int entry_count;
    HashFunc hash_func;
    CompareFunc compare;
    FreeFunc free_key;
    FreeFunc free_value;
    PrintFunc print_key;
    PrintFunc print_value;
} HashTable;

HashEntry* HASH_ENTRY_create(const void* key, void* value) {
    HashEntry* hash_entry = malloc(sizeof(HashEntry));
    hash_entry->key = (void *) key;
    hash_entry->value = value;
    return hash_entry;
}

// Dummy print function for lists (we handle printing separately)
void dummy_print(const void* data) {
    // Do nothing - HASH_print handles printing
}

// Compare function for hash entries (compares keys)
int compare_hash_entries(const void* a, const void* b) {
    const HashEntry* entry_a = (HashEntry *) a;
    const HashEntry* entry_b = (HashEntry *) b;
    return strcmp(entry_a->key, entry_b->key);
}

HashTable* HASH_create(const int bucket_count,
                       const HashFunc hash_func,
                       const CompareFunc cmp,
                       const FreeFunc free_key,
                       const FreeFunc free_value,
                       const PrintFunc print_key,
                       const PrintFunc print_value) {
    HashTable* hashtable = malloc(sizeof(HashTable));
    List** buckets = malloc(bucket_count * sizeof(List *));

    // Initialize each bucket as an empty list
    for (int i = 0; i < bucket_count; i++) {
        buckets[i] = LIST_create(compare_hash_entries, free, dummy_print);
    }

    hashtable->buckets = buckets;
    hashtable->bucket_count = bucket_count;
    hashtable->entry_count = 0;
    hashtable->hash_func = hash_func;
    hashtable->compare = cmp;
    hashtable->free_key = free_key;
    hashtable->free_value = free_value;
    hashtable->print_key = print_key;
    hashtable->print_value = print_value;

    return hashtable;
}

void HASH_insert(HashTable* ht, const void* key, void* value) {

    const int index = ht->hash_func(key, ht->bucket_count);
    List* ls = ht->buckets[index];

    const HashEntry temp = {(void *) key, NULL};
    const int l_i = LIST_find(ls, &temp);

    if (l_i >= 0) {
        HashEntry* entry = LIST_get(ls, l_i).data;
        if (ht->free_value) ht->free_value(entry->value);
        entry->value = value;
        return;
    }
    HashEntry* hash_entry = HASH_ENTRY_create(key, value);
    LIST_append(ls, hash_entry);
    ht->entry_count++;
    if (HASH_load_factor_exceeded(ht) == 1)
        HASH_resize(ht);
}

void* HASH_get(const HashTable* ht, const void* key) {
    const int index = ht->hash_func(key, ht->bucket_count);
    const List* ls = ht->buckets[index];
    const HashEntry temp = {(void *) key, NULL};
    const int l_i = LIST_find(ls, &temp);

    if (l_i >= 0) {
        const ListValue res = LIST_get(ls, l_i);
        const HashEntry* entry = (HashEntry *) res.data;
        return entry->value;
    }
    return NULL;
}


int HASH_delete(HashTable* ht, const void* key) {
    const int index = ht->hash_func(key, ht->bucket_count);
    List* ls = ht->buckets[index];
    const HashEntry temp = {(void *) key, NULL};
    const int l_i = LIST_find(ls, &temp);

    if (l_i >= 0) {
        const HashEntry* entry = (HashEntry *) LIST_get(ls, l_i).data;
        if (ht->free_key) ht->free_key(entry->key);
        if (ht->free_value) ht->free_value(entry->value);
        LIST_remove(ls, l_i);
        ht->entry_count--;
        return 1;
    }
    return 0;
}


int HASH_contains(HashTable* ht, const void* key) {
    const int index = ht->hash_func(key, ht->bucket_count);
    const List* ls = ht->buckets[index];
    const HashEntry temp = {(void *) key, NULL};
    const int l_i = LIST_find(ls, &temp);
    if (l_i >= 0) return 1;
    return 0;
}

void HASH_free(HashTable* ht) {
    List** buckets = ht->buckets;
    for (int i = 0; i < ht->bucket_count; i++) {
        List* ls = buckets[i];
        while (LIST_size(ls) > 0) {
            const HashEntry* entry = LIST_get(ls, 0).data;
            if (ht->free_key) ht->free_key(entry->key);
            if (ht->free_value) ht->free_value(entry->value);
            LIST_shift(ls);
        }
        LIST_free(ls);
    }
    free(buckets);
    free(ht);
}

int HASH_size(const HashTable* ht) {
    return ht->entry_count;
}

void HASH_print(const HashTable* ht) {
    List** buckets = ht->buckets;
    printf("{");
    int printed = 0;
    for (int i = 0; i < ht->bucket_count; i++) {
        const List* ls = buckets[i];
        for (int j = 0; j < LIST_size(ls); j++) {
            if (printed > 0) printf(", ");
            const HashEntry* entry = LIST_get(ls, j).data;
            printf("(");
            ht->print_key(entry->key);
            printf(" : ");
            ht->print_value(entry->value);
            printf(")");
            printed++;
        }
    }
    printf("}");
}

int HASH_load_factor_exceeded(const HashTable* ht) {
    return (float) ht->entry_count / (float) ht->bucket_count > 0.75;
}

void HASH_resize(HashTable* ht) {
    const int new_bucket_count = ht->bucket_count * 2;
    List** new_buckets = malloc(new_bucket_count * sizeof(List *));
    for (int i = 0; i < new_bucket_count; i++) {
        new_buckets[i] = LIST_create(compare_hash_entries, free, dummy_print);
    }

    List** old_buckets = ht->buckets;
    const int old_bucket_count = ht->bucket_count;


    ht->bucket_count = new_bucket_count;
    ht->buckets = new_buckets;
    ht->entry_count = 0;


    for (int i = 0; i < old_bucket_count; i++) {
        List* ls = old_buckets[i];
        while (LIST_size(ls) > 0) {
            const HashEntry* entry = LIST_get(ls, 0).data;

            HASH_insert(ht, entry->key, entry->value);

            LIST_shift(ls);
        }
        LIST_free(ls);
    }
    free(old_buckets);
}


List* HASH_keys(const HashTable* ht) {
    List* ret = LIST_create(ht->compare, NULL, ht->print_key);
    List** buckets = ht->buckets;
    for (int i = 0; i < ht->bucket_count; i++) {
        const List* ls = buckets[i];
        ListIter* iter = LIST_iter(ls);
        while (LIST_iter_has_next(iter)) {
            const HashEntry* entry = LIST_iter_next(iter);
            LIST_append(ret, entry->key);
        }
        LIST_iter_free(iter);
    }
    return ret;
}

List* HASH_values(const HashTable* ht) {
    List* ret = LIST_create(NULL, NULL, ht->print_value);
    List** buckets = ht->buckets;
    for (int i = 0; i < ht->bucket_count; i++) {
        const List* ls = buckets[i];
        ListIter* iter = LIST_iter(ls);
        while (LIST_iter_has_next(iter)) {
            const HashEntry* entry = LIST_iter_next(iter);
            LIST_append(ret, entry->value);
        }
        LIST_iter_free(iter);
    }
    return ret;
}
