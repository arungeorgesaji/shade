#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>  
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char* string_duplicate(const char* s);
char* string_duplicate_n(const char* s, size_t n);
int string_case_compare(const char* s1, const char* s2);

#endif
