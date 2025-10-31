#include "string_utils.h"

char* string_duplicate(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

char* string_duplicate_n(const char* s, size_t n) {
    if (!s) return NULL;
    
    size_t len = 0;
    while (len < n && s[len] != '\0') {
        len++;
    }

    char* copy = malloc(len + 1);
    if (!copy) return NULL;

    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

int string_case_compare(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}
