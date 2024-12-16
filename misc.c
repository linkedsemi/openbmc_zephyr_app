#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/shm.h>
void OPENSSL_cleanse(void *ptr, size_t len)
{
    memset(ptr, 0, len);
}

int ioctl(int fd, unsigned long request, ...)
{
    while(1);
}

time_t timegm(struct tm *tm)
{
    return mktime(tm);
}

int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
{

}


int shmctl (int __shmid, int __cmd, struct shmid_ds *__buf){}
int shmget (key_t __key, size_t __size, int __shmflg){}
void *shmat (int __shmid, const void *__shmaddr, int __shmflg){}
int shmdt (const void *__shmaddr){}
int chmod(const char *__path, mode_t __mode){}
int fchmod(int __fd, mode_t __mode){}
int     pipe (int __fildes[2]){}
int _stat (const char *fname, struct stat *st){}
int	munmap(void *, size_t){}
void *	mmap(void *, size_t, int, int, int, off_t){}
int getrlimit (int resource, struct rlimit *rlp){}
