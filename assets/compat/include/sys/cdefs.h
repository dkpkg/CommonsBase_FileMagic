#ifndef COMMONSBASE_FILEMAGIC_SYS_CDEFS_H
#define COMMONSBASE_FILEMAGIC_SYS_CDEFS_H

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#ifndef __bounded__
#define __bounded__(x, y, z)
#endif

#ifndef DEF_WEAK
#define DEF_WEAK(x)
#endif

#endif
