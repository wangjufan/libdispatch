#include "semaphore_group.h"

#pragma mark -
#pragma mark dispatch_group_t


long
dispatch_group_wait(dispatch_group_t dg, dispatch_time_t timeout)
{
	dispatch_semaphore_t dsema = (dispatch_semaphore_t)dg;
	if (dsema->dsema_value == dsema->dsema_orig) {
		return 0;
	}
	if (timeout == 0) {
#if USE_MACH_SEM
		return KERN_OPERATION_TIMED_OUT;
#elif USE_POSIX_SEM || USE_FUTEX_SEM
		errno = ETIMEDOUT;
		return (-1);
#endif
	}
	return _dispatch_group_wait_slow(dsema, timeout);
}

DISPATCH_NOINLINE
static long
_dispatch_group_wait_slow(dispatch_semaphore_t dsema,
						  dispatch_time_t timeout)
{
	long orig;
	
#if USE_MACH_SEM
	mach_timespec_t _timeout;
	kern_return_t kr;
//#elif USE_POSIX_SEM || USE_FUTEX_SEM // KVV
//	struct timespec _timeout;
//	int ret;
#endif
	
again:
	// check before we cause another signal to be sent by incrementing
	// dsema->dsema_group_waiters
	if (dsema->dsema_value == dsema->dsema_orig) {
		return _dispatch_group_wake(dsema);
	}
	// Mach semaphores appear to sometimes spuriously wake up. Therefore,
	// we keep a parallel count of the number of times a Mach semaphore is
	// signaled (6880961).
	//	wjf semaphores spuriously wake up
	//	Even after a condition variable appears to have been signaled from a waiting thread’s point of view, the condition that was awaited may still be false.
	//
	//	Simply, it means a thread can wakeup from its waiting state without being signaled or interrupted or timing out. To make things correct, awakened thread has to verify the condition that should have caused the thread to be awakened. And it must continue waiting if the condition is not satisfied.
	(void)dispatch_atomic_inc2o(dsema, dsema_group_waiters);
	// check the values again in case we need to wake any threads
	if (dsema->dsema_value == dsema->dsema_orig) {
		return _dispatch_group_wake(dsema);/////唤醒某个 等待者 wjf
	}
	
#if USE_MACH_SEM
	_dispatch_semaphore_create_port(&dsema->dsema_port);
#elif USE_POSIX_SEM || USE_FUTEX_SEM
	// Created in _dispatch_semaphore_init.
#endif
	
	// From xnu/osfmk/kern/sync_sema.c:
	// wait_semaphore->count = -1; /* we don't keep an actual count */
	//
	// The code above does not match the documentation, and that fact is
	// not surprising. The documented semantics are clumsy to use in any
	// practical way. The above hack effectively tricks the rest of the
	// Mach semaphore logic to behave like the libdispatch algorithm.
	
	switch (timeout) {[uii]
		default:
#if USE_MACH_SEM
			do {
				uint64_t nsec = _dispatch_timeout(timeout);
				_timeout.tv_sec = (typeof(_timeout.tv_sec))(nsec / NSEC_PER_SEC);
				_timeout.tv_nsec = (typeof(_timeout.tv_nsec))(nsec % NSEC_PER_SEC);
				kr = slowpath(semaphore_timedwait(dsema->dsema_waiter_port,
												  _timeout));
			} while (kr == KERN_ABORTED);
			
			if (kr != KERN_OPERATION_TIMED_OUT) {
				DISPATCH_SEMAPHORE_VERIFY_KR(kr);
				break;
			}
			//#elif USE_POSIX_SEM
			//			do {
			//				_timeout = _dispatch_timeout_ts(timeout);
			//				ret = slowpath(sem_timedwait(&dsema->dsema_sem, &_timeout));
			//			} while (ret == -1 && errno == EINTR);
			//
			//			if (!(ret == -1 && errno == ETIMEDOUT)) {
			//				DISPATCH_SEMAPHORE_VERIFY_RET(ret);
			//				break;
			//			}
			//#elif USE_FUTEX_SEM
			//			do {
			//				uint64_t nsec = _dispatch_timeout(timeout);
			//				_timeout.tv_sec = nsec / NSEC_PER_SEC;
			//				_timeout.tv_nsec = nsec % NSEC_PER_SEC;
			//				ret = slowpath(
			//							   _dispatch_futex_wait(&dsema->dsema_futex, &_timeout));
			//			} while (ret == -1 && errno == EINTR);
			//
			//			if (!(ret == -1 && errno == ETIMEDOUT)) {
			//				DISPATCH_SEMAPHORE_VERIFY_RET(ret);
			//				break;
			//			}
#endif
			// Fall through and try to undo the earlier change to
			// dsema->dsema_group_waiters
		case DISPATCH_TIME_NOW:
			while ((orig = dsema->dsema_group_waiters)) {
				if (dispatch_atomic_cmpxchg2o(dsema,
											  dsema_group_waiters,
											  orig,
											  orig - 1)) {
#if USE_MACH_SEM
					return KERN_OPERATION_TIMED_OUT;
					//#elif USE_POSIX_SEM || USE_FUTEX_SEM
					//					errno = ETIMEDOUT;
					//					return -1;
#endif
				}
			}
			// Another thread called semaphore_signal().
			// Fall through and drain the wakeup.
		case DISPATCH_TIME_FOREVER:
#if USE_MACH_SEM
			do {
				kr = semaphore_wait(dsema->dsema_waiter_port);
			} while (kr == KERN_ABORTED);
			DISPATCH_SEMAPHORE_VERIFY_KR(kr);
			//#elif USE_POSIX_SEM
			//			do {
			//				ret = sem_wait(&dsema->dsema_sem);
			//			} while (ret == -1 && errno == EINTR);
			//			DISPATCH_SEMAPHORE_VERIFY_RET(ret);
			//#elif USE_FUTEX_SEM
			//			do {
			//				ret = _dispatch_futex_wait(&dsema->dsema_futex, NULL);
			//			} while (ret == -1 && errno == EINTR);
			//			DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
			break;
	}
	
	goto again;
}

