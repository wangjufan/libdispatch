#include <stdio.h>


DISPATCH_NOINLINE
static long
_dispatch_group_wake(dispatch_semaphore_t dsema)
{
	struct dispatch_sema_notify_s *next, *head, *tail = NULL;
	long rval;
	
	head = dispatch_atomic_xchg2o(dsema, dsema_notify_head, NULL);
	if (head) {
		// snapshot before anything is notified/woken <rdar://problem/8554546>
		tail = dispatch_atomic_xchg2o(dsema, dsema_notify_tail, NULL);
	}
	rval = dispatch_atomic_xchg2o(dsema, dsema_group_waiters, 0);
	//int old_value = __sync_swap(&value, new_value);
	if (rval) {
		// wake group waiters
#if USE_MACH_SEM
		_dispatch_semaphore_create_port(&dsema->dsema_waiter_port);
		do {//notify n times wjf
			kern_return_t kr = semaphore_signal(dsema->dsema_waiter_port);
			DISPATCH_SEMAPHORE_VERIFY_KR(kr);
		} while (--rval);
//#elif USE_POSIX_SEM
//		do {
//			int ret = sem_post(&dsema->dsema_sem);
//			DISPATCH_SEMAPHORE_VERIFY_RET(ret);
//		} while (--rval);
//#elif USE_FUTEX_SEM
//		do {
//			int ret = _dispatch_futex_signal(&dsema->dsema_futex);
//			DISPATCH_SEMAPHORE_VERIFY_RET(ret);
//		} while (--rval);
#endif
	}
	if (head) {
		// async group notify blocks
		do {
			dispatch_async_f(head->dsn_queue, head->dsn_ctxt, head->dsn_func);
			_dispatch_release(head->dsn_queue);
			next = fastpath(head->dsn_next);
			if (!next && head != tail) {
				while (!(next = fastpath(head->dsn_next))) {/////////组同步机制
					_dispatch_hardware_pause();
				}
			}
			free(head);
		} while ((head = next));
		_dispatch_release(dsema);
	}
	return 0;
}

