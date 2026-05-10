#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

static int compare_string(const void* a, const void* b) {
    return strcmp(a, b);
}

static void print_string(const void *data) {
    printf("%s", (char*)data);
}

static int hash_str_fnv1a(const void* key, const int bucket_count) {
    const unsigned char* s = key;
    unsigned long h = 2166136261u;
    while (*s) {
        h ^= *s++;
        h *= 16777619u;
    }
    return (int)(h % bucket_count);
}
HashTable* HASH_create_str(const FreeFunc free_value, const PrintFunc print_value) {
    HashTable* ht = HASH_create(64, hash_str_fnv1a, compare_string, free,free_value,print_string,print_value);
    return ht;
}