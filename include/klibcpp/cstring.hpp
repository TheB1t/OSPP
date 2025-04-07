#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cmath.hpp>

/* String manipulation functions */
void        reverse(char s[]);
void        strcpy(char *dst, char *src);
void        strncpy(char *dst, char *src, uint32_t max);
int         strcmp(char *s1, char *s2);
int         strncmp(char *s1, char *s2, uint32_t len);
void        strcat(char *dst, char *src);
uint32_t    strlen(char str[]);
const char* strrchr(const char* str, int ch);

int         sprintf(char *dst, const char *fmt, ...);
int         snprintf(char *dst, uint32_t max, const char *fmt, ...);

/* Integer functions */
void        utoa(uint32_t n, char str[]);
uint32_t    atou(char str[]);
void        itoa(int32_t n, char str[]);
void        htoa(uint32_t in, char str[]);
void        ftoa(double in, char str[], uint32_t precision);

/* Memory functions */
void        memcpy(uint8_t *dst, uint8_t *src, uint32_t count);
void        memcpy32(uint32_t *dst, uint32_t *src, uint32_t count);
void        memset(uint8_t *dst, uint8_t data, uint32_t count);
void        memset32(uint32_t *dst, uint32_t data, uint32_t count);