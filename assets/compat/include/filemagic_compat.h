#ifndef COMMONSBASE_FILEMAGIC_COMPAT_H
#define COMMONSBASE_FILEMAGIC_COMPAT_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _WIN32
typedef unsigned char u_char;
typedef unsigned int u_int;
#ifndef _SSIZE_T_DEFINED
typedef ptrdiff_t ssize_t;
#endif

static inline uint16_t
be16toh(uint16_t value)
{
    return __builtin_bswap16(value);
}

static inline uint32_t
be32toh(uint32_t value)
{
    return __builtin_bswap32(value);
}

static inline uint64_t
be64toh(uint64_t value)
{
    return __builtin_bswap64(value);
}

static inline uint16_t
le16toh(uint16_t value)
{
    return value;
}

static inline uint32_t
le32toh(uint32_t value)
{
    return value;
}

static inline uint64_t
le64toh(uint64_t value)
{
    return value;
}
#endif

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

size_t strlcpy(char *dst, const char *src, size_t dsize);
size_t strlcat(char *dst, const char *src, size_t dsize);
void *reallocarray(void *ptr, size_t nmemb, size_t size);
int vasprintf(char **strp, const char *fmt, va_list ap);
int asprintf(char **strp, const char *fmt, ...);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
char *ctime_r(const time_t *timer, char *buf);
char *asctime_r(const struct tm *tm, char *buf);

#endif
