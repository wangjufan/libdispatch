//
//  semaphore_thread.c
#include "semaphore_thread.h"

#pragma mark -
#pragma mark _dispatch_thread_semaphore_t

DISPATCH_NOINLINE
static _dispatch_thread_semaphore_t
_dispatch_thread_semaphore_create(void)
{
	_dispatch_safe_fork = false;
#if USE_MACH_SEM
	semaphore_t s4;
	kern_return_t kr;
	while (slowpath(kr = semaphore_create(mach_task_self(), &s4,
										  SYNC_POLICY_FIFO, 0))) {
		DISPATCH_VERIFY_MIG(kr);
		sleep(1);
	}
	return s4;
//	pv 44w
//	uv 14w
//
#elif USE_POSIX_SEM
	sem_t *s4 = malloc(sizeof(*s4));
	int ret = sem_init(s4, 0, 0);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
	return (_dispatch_thread_semaphore_t) s4;
#elif USE_FUTEX_SEM
	dispatch_futex_t *futex = malloc(sizeof(*futex));
	int ret = _dispatch_futex_init(futex);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
	return (_dispatch_thread_semaphore_t)futex;
#endif
}
DISPATCH_NOINLINE
void
_dispatch_thread_semaphore_dispose(_dispatch_thread_semaphore_t sema)
{
#if USE_MACH_SEM
	semaphore_t s4 = (semaphore_t)sema;
	kern_return_t kr = semaphore_destroy(mach_task_self(), s4);
	DISPATCH_SEMAPHORE_VERIFY_KR(kr);
#elif USE_POSIX_SEM
	int ret = sem_destroy((sem_t *)sema);
	free((sem_t *) sema);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#elif USE_FUTEX_SEM
	int ret = _dispatch_futex_dispose((dispatch_futex_t *)sema);
	free((dispatch_futex_t *)sema);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

void
_dispatch_thread_semaphore_signal(_dispatch_thread_semaphore_t sema)
{
#if USE_MACH_SEM
	semaphore_t s4 = (semaphore_t)sema;
	kern_return_t kr = semaphore_signal(s4);
	/*
	 *	Routine:	semaphore_signal_internal
	 *		Signals the semaphore as direct.
	 *	Assumptions:
	 *		Semaphore is locked.
	 */
	DISPATCH_SEMAPHORE_VERIFY_KR(kr);
//#elif USE_POSIX_SEM
//	int ret = sem_post((sem_t *)sema);
//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
//#elif USE_FUTEX_SEM
//	int ret = _dispatch_futex_signal((dispatch_futex_t *)sema);
//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
}
void
_dispatch_thread_semaphore_wait(_dispatch_thread_semaphore_t sema)
{
#if USE_MACH_SEM
	semaphore_t s4 = (semaphore_t)sema;
	kern_return_t kr;
	do {
		kr = semaphore_wait(s4);
//		*	Routine:	semaphore_wait_internal --》waitq_assert_wait64_locked
//		*		Decrements the semaphore count by one.  If the count is
//		*		negative after the decrement, the calling thread blocks
//		*		(possibly at a continuation and/or with a timeout).
	} while (slowpath(kr == KERN_ABORTED));
	DISPATCH_SEMAPHORE_VERIFY_KR(kr);
//#elif USE_POSIX_SEM
//	int ret;
//	do {
//		ret = sem_wait((sem_t *) sema);
//	} while (slowpath(ret == -1 && errno == EINTR));
//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
//#elif USE_FUTEX_SEM
//	int ret;
//	do {
//		ret = _dispatch_futex_wait((dispatch_futex_t *)sema, NULL);
//	} while (slowpath(ret == -1 && errno == EINTR));
//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
}

_dispatch_thread_semaphore_t
_dispatch_get_thread_semaphore(void)
{//https://linux.die.net/man/3/pthread_setspecific
	_dispatch_thread_semaphore_t sema = (_dispatch_thread_semaphore_t)
	_dispatch_thread_getspecific(dispatch_sema4_key);
	if (slowpath(!sema)) {
		return _dispatch_thread_semaphore_create();
	}
	_dispatch_thread_setspecific(dispatch_sema4_key, NULL);
	return sema;
}
void
_dispatch_put_thread_semaphore(_dispatch_thread_semaphore_t sema)
{
	_dispatch_thread_semaphore_t old_sema = (_dispatch_thread_semaphore_t)
	_dispatch_thread_getspecific(dispatch_sema4_key);
	_dispatch_thread_setspecific(dispatch_sema4_key, (void*)sema);
	if (slowpath(old_sema)) {
		return _dispatch_thread_semaphore_dispose(old_sema);
	}
}
//The pthread_setspecific() function shall associate a thread-specific value
//with a key obtained via a previous call to pthread_key_create().

