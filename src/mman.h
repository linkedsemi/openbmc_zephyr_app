#ifndef ZEPHYR_MMAN_H
#define ZEPHYR_MMAN_H

#include <stddef.h>

/* Memory protection flags */
#ifndef PROT_READ
#define PROT_READ  0x1
#endif

#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif

#ifndef PROT_EXEC
#define PROT_EXEC  0x4
#endif

#ifndef PROT_NONE
#define PROT_NONE  0x0
#endif

/* Mapping flags */
#ifndef MAP_SHARED
#define MAP_SHARED    0x01
#endif

#ifndef MAP_PRIVATE
#define MAP_PRIVATE   0x02
#endif

#ifndef MAP_FIXED
#define MAP_FIXED     0x10
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

/* Functions declared in basu_zephyr_compat.h and implemented in sys_compat.c */
#ifdef __ZEPHYR__
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
#endif

#endif /* ZEPHYR_MMAN_H */
