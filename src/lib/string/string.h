#ifndef STR_H
#define STR_H

char* STR_trim(const char* str);

List* STR_split(const char* str, char delimiter);

int STR_starts_with(const char* str, const char* prefix);

int STR_ends_with(const char* str, const char* suffix);

char* STR_to_upper(const char* str);

char* STR_to_lower(const char* str);

char* STR_concat(const char* a, const char* b);

char* STR_copy(const char* str);

char* STR_replace(char* str, const char* old, const char* new);

char* STR_join(const List* parts, const char* delimiter);

char* STR_substring(const char* str, int start, int end);

int STR_index(const char* str, const char* substr);

char* STR_repeat(const char* str,int n);
#endif
