#ifndef _STRING_H
#define _STRING_H

#include "kernel.h"

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strtok(char* str, const char* delim) {
    static char* last;
    if (str) {
        last = str;
    }
    if (!last) {
        return (char*)NULL;
    }
    char* token = last;
    while (*last) {
        for (const char* d = delim; *d; d++) {
            if (*last == *d) {
                *last = '\0';
                last++;
                return token;
            }
        }
        last++;
    }
    last = (char*)NULL;
    return token;
}  

#endif // _STRING_H