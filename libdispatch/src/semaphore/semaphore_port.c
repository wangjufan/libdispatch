#include "semaphore_port.h"

#if USE_MACH_SEM
static void
_dispatch_semaphore_create_port(semaphore_t *s4)
{
	kern_return_t kr;
	semaphore_t tmp;//kernel object
	
	if (*s4) {
		return;
	}
	_dispatch_safe_fork = false;
	
	// lazily allocate the semaphore port
	
	// Someday:
	// 1) Switch to a doubly-linked FIFO in user-space.
	// 2) User-space timers for the timeout.
	// 3) Use the per-thread semaphore port.
	while ((kr = semaphore_create(mach_task_self(), &tmp,
								  SYNC_POLICY_FIFO, 0))) {//success == 0
		DISPATCH_VERIFY_MIG(kr);
		sleep(1);
	}
	
	if (!dispatch_atomic_cmpxchg(s4, 0, tmp)) {
		kr = semaphore_destroy(mach_task_self(), tmp);
		DISPATCH_SEMAPHORE_VERIFY_KR(kr);
	}
}
#endif

