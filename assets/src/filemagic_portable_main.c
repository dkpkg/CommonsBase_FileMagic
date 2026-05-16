#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#else
#include <io.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "file.h"
#include "filemagic_embedded_magic.h"
#include "filemagic_compat.h"
#include "magic.h"
#include "xmalloc.h"

struct input_msg {
    struct stat sb;
    int error;
};

struct input_file {
    struct magic *m;
    struct input_msg *msg;
    const char *path;
    int fd;
    void *base;
    size_t size;
    int mapped;
    char *result;
};

static int bflag;
static int cflag;
static int iflag;
static int Lflag;
static int sflag;
static int Wflag;
static char *magicpath;
static FILE *magicfp;

static __dead void usage(void);
static int prepare_message(struct input_msg *, const char *);
static int load_file(struct input_file *);
static int try_stat(struct input_file *);
static int try_empty(struct input_file *);
static int try_access(struct input_file *);
static int try_text(struct input_file *);
static int try_magic(struct input_file *);
static int try_unknown(struct input_file *);
static void test_file(struct input_file *, size_t);

static __dead void
usage(void)
{
    fprintf(stderr, "usage: file.exe [-bchiLsW] file ...\n");
    exit(1);
}

static int
prepare_message(struct input_msg *msg, const char *path)
{
    memset(msg, 0, sizeof(*msg));

    if (strcmp(path, "-") == 0) {
        if (fstat(STDIN_FILENO, &msg->sb) == -1) {
            msg->error = errno;
            return -1;
        }
        return STDIN_FILENO;
    }

    if (stat(path, &msg->sb) == -1) {
        msg->error = errno;
        return -1;
    }

    if (!S_ISDIR(msg->sb.st_mode))
        return open(path, O_RDONLY | O_BINARY);
    return -1;
}

static void *
fill_buffer(int fd, size_t size, size_t *used)
{
    static void *buffer;
    ssize_t got;
    size_t left;
    void *next;

    if (buffer == NULL)
        buffer = xmalloc(FILE_READ_SIZE);

    next = buffer;
    left = size;
    while (left != 0) {
        got = read(fd, next, left);
        if (got == -1) {
            if (errno == EINTR)
                continue;
            return NULL;
        }
        if (got == 0)
            break;
        next = (char *)next + got;
        left -= (size_t)got;
    }

    *used = size - left;
    return buffer;
}

static int
load_file(struct input_file *inf)
{
    size_t used;

    if (inf->msg->sb.st_size == 0 && S_ISREG(inf->msg->sb.st_mode))
        return 0;

    if (inf->msg->sb.st_size == 0 || inf->msg->sb.st_size > FILE_READ_SIZE)
        inf->size = FILE_READ_SIZE;
    else
        inf->size = (size_t)inf->msg->sb.st_size;

#ifndef _WIN32
    if (S_ISREG(inf->msg->sb.st_mode)) {
        inf->base = mmap(NULL, inf->size, PROT_READ, MAP_PRIVATE, inf->fd, 0);
        if (inf->base != MAP_FAILED) {
            inf->mapped = 1;
            return 0;
        }
    }
#endif

    inf->base = fill_buffer(inf->fd, inf->size, &used);
    if (inf->base == NULL) {
        xasprintf(&inf->result, "cannot read '%s' (%s)", inf->path,
            strerror(errno));
        return 1;
    }
    inf->size = used;
    return 0;
}

static int
try_stat(struct input_file *inf)
{
    if (inf->msg->error != 0) {
        xasprintf(&inf->result, "cannot stat '%s' (%s)", inf->path,
            strerror(inf->msg->error));
        return 1;
    }

    if (iflag && !S_ISREG(inf->msg->sb.st_mode)) {
        inf->result = xstrdup("application/x-not-regular-file");
        return 1;
    }

    if (sflag || strcmp(inf->path, "-") == 0) {
        if (S_ISREG(inf->msg->sb.st_mode))
            return 0;
    }

    if (S_ISDIR(inf->msg->sb.st_mode)) {
        inf->result = xstrdup("directory");
        return 1;
    }

    return 0;
}

static int
try_empty(struct input_file *inf)
{
    if (inf->size != 0)
        return 0;

    if (iflag)
        inf->result = xstrdup("application/x-empty");
    else
        inf->result = xstrdup("empty");
    return 1;
}

static int
try_access(struct input_file *inf)
{
    char tmp[256] = "";

    if (inf->msg->sb.st_size == 0 && S_ISREG(inf->msg->sb.st_mode))
        return 0;
    if (inf->fd != -1)
        return 0;

    if (inf->msg->sb.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH))
        strlcat(tmp, "writable, ", sizeof(tmp));
    if (inf->msg->sb.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        strlcat(tmp, "executable, ", sizeof(tmp));
    if (S_ISREG(inf->msg->sb.st_mode))
        strlcat(tmp, "regular file, ", sizeof(tmp));
    strlcat(tmp, "no read permission", sizeof(tmp));
    inf->result = xstrdup(tmp);
    return 1;
}

static int
try_text(struct input_file *inf)
{
    const char *type;
    const char *s;
    int flags;

    flags = MAGIC_TEST_TEXT;
    if (iflag)
        flags |= MAGIC_TEST_MIME;

    type = text_get_type(inf->base, inf->size);
    if (type == NULL)
        return 0;

    s = magic_test(inf->m, inf->base, inf->size, flags);
    if (s != NULL) {
        inf->result = xstrdup(s);
        return 1;
    }

    s = text_try_words(inf->base, inf->size, flags);
    if (s != NULL) {
        if (iflag)
            inf->result = xstrdup(s);
        else
            xasprintf(&inf->result, "%s %s text", type, s);
        return 1;
    }

    if (iflag)
        inf->result = xstrdup("text/plain");
    else
        xasprintf(&inf->result, "%s text", type);
    return 1;
}

static int
try_magic(struct input_file *inf)
{
    const char *s;
    int flags;

    flags = 0;
    if (iflag)
        flags |= MAGIC_TEST_MIME;

    s = magic_test(inf->m, inf->base, inf->size, flags);
    if (s != NULL) {
        inf->result = xstrdup(s);
        return 1;
    }
    return 0;
}

static int
try_unknown(struct input_file *inf)
{
    if (iflag)
        inf->result = xstrdup("application/octet-stream");
    else
        inf->result = xstrdup("data");
    return 1;
}

static void
test_file(struct input_file *inf, size_t width)
{
    char *label;
    int stop;

    stop = 0;
    if (!stop)
        stop = try_stat(inf);
    if (!stop)
        stop = try_access(inf);
    if (!stop)
        stop = load_file(inf);
    if (!stop)
        stop = try_empty(inf);
    if (!stop)
        stop = try_magic(inf);
    if (!stop)
        stop = try_text(inf);
    if (!stop)
        stop = try_unknown(inf);

    if (bflag) {
        printf("%s\n", inf->result);
    } else {
        if (strcmp(inf->path, "-") == 0)
            label = xstrdup("/dev/stdin:");
        else
            xasprintf(&label, "%s:", inf->path);
        printf("%-*s %s\n", (int)width, label, inf->result);
        free(label);
    }

    free(inf->result);
#ifndef _WIN32
    if (inf->mapped && inf->base != NULL)
        munmap(inf->base, inf->size);
#endif
}

static int
open_magic_file(void)
{
    if (magicfp != NULL)
        return 0;

    magicfp = tmpfile();
    if (magicfp == NULL)
        return -1;
    if (fwrite(filemagic_embedded_magic, 1, filemagic_embedded_magic_size,
        magicfp) != filemagic_embedded_magic_size)
        return -1;
    if (fflush(magicfp) != 0)
        return -1;
    if (fseek(magicfp, 0, SEEK_SET) != 0)
        return -1;

    magicpath = xstrdup("<embedded>");
    return 0;
}

int
main(int argc, char **argv)
{
    int idx;
    int fd;
    int opt;
    size_t len;
    size_t width;
    struct magic *m;
    struct input_msg msg;
    struct input_file inf;

    tzset();
    for (;;) {
        opt = getopt(argc, argv, "bchiLsW");
        if (opt == -1)
            break;
        switch (opt) {
        case 'b':
            bflag = 1;
            break;
        case 'c':
            cflag = 1;
            break;
        case 'h':
            Lflag = 0;
            break;
        case 'i':
            iflag = 1;
            break;
        case 'L':
            Lflag = 1;
            break;
        case 's':
            sflag = 1;
            break;
        case 'W':
            Wflag = 1;
            break;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;
    if (cflag) {
        if (argc != 0)
            usage();
    } else if (argc == 0) {
        usage();
    }

    if (open_magic_file() != 0) {
        fprintf(stderr, "file.exe: could not load embedded magic database\n");
        return 1;
    }

    m = magic_load(magicfp, magicpath, cflag || Wflag);
    if (cflag) {
        magic_dump(m);
        fclose(magicfp);
        free(magicpath);
        return 0;
    }

    width = 0;
    for (idx = 0; idx < argc; idx++) {
        len = strlen(argv[idx]) + 1;
        if (len > width)
            width = len;
    }

    for (idx = 0; idx < argc; idx++) {
        memset(&inf, 0, sizeof(inf));
        fd = prepare_message(&msg, argv[idx]);
        inf.m = m;
        inf.msg = &msg;
        inf.path = argv[idx];
        inf.fd = fd;
        test_file(&inf, width);
        if (inf.fd != -1 && inf.fd != STDIN_FILENO)
            close(inf.fd);
    }

    fclose(magicfp);
    free(magicpath);
    return 0;
}
