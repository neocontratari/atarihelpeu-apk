/* B316 test: nahrada util.h (Atari800) */
#ifndef UTIL_H_
#define UTIL_H_
#include <stdio.h>
#include <stddef.h>
void *Util_malloc(size_t size);
int Util_flen(FILE *fp);
#define Util_rewind(fp) fseek(fp, 0, SEEK_SET)
size_t Util_strlcpy(char *dest, const char *src, size_t size);
int Util_sscandec(const char *s);
int Util_sscansdec(const char *s, int *result);
int Util_sscanbool(const char *s);
#endif
