/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _LINUX_SHM_H_
#define _LINUX_SHM_H_
#ifdef __cplusplus
extern "C" {
#endif
/*
 * SHMMNI, SHMMAX and SHMALL are default upper limits which can be
 * modified by sysctl. The SHMMAX and SHMALL values have been chosen to
 * be as large possible without facilitating scenarios where userspace
 * causes overflows when adjusting the limits via operations of the form
 * "retrieve current limit; add X; update limit". It is therefore not
 * advised to make SHMMAX and SHMALL any larger. These limits are
 * suitable for both 32 and 64-bit systems.
 */
#define SHMMIN 1			 /* min shared seg size (bytes) */
#define SHMMNI 4096			 /* max num of segs system wide */
#define SHMMAX (ULONG_MAX - (1UL << 24)) /* max shared seg size (bytes) */
#define SHMALL (ULONG_MAX - (1UL << 24)) /* max shm system wide (pages) */
#define SHMSEG SHMMNI			 /* max shared segs per process */

/*
 * shmget() shmflg values.
 */
/* The bottom nine bits are the same as open(2) mode flags */
#define SHM_R		0400	/* or S_IRUGO from <linux/stat.h> */
#define SHM_W		0200	/* or S_IWUGO from <linux/stat.h> */
/* Bits 9 & 10 are IPC_CREAT and IPC_EXCL */
#define SHM_HUGETLB	04000	/* segment will use huge TLB pages */
#define SHM_NORESERVE	010000	/* don't check for reservations */

/*
 * Huge page size encoding when SHM_HUGETLB is specified, and a huge page
 * size other than the default is desired.  See hugetlb_encode.h
 */
#define SHM_HUGE_SHIFT	HUGETLB_FLAG_ENCODE_SHIFT
#define SHM_HUGE_MASK	HUGETLB_FLAG_ENCODE_MASK

#define SHM_HUGE_64KB	HUGETLB_FLAG_ENCODE_64KB
#define SHM_HUGE_512KB	HUGETLB_FLAG_ENCODE_512KB
#define SHM_HUGE_1MB	HUGETLB_FLAG_ENCODE_1MB
#define SHM_HUGE_2MB	HUGETLB_FLAG_ENCODE_2MB
#define SHM_HUGE_8MB	HUGETLB_FLAG_ENCODE_8MB
#define SHM_HUGE_16MB	HUGETLB_FLAG_ENCODE_16MB
#define SHM_HUGE_32MB	HUGETLB_FLAG_ENCODE_32MB
#define SHM_HUGE_256MB	HUGETLB_FLAG_ENCODE_256MB
#define SHM_HUGE_512MB	HUGETLB_FLAG_ENCODE_512MB
#define SHM_HUGE_1GB	HUGETLB_FLAG_ENCODE_1GB
#define SHM_HUGE_2GB	HUGETLB_FLAG_ENCODE_2GB
#define SHM_HUGE_16GB	HUGETLB_FLAG_ENCODE_16GB

/*
 * shmat() shmflg values
 */
#define	SHM_RDONLY	010000	/* read-only access */
#define	SHM_RND		020000	/* round attach address to SHMLBA boundary */
#define	SHM_REMAP	040000	/* take-over region on attach */
#define	SHM_EXEC	0100000	/* execution access */

/* super user shmctl commands */
#define SHM_LOCK 	11
#define SHM_UNLOCK 	12

/* ipcs ctl commands */
#define SHM_STAT	13
#define SHM_INFO	14
#define SHM_STAT_ANY    15

#define IPC_RMID	0		/* Remove identifier.  */
#define IPC_SET		1		/* Set `ipc_perm' options.  */
#define IPC_STAT	2		/* Get `ipc_perm' options.  */
#define IPC_INFO	3		/* See ipcs.  */

/* Obsolete, used only for backwards compatibility */
struct	shminfo {
	int shmmax;
	int shmmin;
	int shmmni;
	int shmseg;
	int shmall;
};
/* Data structure describing a shared memory segment.  */
struct shmid_ds
{

    size_t shm_segsz;			/* size of segment in bytes */

};

extern int shmctl (int __shmid, int __cmd, struct shmid_ds *__buf) ;
extern int shmget (key_t __key, size_t __size, int __shmflg);
/* Attach shared memory segment.  */
extern void *shmat (int __shmid, const void *__shmaddr, int __shmflg);
/* Detach shared memory segment.  */
extern int shmdt (const void *__shmaddr);

#ifdef __cplusplus
}
#endif
#endif /* _LINUX_SHM_H_ */
