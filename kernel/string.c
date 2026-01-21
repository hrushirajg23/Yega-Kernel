
#include "string.h"
#include <stdint.h>
#include <stddef.h>

void *memmove(void *dest, const void *src, size_t n) {
    // check for overlap.
    // if we copy from start to end, 
    // the "bad" case is when dst < (src + n). in that case, 
    // we overwrite the end of src with the front of src, 
    // before we can copy the end of src.
    // if src < (dst + n), it's fine, since we'll never lose data
    uint8_t *dest_p = (uint8_t *) dest;
    uint8_t *src_p = (uint8_t *) src;

    uint32_t di = (uint32_t) dest;
    uint32_t si = (uint32_t) src;
    if (di < si + n) {
        // copy in reverse. 
        for (size_t i = n; i > 0; i--) {
            dest_p[i - 1] = src_p[i - 1];
        }
    } else {
        // copy normally.
        for (size_t i = 0; i < n; i++) {
            dest_p[i] = src_p[i];
        }
    }
    return ((void *) dest_p);
}

void *memchr(const void *s, int c, size_t n) {
    uint8_t *sp = (uint8_t *) s;
    uint8_t ch = (uint8_t) c;
    for (size_t i = 0; i < n; i++) {
        if (sp[i] == ch) {
            return (sp+i);
        }
    }
}

int memcmp(const void *s1, const void *s2, size_t n) {
    uint8_t *p1 = (uint8_t *) s1;
    uint8_t *p2 = (uint8_t *) p2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]);
        }
    }
    return 0;
}

void *memset(void *dest, int c, size_t n) {
    uint8_t *dest_p = (uint8_t *) dest;
    uint8_t ci = (uint8_t) c;
    for (size_t i = 0; i < n; i++) {
        dest_p[i] = ci;
    }
}


int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n-- && *s1 && (*s1 == *s2)) {
        if (n == 0)
            return 0;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *s, const char *ct) {
    char *sn = s;
    while (*ct != '\0') {
        *sn++ = *ct++;
    }
    *sn = '\0';
    return s;
}

size_t strlen(const char *s) {
    size_t i = 0;
    for (; s[i] != '\0'; i++);
    return i;
}

int char_in_set(char c, const char *set) {
    while (*set != '\0') {
        if (c == *set++) return 1;
    }
    return 0;
}

char *strtok(char *str, const char *delim) {
    static char *buf;
    static int chars_left;

    if (str != NULL) {
        chars_left = strlen(str);
        buf = str;
    }

    if (chars_left == 0) return NULL;

    char *tok_start = buf;

    while (!char_in_set(*buf, delim)) {
        // hit end of string. return.
        if (*buf == '\0') {
            return tok_start;
        }
        buf++; chars_left--;
    }
    // found a match. 
    *buf++ = '\0'; chars_left--;
    return tok_start;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    /* Pad with nulls if src is shorter than n */
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    
    return dest;
}


