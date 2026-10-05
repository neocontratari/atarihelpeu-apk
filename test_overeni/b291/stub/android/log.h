// Nahrada <android/log.h> pro test na pocitaci (ne pro build appky).
#pragma once
#include <cstdio>
#include <cstdarg>
enum { ANDROID_LOG_INFO = 4 };
static inline int __android_log_print(int, const char *, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); fputc('\n', stderr); return 0;
}
