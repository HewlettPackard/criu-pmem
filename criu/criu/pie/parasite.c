#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <linux/limits.h>
#include <linux/capability.h>
#include <sys/mount.h>
#include <stdarg.h>
#include <sys/ioctl.h>

#include "int.h"
#include "types.h"
#include <compel/plugins/std/syscall.h>
#include "parasite.h"
#include "config.h"
#include "fcntl.h"
#include "prctl.h"
#include "common/lock.h"
#include "parasite-vdso.h"
#include "criu-log.h"
#include "tty.h"
#include "aio.h"

#include "asm/parasite.h"
#include "restorer.h"
#include "infect-pie.h"
//#include <string.h>
//#include <emmintrin.h>


/*
 * PARASITE_CMD_DUMPPAGES is called many times and the parasite args contains
 * an array of VMAs at this time, so VMAs can be unprotected in any moment
 */
static struct parasite_dump_pages_args *mprotect_args = NULL;

#ifndef SPLICE_F_GIFT
#define SPLICE_F_GIFT	0x08
#endif

#ifndef PR_GET_PDEATHSIG
#define PR_GET_PDEATHSIG  2
#endif



static int mprotect_vmas(struct parasite_dump_pages_args *args)
{
	struct parasite_vma_entry *vmas, *vma;
	int ret = 0, i;

	vmas = pargs_vmas(args);
	for (i = 0; i < args->nr_vmas; i++) {
		vma = vmas + i;
		ret = sys_mprotect((void *)vma->start, vma->len, vma->prot | args->add_prot);
		if (ret) {
			pr_err("mprotect(%08lx, %lu) failed with code %d\n",
						vma->start, vma->len, ret);
			break;
		}
	}

	if (args->add_prot)
		mprotect_args = args;
	else
		mprotect_args = NULL;

	return ret;
}



#if 0


#define FLUSH_ALIGN ((uintptr_t)64)

#define ALIGN_MASK	(FLUSH_ALIGN - 1)

#define CHUNK_SIZE	128 /* 16*8 */
#define CHUNK_SHIFT	7
#define CHUNK_MASK	(CHUNK_SIZE - 1)

#define DWORD_SIZE	4
#define DWORD_SHIFT	2
#define DWORD_MASK	(DWORD_SIZE - 1)

#define MOVNT_SIZE	16
#define MOVNT_MASK	(MOVNT_SIZE - 1)
#define MOVNT_SHIFT	4

#define MOVNT_THRESHOLD	256

static size_t  Movnt_threshold = MOVNT_THRESHOLD;

/*
 ** memmove_nodrain_movnt -- (internal) memmove to pmem without hw drain, movnt
 **/
static void *
memmove_nodrain_movnt(void *pmemdest, const void *src, size_t len)
{
//	LOG(15, "pmemdest %p src %p len %zu", pmemdest, src, len);

	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	size_t i;
	__m128i *d;
	__m128i *s;
	void *dest1 = pmemdest;
	size_t cnt;

	if (len == 0 || src == pmemdest)
		return pmemdest;


	if (len < Movnt_threshold) {
		while(1) ;
		return NULL;
//		memmove(pmemdest, src, len);
//		pmem_flush(pmemdest, len);
//		return pmemdest;
	}


	if ((uintptr_t)dest1 - (uintptr_t)src >= len) {
		/*
 * 		 * Copy the range in the forward direction.
 * 		 *
 * 		 * This is the most common, most optimized case, used unless
 * 		 * the overlap specifically prevents it.
 * 		 */

		/* copy up to FLUSH_ALIGN boundary */
		cnt = (uint64_t)dest1 & ALIGN_MASK;
		if (cnt > 0) {
		#if 0
			cnt = FLUSH_ALIGN - cnt;

			/* never try to copy more the len bytes */
			if (cnt > len)
				cnt = len;

			uint8_t *d8 = (uint8_t *)dest1;
			const uint8_t *s8 = (uint8_t *)src;
			for (i = 0; i < cnt; i++) {
				*d8 = *s8;
				d8++;
				s8++;
			}
			//pmem_flush(dest1, cnt);
			dest1 = (char *)dest1 + cnt;
			src = (char *)src + cnt;
			len -= cnt;
		#endif
			while(1) ;
			return NULL;
		}

		d = (__m128i *)dest1;
		s = (__m128i *)src;

		cnt = len >> CHUNK_SHIFT;
		for (i = 0; i < cnt; i++) {

			xmm0 = _mm_loadu_si128(s);
			xmm1 = _mm_loadu_si128(s + 1);
			xmm2 = _mm_loadu_si128(s + 2);
			xmm3 = _mm_loadu_si128(s + 3);
			xmm4 = _mm_loadu_si128(s + 4);
			xmm5 = _mm_loadu_si128(s + 5);
			xmm6 = _mm_loadu_si128(s + 6);
			xmm7 = _mm_loadu_si128(s + 7);

			s += 8;

			_mm_stream_si128(d,	xmm0);
			_mm_stream_si128(d + 1,	xmm1);
			_mm_stream_si128(d + 2,	xmm2);
			_mm_stream_si128(d + 3,	xmm3);
			_mm_stream_si128(d + 4,	xmm4);
			_mm_stream_si128(d + 5, xmm5);
			_mm_stream_si128(d + 6,	xmm6);
			_mm_stream_si128(d + 7,	xmm7);

/*
			_mm_store_si128(d,	xmm0);
			_mm_store_si128(d + 1,	xmm1);
			_mm_store_si128(d + 2,	xmm2);
			_mm_store_si128(d + 3,	xmm3);
			_mm_store_si128(d + 4,	xmm4);
			_mm_store_si128(d + 5,  xmm5);
			_mm_store_si128(d + 6,	xmm6);
			_mm_store_si128(d + 7,	xmm7);
*/
/*
			_mm_storeu_si128(d,	xmm0);
			_mm_storeu_si128(d + 1,	xmm1);
			_mm_storeu_si128(d + 2,	xmm2);
			_mm_storeu_si128(d + 3,	xmm3);
			_mm_storeu_si128(d + 4,	xmm4);
			_mm_storeu_si128(d + 5,  xmm5);
			_mm_storeu_si128(d + 6,	xmm6);
			_mm_storeu_si128(d + 7,	xmm7);
*/
	//		VALGRIND_DO_FLUSH(d, 8 * sizeof(*d));
			d += 8;
		}

		/* copy the tail (<128 bytes) in 16 bytes chunks */
		len &= CHUNK_MASK;
		if (len != 0) {
		#if 0
			cnt = len >> MOVNT_SHIFT;
			for (i = 0; i < cnt; i++) {
				xmm0 = _mm_loadu_si128(s);
				_mm_stream_si128(d, xmm0);
				VALGRIND_DO_FLUSH(d, sizeof(*d));
				s++;
				d++;
			}
		#endif
			while(1) ;
			return NULL;
		}

		/* copy the last bytes (<16), first dwords then bytes */
		len &= MOVNT_MASK;
		if (len != 0) {
		#if 0
			cnt = len >> DWORD_SHIFT;
			int32_t *d32 = (int32_t *)d;
			int32_t *s32 = (int32_t *)s;
			for (i = 0; i < cnt; i++) {
				_mm_stream_si32(d32, *s32);
				VALGRIND_DO_FLUSH(d32, sizeof(*d32));
				d32++;
				s32++;
			}
			cnt = len & DWORD_MASK;
			uint8_t *d8 = (uint8_t *)d32;
			const uint8_t *s8 = (uint8_t *)s32;

			for (i = 0; i < cnt; i++) {
				*d8 = *s8;
				d8++;
				s8++;
			}
			pmem_flush(d32, cnt);
		#endif
			while(1) ;
			return NULL;
		}
	} else {
#if 0
		/*
 * 		 * Copy the range in the backward direction.
 * 		 		 *
 * 		 		 		 * This prevents overwriting source data due to an
 * 		 		 		 		 * overlapped destination range.
 * 		 		 		 		 		 */

		dest1 = (char *)dest1 + len;
		src = (char *)src + len;

		cnt = (uint64_t)dest1 & ALIGN_MASK;
		if (cnt > 0) {
			/* never try to copy more the len bytes */
			if (cnt > len)
				cnt = len;

			uint8_t *d8 = (uint8_t *)dest1;
			const uint8_t *s8 = (uint8_t *)src;
			for (i = 0; i < cnt; i++) {
				d8--;
				s8--;
				*d8 = *s8;
			}
			pmem_flush(d8, cnt);
			dest1 = (char *)dest1 - cnt;
			src = (char *)src - cnt;
			len -= cnt;
		}

		d = (__m128i *)dest1;
		s = (__m128i *)src;

		cnt = len >> CHUNK_SHIFT;
		for (i = 0; i < cnt; i++) {
			xmm0 = _mm_loadu_si128(s - 1);
			xmm1 = _mm_loadu_si128(s - 2);
			xmm2 = _mm_loadu_si128(s - 3);
			xmm3 = _mm_loadu_si128(s - 4);
			xmm4 = _mm_loadu_si128(s - 5);
			xmm5 = _mm_loadu_si128(s - 6);
			xmm6 = _mm_loadu_si128(s - 7);
			xmm7 = _mm_loadu_si128(s - 8);
			s -= 8;
			_mm_stream_si128(d - 1, xmm0);
			_mm_stream_si128(d - 2, xmm1);
			_mm_stream_si128(d - 3, xmm2);
			_mm_stream_si128(d - 4, xmm3);
			_mm_stream_si128(d - 5, xmm4);
			_mm_stream_si128(d - 6, xmm5);
			_mm_stream_si128(d - 7, xmm6);
			_mm_stream_si128(d - 8, xmm7);
			d -= 8;
			VALGRIND_DO_FLUSH(d, 8 * sizeof(*d));
		}

		/* copy the tail (<128 bytes) in 16 bytes chunks */
		len &= CHUNK_MASK;
		if (len != 0) {
			cnt = len >> MOVNT_SHIFT;
			for (i = 0; i < cnt; i++) {
				d--;
				s--;
				xmm0 = _mm_loadu_si128(s);
				_mm_stream_si128(d, xmm0);
				VALGRIND_DO_FLUSH(d, sizeof(*d));
			}
		}

		/* copy the last bytes (<16), first dwords then bytes */
		len &= MOVNT_MASK;
		if (len != 0) {
			cnt = len >> DWORD_SHIFT;
			int32_t *d32 = (int32_t *)d;
			int32_t *s32 = (int32_t *)s;
			for (i = 0; i < cnt; i++) {
				d32--;
				s32--;
				_mm_stream_si32(d32, *s32);
				VALGRIND_DO_FLUSH(d32, sizeof(*d32));
			}

			cnt = len & DWORD_MASK;
			uint8_t *d8 = (uint8_t *)d32;
			const uint8_t *s8 = (uint8_t *)s32;

			for (i = 0; i < cnt; i++) {
				d8--;
				s8--;
				*d8 = *s8;
			}
			pmem_flush(d8, cnt);
		}
	#endif
		while(1) ;
		return NULL;
	}

	/* serialize non-temporal store instructions */
	//
	//predrain_fence_sfence();

	//_mm_sfence();

	return pmemdest;
}

#endif

#if 0

static int dump_pages_nvm(struct parasite_dump_pages_args *args)
{
	int ret;
	int i, count = 14;
	void *fileaddr; // to munmap
	void *addr; // to memcpy
	struct iovec *iovs;
	//void *ptr;
//	char str1[16] = "I am here, 4!";


	// fallocate and mmap file here, after memcpy, munmap the file

	// 1. fallocate file with exact file size
	ret = sys_fallocate(args->fd, 0, args->file_offset*PAGE_SIZE, args->pages*PAGE_SIZE);
	if(ret != 0)
		return -1;


	// 2. mmap file
	fileaddr = (char *)sys_mmap(0, args->pages*PAGE_SIZE, PROT_WRITE, 
			MAP_SHARED, args->fd, args->file_offset*PAGE_SIZE);
	args->pmemaddr = fileaddr;
	addr = fileaddr;

	if(fileaddr == MAP_FAILED){
		sys_close(args->fd);
		pr_err("mmap");
		return -1;
	}

	// 3. Close file
	if(args->is_last == true)
		sys_close(args->fd);

	// Get IO vects info
	iovs = pargs_iovs(args);

//	if(count > args->segs)
//		count = args->segs;

	// 4. copy to NVM here
	for(i = 0; i < args->segs; i++){

		//normal memcpy
		memcpy(addr, iovs[args->seg_off+i].iov_base, iovs[args->seg_off+i].iov_len);

		// nt-store
//		ptr = memmove_nodrain_movnt(addr, iovs[args->seg_off+i].iov_base, iovs[args->seg_off+i].iov_len);

	//	if(ptr == NULL){
	//		pr_err("memmove");
	//		return -1;
	//	}
/*
		if(i < count){
			args->addrs[i] = iovs[args->seg_off+i].iov_base;
			args->len[i] = iovs[args->seg_off+i].iov_len;
		}
*/
		addr += iovs[args->seg_off+i].iov_len;
	}

	// 5. mummap here
	ret = sys_munmap(fileaddr, args->pages*PAGE_SIZE);
	if(ret == -1){
		pr_err("munmap");
		return -1;
	}

	return 0;
}

#endif


static int dump_pages_nvm(struct parasite_dump_pages_args *args)
{
	ssize_t ret;
//	ssize_t total;
//	int i;
//	int count = 14;
	struct iovec *iovs;


	// get IO vectors info
	iovs = pargs_iovs(args);

	//if(count > args->segs)
	//	count = args->segs;

	// write to file

	ret = sys_writev(args->fd, &iovs[args->seg_off], args->segs);

#if 0
	total = 0;
	for(i = 0; i < args->segs; i++){

	//	ret = sys_write(args->fd, iovs[args->seg_off+i].iov_base, iovs[args->seg_off+i].iov_len);

		total +=  iovs[args->seg_off+i].iov_len;
/*
		if(ret != iovs[args->seg_off+i].iov_len){
			pr_err("write");
			return -1;
		}
*/
/*
		if(i < count){
			args->addrs[i] = iovs[args->seg_off+i].iov_base;
			args->len[i] = iovs[args->seg_off+i].iov_len;
		}
*/
	}

#endif

	if(ret != args->pages*PAGE_SIZE){
//		args->a = ret;
//		args->b = total;
		pr_err("writev");
		return -1;
	}

	// Close file for the last write
	if(args->is_last == true)
		sys_close(args->fd);

	return 0;
}


static int dump_pages(struct parasite_dump_pages_args *args)
{
	int p, ret, tsock;
	struct iovec *iovs;
//	int count = 14, i;

	tsock = parasite_get_rpc_sock();
	p = recv_fd(tsock);
	if (p < 0)
		return -1;

	iovs = pargs_iovs(args);

	ret = sys_vmsplice(p, &iovs[args->off], args->nr_segs,
				SPLICE_F_GIFT | SPLICE_F_NONBLOCK);

/*
	if(count > args->nr_segs)
		count = args->nr_segs;

	for(i = 0; i < count; i++){
		args->addrs[i] = iovs[args->off+i].iov_base;
		args->len[i] = iovs[args->off+i].iov_len;
	}
*/
	if (ret != PAGE_SIZE * args->nr_pages) {
		sys_close(p);
		pr_err("Can't splice pages to pipe (%d/%d)\n", ret, args->nr_pages);
		return -1;
	}

	sys_close(p);
	return 0;
}


static int dump_sigact(struct parasite_dump_sa_args *da)
{
	int sig, ret = 0;

	for (sig = 1; sig <= SIGMAX; sig++) {
		int i = sig - 1;

		if (sig == SIGKILL || sig == SIGSTOP)
			continue;

		ret = sys_sigaction(sig, NULL, &da->sas[i], sizeof(k_rtsigset_t));
		if (ret < 0) {
			pr_err("sys_sigaction failed (%d)\n", ret);
			break;
		}
	}

	return ret;
}

static int dump_itimers(struct parasite_dump_itimers_args *args)
{
	int ret;

	ret = sys_getitimer(ITIMER_REAL, &args->real);
	if (!ret)
		ret = sys_getitimer(ITIMER_VIRTUAL, &args->virt);
	if (!ret)
		ret = sys_getitimer(ITIMER_PROF, &args->prof);

	if (ret)
		pr_err("getitimer failed (%d)\n", ret);

	return ret;
}

static int dump_posix_timers(struct parasite_dump_posix_timers_args *args)
{
	int i;
	int ret = 0;

	for(i = 0; i < args->timer_n; i++) {
		ret = sys_timer_gettime(args->timer[i].it_id, &args->timer[i].val);
		if (ret < 0) {
			pr_err("sys_timer_gettime failed (%d)\n", ret);
			return ret;
		}
		args->timer[i].overrun = sys_timer_getoverrun(args->timer[i].it_id);
		ret = args->timer[i].overrun;
		if (ret < 0) {
			pr_err("sys_timer_getoverrun failed (%d)\n", ret);
			return ret;
		}
	}

	return ret;
}

static int dump_creds(struct parasite_dump_creds *args);

static int dump_thread_common(struct parasite_dump_thread *ti)
{
	int ret;

	arch_get_tls(&ti->tls);
	ret = sys_prctl(PR_GET_TID_ADDRESS, (unsigned long) &ti->tid_addr, 0, 0, 0);
	if (ret)
		goto out;

	ret = sys_sigaltstack(NULL, &ti->sas);
	if (ret)
		goto out;

	ret = sys_prctl(PR_GET_PDEATHSIG, (unsigned long)&ti->pdeath_sig, 0, 0, 0);
	if (ret)
		goto out;

	ret = dump_creds(ti->creds);
out:
	return ret;
}

static int dump_misc(struct parasite_dump_misc *args)
{
	args->brk = sys_brk(0);

	args->pid = sys_getpid();
	args->sid = sys_getsid();
	args->pgid = sys_getpgid(0);
	args->umask = sys_umask(0);
	sys_umask(args->umask); /* never fails */
	args->dumpable = sys_prctl(PR_GET_DUMPABLE, 0, 0, 0, 0);

	return 0;
}

static int dump_creds(struct parasite_dump_creds *args)
{
	int ret, i, j;
	struct cap_data data[_LINUX_CAPABILITY_U32S_3];
	struct cap_header hdr = {_LINUX_CAPABILITY_VERSION_3, 0};

	ret = sys_capget(&hdr, data);
	if (ret < 0) {
		pr_err("Unable to get capabilities: %d\n", ret);
		return -1;
	}

	/*
	 * Loop through the capability constants until we reach cap_last_cap.
	 * The cap_bnd set is stored as a bitmask comprised of CR_CAP_SIZE number of
	 * 32-bit uints, hence the inner loop from 0 to 32.
	 */
	for (i = 0; i < CR_CAP_SIZE; i++) {
		args->cap_eff[i] = data[i].eff;
		args->cap_prm[i] = data[i].prm;
		args->cap_inh[i] = data[i].inh;
		args->cap_bnd[i] = 0;

		for (j = 0; j < 32; j++) {
			if (j + i * 32 > args->cap_last_cap)
				break;
			ret = sys_prctl(PR_CAPBSET_READ, j + i * 32, 0, 0, 0);
			if (ret < 0) {
				pr_err("Unable to read capability %d: %d\n",
					j + i * 32, ret);
				return -1;
			}
			if (ret)
				args->cap_bnd[i] |= (1 << j);
		}
	}

	args->secbits = sys_prctl(PR_GET_SECUREBITS, 0, 0, 0, 0);

	ret = sys_getgroups(0, NULL);
	if (ret < 0)
		goto grps_err;

	args->ngroups = ret;
	if (args->ngroups >= PARASITE_MAX_GROUPS) {
		pr_err("Too many groups in task %d\n", (int)args->ngroups);
		return -1;
	}

	ret = sys_getgroups(args->ngroups, args->groups);
	if (ret < 0)
		goto grps_err;

	if (ret != args->ngroups) {
		pr_err("Groups changed on the fly %d -> %d\n",
				args->ngroups, ret);
		return -1;
	}

	ret = sys_getresuid(&args->uids[0], &args->uids[1], &args->uids[2]);
	if (ret) {
		pr_err("Unable to get uids: %d\n", ret);
		return -1;
	}

	args->uids[3] = sys_setfsuid(-1L);

	/*
	 * FIXME In https://github.com/xemul/criu/issues/95 it is
	 * been reported that only low 16 bits are set upon syscall
	 * on ARMv7.
	 *
	 * We may rather need implement builtin-memset and clear the
	 * whole memory needed here.
	 */
	args->gids[0] = args->gids[1] = args->gids[2] = args->gids[3] = 0;

	ret = sys_getresgid(&args->gids[0], &args->gids[1], &args->gids[2]);
	if (ret) {
		pr_err("Unable to get uids: %d\n", ret);
		return -1;
	}

	args->gids[3] = sys_setfsgid(-1L);

	return 0;

grps_err:
	pr_err("Error calling getgroups (%d)\n", ret);
	return -1;
}

static int fill_fds_opts(struct parasite_drain_fd *fds, struct fd_opts *opts)
{
	int i;

	for (i = 0; i < fds->nr_fds; i++) {
		int flags, fd = fds->fds[i], ret;
		struct fd_opts *p = opts + i;
		struct f_owner_ex owner_ex;
		uint32_t v[2];

		flags = sys_fcntl(fd, F_GETFD, 0);
		if (flags < 0) {
			pr_err("fcntl(%d, F_GETFD) -> %d\n", fd, flags);
			return -1;
		}

		p->flags = (char)flags;

		ret = sys_fcntl(fd, F_GETOWN_EX, (long)&owner_ex);
		if (ret) {
			pr_err("fcntl(%d, F_GETOWN_EX) -> %d\n", fd, ret);
			return -1;
		}

		/*
		 * Simple case -- nothing is changed.
		 */
		if (owner_ex.pid == 0) {
			p->fown.pid = 0;
			continue;
		}

		ret = sys_fcntl(fd, F_GETOWNER_UIDS, (long)&v);
		if (ret) {
			pr_err("fcntl(%d, F_GETOWNER_UIDS) -> %d\n", fd, ret);
			return -1;
		}

		p->fown.uid	 = v[0];
		p->fown.euid	 = v[1];
		p->fown.pid_type = owner_ex.type;
		p->fown.pid	 = owner_ex.pid;
	}

	return 0;
}

static int drain_fds(struct parasite_drain_fd *args)
{
	int ret, tsock;
	struct fd_opts *opts;

	/*
	 * See the drain_fds_size() in criu code, the memory
	 * for this args is ensured to be large enough to keep
	 * an array of fd_opts at the tail.
	 */
	opts = ((void *)args) + sizeof(*args) + args->nr_fds * sizeof(args->fds[0]);
	ret = fill_fds_opts(args, opts);
	if (ret)
		return ret;

	tsock = parasite_get_rpc_sock();
	ret = send_fds(tsock, NULL, 0,
		       args->fds, args->nr_fds, opts, sizeof(struct fd_opts));
	if (ret)
		pr_err("send_fds failed (%d)\n", ret);

	return ret;
}

static int dump_thread(struct parasite_dump_thread *args)
{
	args->tid = sys_gettid();
	return dump_thread_common(args);
}

static char proc_mountpoint[] = "proc.crtools";

static int pie_atoi(char *str)
{
	int ret = 0;

	while (*str) {
		ret *= 10;
		ret += *str - '0';
		str++;
	}

	return ret;
}

static int get_proc_fd(void)
{
	int ret;
	char buf[11];

	ret = sys_readlinkat(AT_FDCWD, "/proc/self", buf, sizeof(buf) - 1);
	if (ret < 0 && ret != -ENOENT) {
		pr_err("Can't readlink /proc/self (%d)\n", ret);
		return ret;
	}
	if (ret > 0) {
		buf[ret] = 0;

		/* Fast path -- if /proc belongs to this pidns */
		if (pie_atoi(buf) == sys_getpid())
			return sys_open("/proc", O_RDONLY, 0);
	}

	ret = sys_mkdir(proc_mountpoint, 0700);
	if (ret) {
		pr_err("Can't create a directory (%d)\n", ret);
		return -1;
	}

	ret = sys_mount("proc", proc_mountpoint, "proc", MS_MGC_VAL, NULL);
	if (ret) {
		if (ret == -EPERM)
			pr_err("can't dump unpriviliged task whose /proc doesn't belong to it\n");
		else
			pr_err("mount failed (%d)\n", ret);
		sys_rmdir(proc_mountpoint);
		return -1;
	}

	return open_detach_mount(proc_mountpoint);
}

static int parasite_get_proc_fd(void)
{
	int fd, ret, tsock;

	fd = get_proc_fd();
	if (fd < 0) {
		pr_err("Can't get /proc fd\n");
		return -1;
	}

	tsock = parasite_get_rpc_sock();
	ret = send_fd(tsock, NULL, 0, fd);
	sys_close(fd);
	return ret;
}

static inline int tty_ioctl(int fd, int cmd, int *arg)
{
	int ret;

	ret = sys_ioctl(fd, cmd, (unsigned long)arg);
	if (ret < 0) {
		if (ret != -ENOTTY)
			return ret;
		*arg = 0;
	}
	return 0;
}

/*
 * Stolen from kernel/fs/aio.c
 *
 * Is it valid to go to memory and check it? Should be,
 * as libaio does the same.
 */

#define AIO_RING_MAGIC			0xa10a10a1
#define AIO_RING_COMPAT_FEATURES	1
#define AIO_RING_INCOMPAT_FEATURES	0

static int sane_ring(struct parasite_aio *aio)
{
	struct aio_ring *ring = (struct aio_ring *)aio->ctx;
	unsigned nr;

	nr = (aio->size - sizeof(struct aio_ring)) / sizeof(struct io_event);

	return ring->magic == AIO_RING_MAGIC &&
		ring->compat_features == AIO_RING_COMPAT_FEATURES &&
		ring->incompat_features == AIO_RING_INCOMPAT_FEATURES &&
		ring->header_length == sizeof(struct aio_ring) &&
		ring->nr == nr;
}

static int parasite_check_aios(struct parasite_check_aios_args *args)
{
	int i;

	for (i = 0; i < args->nr_rings; i++) {
		struct aio_ring *ring;

		ring = (struct aio_ring *)args->ring[i].ctx;
		if (!sane_ring(&args->ring[i])) {
			pr_err("Not valid ring #%d\n", i);
			pr_info(" `- magic %x\n", ring->magic);
			pr_info(" `- cf    %d\n", ring->compat_features);
			pr_info(" `- if    %d\n", ring->incompat_features);
			pr_info(" `- header size  %d (%zd)\n", ring->header_length, sizeof(struct aio_ring));
			pr_info(" `- nr    %d\n", ring->nr);
			return -1;
		}

		/* XXX: wait aio completion */
	}

	return 0;
}

static int parasite_dump_tty(struct parasite_tty_args *args)
{
	int ret;

#ifndef TIOCGPKT
# define TIOCGPKT	_IOR('T', 0x38, int)
#endif

#ifndef TIOCGPTLCK
# define TIOCGPTLCK	_IOR('T', 0x39, int)
#endif

#ifndef TIOCGEXCL
# define TIOCGEXCL	_IOR('T', 0x40, int)
#endif

	args->sid = 0;
	args->pgrp = 0;
	args->st_pckt = 0;
	args->st_lock = 0;
	args->st_excl = 0;

#define __tty_ioctl(cmd, arg)					\
	do {							\
		ret = tty_ioctl(args->fd, cmd, &arg);		\
		if (ret < 0) {					\
			if (ret == -ENOTTY)			\
				arg = 0;			\
			else if (ret == -EIO)			\
				goto err_io;			\
			else					\
				goto err;			\
		}						\
	} while (0)

	__tty_ioctl(TIOCGSID, args->sid);
	__tty_ioctl(TIOCGPGRP, args->pgrp);
	__tty_ioctl(TIOCGEXCL,	args->st_excl);

	if (args->type == TTY_TYPE__PTY) {
		__tty_ioctl(TIOCGPKT,	args->st_pckt);
		__tty_ioctl(TIOCGPTLCK,	args->st_lock);
	}

	args->hangup = false;
	return 0;

err:
	pr_err("tty: Can't fetch params: err = %d\n", ret);
	return -1;
err_io:

	/* kernel reports EIO for get ioctls on pair-less ptys */
	pr_debug("tty: EIO on tty\n");
	args->hangup = true;
	return 0;
#undef __tty_ioctl
}

#ifdef CONFIG_VDSO
static int parasite_check_vdso_mark(struct parasite_vdso_vma_entry *args)
{
	struct vdso_mark *m = (void *)args->start;

	if (is_vdso_mark(m)) {
		/*
		 * Make sure we don't meet some corrupted entry
		 * where signature matches but verions is not!
		 */
		if (m->version != VDSO_MARK_CUR_VERSION) {
			pr_err("vdso: Mark version mismatch!\n");
			return -EINVAL;
		}
		args->is_marked = 1;
		args->proxy_vdso_addr = m->proxy_vdso_addr;
		args->proxy_vvar_addr = m->proxy_vvar_addr;
	} else {
		args->is_marked = 0;
		args->proxy_vdso_addr = VDSO_BAD_ADDR;
		args->proxy_vvar_addr = VVAR_BAD_ADDR;

		if (args->try_fill_symtable) {
			struct vdso_symtable t;

			if (vdso_fill_symtable(args->start, args->len, &t))
				args->is_vdso = false;
			else
				args->is_vdso = true;
		}
	}

	return 0;
}
#else
static inline int parasite_check_vdso_mark(struct parasite_vdso_vma_entry *args)
{
	pr_err("Unexpected VDSO check command\n");
	return -1;
}
#endif

static int parasite_dump_cgroup(struct parasite_dump_cgroup_args *args)
{
	int proc, cgroup, len;

	proc = get_proc_fd();
	if (proc < 0) {
		pr_err("can't get /proc fd\n");
		return -1;
	}

	cgroup = sys_openat(proc, "self/cgroup", O_RDONLY, 0);
	sys_close(proc);
	if (cgroup < 0) {
		pr_err("can't get /proc/self/cgroup fd\n");
		sys_close(cgroup);
		return -1;
	}

	len = sys_read(cgroup, args->contents, sizeof(args->contents));
	sys_close(cgroup);
	if (len < 0) {
		pr_err("can't read /proc/self/cgroup %d\n", len);
		return -1;
	}

	if (len == sizeof(*args)) {
		pr_warn("/proc/self/cgroup was bigger than the page size\n");
		return -1;
	}

	/* null terminate */
	args->contents[len] = 0;
	return 0;
}

void parasite_cleanup(void)
{
	if (mprotect_args) {
		mprotect_args->add_prot = 0;
		mprotect_vmas(mprotect_args);
	}
}

int parasite_daemon_cmd(int cmd, void *args)
{
	int ret;

	switch (cmd) {
	case PARASITE_CMD_DUMPPAGES:
		ret = dump_pages(args);
		break;
	case PARASITE_CMD_DUMPPAGES_NVM:
		ret = dump_pages_nvm(args);
		break;
	case PARASITE_CMD_MPROTECT_VMAS:
		ret = mprotect_vmas(args);
		break;
	case PARASITE_CMD_DUMP_SIGACTS:
		ret = dump_sigact(args);
		break;
	case PARASITE_CMD_DUMP_ITIMERS:
		ret = dump_itimers(args);
		break;
	case PARASITE_CMD_DUMP_POSIX_TIMERS:
		ret = dump_posix_timers(args);
		break;
	case PARASITE_CMD_DUMP_THREAD:
		ret = dump_thread(args);
		break;
	case PARASITE_CMD_DUMP_MISC:
		ret = dump_misc(args);
		break;
	case PARASITE_CMD_DRAIN_FDS:
		ret = drain_fds(args);
		break;
	case PARASITE_CMD_GET_PROC_FD:
		ret = parasite_get_proc_fd();
		break;
	case PARASITE_CMD_DUMP_TTY:
		ret = parasite_dump_tty(args);
		break;
	case PARASITE_CMD_CHECK_AIOS:
		ret = parasite_check_aios(args);
		break;
	case PARASITE_CMD_CHECK_VDSO_MARK:
		ret = parasite_check_vdso_mark(args);
		break;
	case PARASITE_CMD_DUMP_CGROUP:
		ret = parasite_dump_cgroup(args);
		break;
	default:
		pr_err("Unknown command in parasite daemon thread leader: %d\n", cmd);
		ret = -1;
		break;
	}

	return ret;
}

int parasite_trap_cmd(int cmd, void *args)
{
	switch (cmd) {
	case PARASITE_CMD_DUMP_THREAD:
		return dump_thread(args);
	}

	pr_err("Unknown command to parasite: %d\n", cmd);
	return -EINVAL;
}
