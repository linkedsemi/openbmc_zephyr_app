#ifndef ZEPHYR_WAIT_H
#define ZEPHYR_WAIT_H

/* Process status macros for wait/waitpid */
#ifndef WIFEXITED
#define WIFEXITED(status) (((status) & 0x7f) == 0)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#endif

#ifndef WIFSIGNALED
#define WIFSIGNALED(status) (!WIFEXITED(status) && !WIFSTOPPED(status))
#endif

#ifndef WTERMSIG
#define WTERMSIG(status) ((status) & 0x7f)
#endif

#ifndef WIFSTOPPED
#define WIFSTOPPED(status) (((status) & 0xff) == 0x7f)
#endif

#ifndef WSTOPSIG
#define WSTOPSIG(status) (((status) >> 8) & 0xff)
#endif

#ifndef WNOHANG
#define WNOHANG 1
#endif

#ifndef WUNTRACED
#define WUNTRACED 2
#endif

/* Function declarations */
#ifdef __ZEPHYR__
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
#endif

#endif /* ZEPHYR_WAIT_H */
