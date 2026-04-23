#include <string.h>
#include <stdarg.h>
#include <time.h>
// #include <sys/shm.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <zephyr/kernel.h>

// void OPENSSL_cleanse(void *ptr, size_t len)
// {
//     memset(ptr, 0, len);
// }

// int ioctl(int fd, unsigned long request, ...)
// {
//     // while(1);
// }

// time_t timegm(struct tm *tm)
// {
//     return mktime(tm);
// }

int posix_memalign(void **__memptr, size_t __alignment, size_t __size)
{
    // void *ret = k_aligned_alloc(__alignment,__size);
    //工具链中的aligned_alloc会跳转到此posix_memalign函数,不能直接调用void * aligned_alloc(size_t, size_t);
    __ASSERT(__alignment / sizeof(void *) >= 1
    && (__alignment % sizeof(void *)) == 0,
    "align must be a multiple of sizeof(void *)");

    __ASSERT((__alignment & (__alignment - 1)) == 0,
        "align must be a power of 2");

    void *ret = memalign(__alignment,__size);
	if (ret == NULL && __size != 0) {
		return ENOMEM;
	}
    *__memptr = ret;
    return 0;
}

// int zvfprintf(struct fs_file_t *, const char *__restrict, va_list arg)
// {
//     printk("zvfprintf not yet realized\n");
//     return 0;
// }
// int zfprintf(struct fs_file_t *, const char *__restrict, ...)
// {
//     printk("zfprintf not yet realized\n");
//     return 0;
// }
// int shmctl (int __shmid, int __cmd, struct shmid_ds *__buf)
// {
//     return 0;
// }
// int shmget (key_t __key, size_t __size, int __shmflg)
// {
//     return 0;
// }
void *shmat (int __shmid, const void *__shmaddr, int __shmflg)
{
    return 0;
}
int shmdt (const void *__shmaddr)
{
    return 0;
}
// int chmod(const char *__path, mode_t __mode)
// {
//     return 0;
// }
// int fchmod(int __fd, mode_t __mode)
// {
//     return 0;
// }
// int     pipe (int __fildes[2])
// {
//     return 0;
// }
// int _stat (const char *fname, struct stat *st)
// {
//     return stat(fname, st);
// }

// int _mkdir(const char *path, mode_t mode)
// {
//     return mkdir(path, mode);
// }

// int	munmap(void *, size_t)
// {
//     return 0;
// }
// void *	mmap(void *, size_t, int, int, int, off_t)
// {
//     return 0;
// }
int getrlimit (int resource, struct rlimit *rlp)
{
    if (resource == RLIMIT_STACK)
    {
        rlp->rlim_max = 0x100000;
        rlp->rlim_cur = 0;
        return 0;
    }
    
    return 1;
}

// Fake link handling by dummy functions:
// int _link(const char* oldpath, const char* newpath) {
//     //  has no link -> always return an error!
//     errno = ENOSYS;
//     return -1;
// }
