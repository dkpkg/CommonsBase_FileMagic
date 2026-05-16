#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filemagic_compat.h"
#include "xmalloc.h"

static __dead void
filemagic_xalloc_fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void *
xmalloc(size_t size)
{
    void *ptr;

    if (size == 0)
        filemagic_xalloc_fail("xmalloc: zero size");
    ptr = malloc(size);
    if (ptr == NULL)
        filemagic_xalloc_fail("xmalloc: allocation failed");
    return ptr;
}

void *
xcalloc(size_t nmemb, size_t size)
{
    void *ptr;

    if (nmemb == 0 || size == 0)
        filemagic_xalloc_fail("xcalloc: zero size");
    ptr = calloc(nmemb, size);
    if (ptr == NULL)
        filemagic_xalloc_fail("xcalloc: allocation failed");
    return ptr;
}

void *
xreallocarray(void *ptr, size_t nmemb, size_t size)
{
    void *next;

    next = reallocarray(ptr, nmemb, size);
    if (next == NULL)
        filemagic_xalloc_fail("xreallocarray: allocation failed");
    return next;
}

char *
xstrdup(const char *str)
{
    char *copy;

    copy = strdup(str);
    if (copy == NULL)
        filemagic_xalloc_fail("xstrdup: allocation failed");
    return copy;
}

int
xasprintf(char **ret, const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start(ap, fmt);
    rc = vasprintf(ret, fmt, ap);
    va_end(ap);
    if (rc < 0)
        filemagic_xalloc_fail("xasprintf: allocation failed");
    return rc;
}
