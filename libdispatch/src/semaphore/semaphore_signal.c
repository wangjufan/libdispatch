
#include "semaphore_signal.h"


DISPATCH_NOINLINE
long
_dispatch_semaphore_signal_slow(dispatch_semaphore_t dsema)
{
	// Before dsema_sent_ksignals is incremented we can rely on the reference
	// held by the waiter. However, once this value is incremented the waiter
	// may return between the atomic increment and the semaphore_signal(),
	// therefore an explicit reference must be held in order to safely access
	// dsema after the atomic increment.
	_dispatch_retain(dsema);
	
#if USE_MACH_SEM
	(void)dispatch_atomic_inc2o(dsema, dsema_sent_ksignals);
#endif
	
#if USE_MACH_SEM
	_dispatch_semaphore_create_port(&dsema->dsema_port);
	kern_return_t kr = semaphore_signal(dsema->dsema_port);
	DISPATCH_SEMAPHORE_VERIFY_KR(kr);
	//#elif USE_POSIX_SEM
	//	// POSIX semaphore is created in _dispatch_semaphore_init, not lazily.
	//	int ret = sem_post(&dsema->dsema_sem);
	//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
	//#elif USE_FUTEX_SEM
	//	// dispatch_futex_t is created in _dispatch_semaphore_init, not lazily.
	//	int ret = _dispatch_futex_signal(&dsema->dsema_futex);
	//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
	
	_dispatch_release(dsema);
	return 1;
}
long
dispatch_semaphore_signal(dispatch_semaphore_t dsema)
{
	dispatch_atomic_release_barrier();
	long value = dispatch_atomic_inc2o(dsema, dsema_value);
	if (fastpath(value > 0)) {
		return 0;
	}
	if (slowpath(value == LONG_MIN)) {
		//#define LONG_MIN  (-__LONG_MAX__ -1L)
		//#define LONG_MAX  __LONG_MAX__
		DISPATCH_CLIENT_CRASH("Unbalanced call to dispatch_semaphore_signal()");
	}
	return _dispatch_semaphore_signal_slow(dsema);
}


