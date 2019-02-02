
#include "semaphore_wait.h"

DISPATCH_NOINLINE
static long
_dispatch_semaphore_wait_slow(dispatch_semaphore_t dsema,
							  dispatch_time_t timeout)
{
	long orig;
	
#if USE_MACH_SEM
	mach_timespec_t _timeout;
	kern_return_t kr;
#elif USE_POSIX_SEM || USE_FUTEX_SEM
	struct timespec _timeout;
	int ret;
#endif
	
#if USE_MACH_SEM
again:
	// Mach semaphores appear to sometimes spuriously wake up. Therefore,
	// we keep a parallel count of the number of times a Mach semaphore is
	// signaled (6880961).
	while ((orig = dsema->dsema_sent_ksignals)) {
		if (dispatch_atomic_cmpxchg2o(dsema, dsema_sent_ksignals,
									  orig,  orig - 1)) {
			return 0;
		}
	}
#endif
	
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
	
	switch (timeout) {
		default:
#if USE_MACH_SEM
			do {
				uint64_t nsec = _dispatch_timeout(timeout);
				_timeout.tv_sec = (typeof(_timeout.tv_sec))(nsec / NSEC_PER_SEC);
				_timeout.tv_nsec = (typeof(_timeout.tv_nsec))(nsec % NSEC_PER_SEC);
				kr = slowpath(semaphore_timedwait(dsema->dsema_port, _timeout));
			} while (kr == KERN_ABORTED);
			
			if (kr != KERN_OPERATION_TIMED_OUT) {
				DISPATCH_SEMAPHORE_VERIFY_KR(kr);
				break;
			}
#endif
			// Fall through and try to undo what the fast path did to
			// dsema->dsema_value
		case DISPATCH_TIME_NOW:
			while ((orig = dsema->dsema_value) < 0) {
				if (dispatch_atomic_cmpxchg2o(dsema, dsema_value, orig, orig + 1)) {
#if USE_MACH_SEM
					return KERN_OPERATION_TIMED_OUT;
#endif
				}
			}
			// Another thread called semaphore_signal().
			// Fall through and drain the wakeup.
		case DISPATCH_TIME_FOREVER:
#if USE_MACH_SEM
			do {
				kr = semaphore_wait(dsema->dsema_port);
			} while (kr == KERN_ABORTED);
			DISPATCH_SEMAPHORE_VERIFY_KR(kr);
#endif
			break;
	}
	
#if USE_MACH_SEM
	goto again;
#else
	return 0;
#endif
}

long
dispatch_semaphore_wait(dispatch_semaphore_t dsema, dispatch_time_t timeout)
{
	long value = dispatch_atomic_dec2o(dsema, dsema_value);
	dispatch_atomic_acquire_barrier();
	if (fastpath(value >= 0)) {
		return 0;
	}
	return _dispatch_semaphore_wait_slow(dsema, timeout);
}


