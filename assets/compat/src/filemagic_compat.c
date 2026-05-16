#include "filemagic_compat.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef __APPLE__
size_t
strlcpy(char *dst, const char *src, size_t dsize)
{
    size_t srclen;
    size_t copylen;

    srclen = strlen(src);
    if (dsize == 0)
        return srclen;

    copylen = srclen;
    if (copylen >= dsize)
        copylen = dsize - 1;

    memcpy(dst, src, copylen);
    dst[copylen] = '\0';
    return srclen;
}

size_t
strlcat(char *dst, const char *src, size_t dsize)
{
    size_t dstlen;
    size_t srclen;

    dstlen = strnlen(dst, dsize);
    srclen = strlen(src);
    if (dstlen == dsize)
        return dsize + srclen;

    if (srclen < dsize - dstlen) {
        memcpy(dst + dstlen, src, srclen + 1);
    } else {
        memcpy(dst + dstlen, src, dsize - dstlen - 1);
        dst[dsize - 1] = '\0';
    }
    return dstlen + srclen;
}

void *
reallocarray(void *ptr, size_t nmemb, size_t size)
{
    if (size != 0 && nmemb > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc(ptr, nmemb * size);
}

int
vasprintf(char **strp, const char *fmt, va_list ap)
{
    int needed;
    va_list ap_copy;
    char *buf;

    va_copy(ap_copy, ap);
    needed = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    if (needed < 0)
        return -1;

    buf = malloc((size_t)needed + 1);
    if (buf == NULL)
        return -1;

    if (vsnprintf(buf, (size_t)needed + 1, fmt, ap) < 0) {
        free(buf);
        return -1;
    }

    *strp = buf;
    return needed;
}

int
asprintf(char **strp, const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start(ap, fmt);
    rc = vasprintf(strp, fmt, ap);
    va_end(ap);
    return rc;
}

ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
    int ch;
    size_t len;
    char *next;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (*lineptr == NULL)
            return -1;
    }

    len = 0;
    while ((ch = fgetc(stream)) != EOF) {
        if (len + 1 >= *n) {
            size_t next_size;

            next_size = *n * 2;
            next = realloc(*lineptr, next_size);
            if (next == NULL)
                return -1;
            *lineptr = next;
            *n = next_size;
        }
        (*lineptr)[len++] = (char)ch;
        if (ch == '\n')
            break;
    }

    if (ferror(stream))
        return -1;
    if (len == 0 && ch == EOF)
        return -1;

    (*lineptr)[len] = '\0';
    return (ssize_t)len;
}
#endif

#ifdef _WIN32
char *
ctime_r(const time_t *timer, char *buf)
{
    if (buf == NULL || timer == NULL)
        return NULL;
    if (ctime_s(buf, 26, timer) != 0)
        return NULL;
    return buf;
}

char *
asctime_r(const struct tm *tm, char *buf)
{
    if (buf == NULL || tm == NULL)
        return NULL;
    if (asctime_s(buf, 26, tm) != 0)
        return NULL;
    return buf;
}
#endif

static void
filemagic_vwarn_impl(int with_errno, const char *fmt, va_list ap)
{
    int saved_errno;

    saved_errno = errno;
    if (fmt != NULL && fmt[0] != '\0') {
        vfprintf(stderr, fmt, ap);
        if (with_errno)
            fprintf(stderr, ": %s", strerror(saved_errno));
    } else if (with_errno) {
        fprintf(stderr, "%s", strerror(saved_errno));
    }
    fputc('\n', stderr);
}

void
vwarn(const char *fmt, va_list ap)
{
    filemagic_vwarn_impl(1, fmt, ap);
}

void
warn(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vwarn(fmt, ap);
    va_end(ap);
}

void
vwarnx(const char *fmt, va_list ap)
{
    filemagic_vwarn_impl(0, fmt, ap);
}

void
warnx(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vwarnx(fmt, ap);
    va_end(ap);
}

void
verr(int eval, const char *fmt, va_list ap)
{
    vwarn(fmt, ap);
    exit(eval);
}

void
err(int eval, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    verr(eval, fmt, ap);
    va_end(ap);
}

void
verrx(int eval, const char *fmt, va_list ap)
{
    vwarnx(fmt, ap);
    exit(eval);
}

void
errx(int eval, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    verrx(eval, fmt, ap);
    va_end(ap);
}
