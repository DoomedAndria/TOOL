#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "../list/list.h"

char* STR_trim(const char* str) {
    char* empty = malloc(1);
    empty[0] = '\0';
    const size_t len = strlen(str);
    if (len == 0) return empty;
    size_t start = 0;
    int s_set = 0;
    size_t end = len - 1;
    int e_set = 0;
    for (int i = 0; i < len / 2 + 1; i++) {
        if (s_set && e_set) break;
        if (!s_set && !isspace(str[i])) {
            s_set = 1;
            start = i;
        }
        if (!e_set && !isspace(str[len-1-i])) {
            e_set = 1;
            end = len - 1 - i;
        }
    }

    if (!s_set && !e_set) return empty;
    free(empty);

    const size_t new_len = end - start + 1;
    char* result = malloc(new_len * sizeof(char) + 1);
    strncpy(result, str + start, new_len + 1);
    result[new_len] = '\0';
    return result;
}

List* STR_split(const char* str, const char delimiter) {
    List* lst = LIST_create_string();
    const size_t len = strlen(str);
    size_t s_len;
    char* s;

    if (len == 0) return lst;

    int start = 0;
    for (int i = 0; i < len; i++) {
        if (str[i] == delimiter) {
            s_len = i - start;
            s = malloc(s_len * sizeof(char) + 1);
            s[s_len] = '\0';
            strncpy(s, str + start, s_len);
            LIST_append(lst, s);
            start = i + 1;
        }
    }

    s_len = len - start;
    if (s_len != 0) {
        s = malloc(s_len * sizeof(char) + 1);
        s[s_len] = '\0';
        strncpy(s, str + start, s_len);
        LIST_append(lst, s);
    }

    return lst;
}


int STR_starts_with(const char* str, const char* prefix) {
    const size_t prefix_len = strlen(prefix);
    const size_t len = strlen(str);
    if (prefix_len > len) return 0;
    for (int i = 0; i < prefix_len; i++) {
        if (str[i] != prefix[i]) return 0;
    }
    return 1;
}


int STR_ends_with(const char* str, const char* suffix) {
    const size_t suffix_len = strlen(suffix);
    const size_t len = strlen(str);
    if (suffix_len > len) return 0;
    for (int i = 0; i < suffix_len; i++) {
        if (str[len - i - 1] != suffix[suffix_len - i - 1]) return 0;
    }
    return 1;
}

char* STR_to_upper(const char* str) {
    const size_t len = strlen(str);
    char* ret = malloc(len + 1);
    for (int i = 0; i < len; i++) {
        ret[i] = (char) toupper(str[i]);
    }
    ret[len] = '\0';
    return ret;
}

char* STR_to_lower(const char* str) {
    const size_t len = strlen(str);
    char* ret = malloc(len + 1);
    for (int i = 0; i < len; i++) {
        ret[i] = (char) tolower(str[i]);
    }
    ret[len] = '\0';
    return ret;
}


char* STR_concat(const char* a, const char* b) {
    const size_t la = strlen(a);
    const size_t lb = strlen(b);
    char* ret = malloc(la + lb + 1);
    strncpy(ret, a, la);
    strncpy(ret + la, b, lb + 1);
    return ret;
}

char* STR_copy(const char* str) {
    const size_t len = strlen(str);
    char* ret = malloc(len + 1);
    strncpy(ret, str, len + 1);
    return ret;
}

char* STR_replace(char* str, const char* old, const char* new) {
    const size_t len = strlen(str);
    const size_t len_o = strlen(old);
    const size_t len_n = strlen(new);

    size_t len_ret = len;

    //finding length
    char* str1 = strstr(str, old);
    while (str1 != NULL) {
        len_ret = len_ret - len_o + len_n;
        str1 = strstr(str1 + len_o, old);
    }

    char* ret = malloc(len_ret + 1);
    char* ret1 = ret;
    char* start = str;
    char* end = strstr(str, old);

    while (end != NULL) {
        strncpy(ret, start, end - start);
        ret = ret + (end - start);
        strncpy(ret, new, len_n);
        ret = ret + len_n;
        start = end + len_o;
        end = strstr(end + len_o, old);
    }
    strncpy(ret, start, len - (start - str));
    ret1[len_ret] = '\0';
    return ret1;
}


char* STR_join(const List* parts, const char* delimiter) {
    const int len_parts = LIST_size(parts);
    if (len_parts <= 0) {
        char* empty = malloc(1);
        empty[0] = '\0';
        return empty;
    }
    const size_t d_len = strlen(delimiter);

    // finding parts len sum
    size_t len = 0;
    ListIter* iter = LIST_iter(parts);
    while (LIST_iter_has_next(iter)) {
        const char* part = (char *) LIST_iter_next(iter);
        len += strlen(part);
    }
    LIST_iter_free(iter);
    len = len + d_len * (len_parts - 1);

    // concat
    char* ret = malloc(len + 1);
    ret[len] = '\0';
    char* ret1 = ret;
    iter = LIST_iter(parts);
    while (LIST_iter_has_next(iter)) {
        const char* part = (char *) LIST_iter_next(iter);
        const size_t part_len = strlen(part);
        strncpy(ret1, part, part_len);
        ret1 += part_len;
        if (LIST_iter_has_next(iter)) {
            strncpy(ret1, delimiter, d_len);
            ret1 += d_len;
        }
    }
    LIST_iter_free(iter);

    return ret;
}

char* STR_substring(const char* str, int start, int end) {
    const size_t len = strlen(str);
    if (start < 0) start = (int) len + start;
    if (end < 0) end = (int) len + end;
    if (start > end || start < 0 || end < 0 || start >= len || end > len) return NULL;
    char* ret = malloc(end - start + 1);
    strncpy(ret, str + start, end - start);
    ret[end - start] = '\0';
    return ret;
}

int STR_index(const char* str, const char* substr) {
    const char* a = strstr(str, substr);
    if (a == NULL) return -1;
    return (int) (a - str);
}

char* STR_repeat(const char* str,const int n) {
    const size_t len = strlen(str);
    char* ret;
    if (len == 0 || n <= 0){
        ret = malloc(1);
        ret[0] = '\0';
        return ret;
    }
    const size_t len_ret = len * n;
    ret = malloc(len_ret + 1);
    ret[len_ret] = '\0';
    char* ret1 = ret;
    for (int i = 0; i < n; i++) {
        strncpy(ret1, str, len);
        ret1 += len;
    }
    return ret;
}