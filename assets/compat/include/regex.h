#ifndef COMMONSBASE_FILEMAGIC_REGEX_H
#define COMMONSBASE_FILEMAGIC_REGEX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef ptrdiff_t regoff_t;

typedef struct {
    regoff_t rm_so;
    regoff_t rm_eo;
} regmatch_t;

typedef struct {
    void *re_impl;
    size_t re_nsub;
    int re_cflags;
} regex_t;

#define REG_NOERROR 0
#define REG_NOMATCH 1
#define REG_BADPAT 2
#define REG_ESPACE 12

#define REG_EXTENDED 0x01
#define REG_ICASE 0x02
#define REG_NOSUB 0x04
#define REG_NEWLINE 0x08

#define REG_NOTBOL 0x01
#define REG_NOTEOL 0x02
#define REG_STARTEND 0x04

int regcomp(regex_t *preg, const char *pattern, int cflags);
int regexec(const regex_t *preg, const char *string, size_t nmatch,
    regmatch_t pmatch[], int eflags);
size_t regerror(int errcode, const regex_t *preg, char *errbuf,
    size_t errbuf_size);
void regfree(regex_t *preg);

#ifdef __cplusplus
}
#endif

#endif
