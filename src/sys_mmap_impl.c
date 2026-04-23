/* SPDX-License-Identifier: Apache-2.0 */
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

#ifndef off_t
typedef long off_t;
#endif

/* Simple mmap implementation for Zephyr using malloc/free */
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *ptr;

	/* Zephyr doesn't support file-backed mappings */
	if (!(flags & MAP_ANONYMOUS) && fd >= 0) {
		errno = ENOSYS;
		return MAP_FAILED;
	}

	/* Ignore addr parameter - always allocate new memory */
	(void)addr;
	(void)offset;
	(void)prot;

	/* Allocate memory */
	ptr = malloc(length);
	if (!ptr) {
		errno = ENOMEM;
		return MAP_FAILED;
	}

	return ptr;
}

/* Simple munmap implementation for Zephyr using free */
int munmap(void *addr, size_t length)
{
	(void)length;

	if (!addr) {
		errno = EINVAL;
		return -1;
	}

	free(addr);
	return 0;
}
