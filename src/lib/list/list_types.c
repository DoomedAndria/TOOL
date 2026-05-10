#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"


static int compare_string(const void* a, const void* b) {
    return strcmp(a, b);
}

static void print_string(const void *data) {
    printf("%s", (char*)data);
}

List *LIST_create_string() {
    return LIST_create(compare_string,free,print_string);;
}



static int compare_int(const void* a, const void* b) {
    const int aa = *((int*)a);
    const int bb = *((int*)b);
    return aa - bb;
}

static void print_int(const void *data) {
    printf("%d", *((int*)data));
}

List* LIST_create_int() {
    return LIST_create(compare_int,free,print_int);
}