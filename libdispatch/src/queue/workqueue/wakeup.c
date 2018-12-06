//
//  wakeup.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "wakeup.h"

#pragma mark -
#pragma mark dispatch_wakeup

// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
dispatch_queue_t
_dispatch_wakeup(dispatch_object_t dou)
{
	dispatch_queue_t tq;
	
	if (slowpath(DISPATCH_OBJECT_SUSPENDED(dou._do))) {
		return NULL;
	}
	if (!dx_probe(dou._do) && !dou._dq->dq_items_tail) {
		return NULL;
	}
	
	// _dispatch_source_invoke() relies on this testing the whole suspend count
	// word, not just the lock bit. In other words, no point taking the lock
	// if the source is suspended or canceled.
	if (!dispatch_atomic_cmpxchg2o(dou._do, do_suspend_cnt,
								   0, DISPATCH_OBJECT_SUSPEND_LOCK)) {
#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
		if (dou._dq == &_dispatch_main_q) {
			return _dispatch_queue_wakeup_main();
		}
#endif
		return NULL;
	}
	dispatch_atomic_acquire_barrier();
	_dispatch_retain(dou._do);
	tq = dou._do->do_targetq;
	_dispatch_queue_push(tq, dou._do);//wake up slowly wjf
	return tq;	// libdispatch does not need this, but the Instrument DTrace
	// probe does
}


#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
DISPATCH_NOINLINE
dispatch_queue_t
_dispatch_queue_wakeup_main(void)
{
#if DISPATCH_COCOA_COMPAT
	kern_return_t kr;
	
	dispatch_once_f(&_dispatch_main_q_port_pred, NULL,
					_dispatch_main_q_port_init);
	if (main_q_port) {
		kr = _dispatch_send_wakeup_main_thread(main_q_port, 0);//wjf no codes
		switch (kr) {
			case MACH_SEND_TIMEOUT:
			case MACH_SEND_TIMED_OUT:
			case MACH_SEND_INVALID_DEST:
				break;
			default:
				(void)dispatch_assume_zero(kr);
				break;
		}
	}
#endif
//#if DISPATCH_LINUX_COMPAT
//	dispatch_once_f(&_dispatch_main_q_eventfd_pred, NULL,
//					_dispatch_main_q_eventfd_init);
//	if (main_q_eventfd != -1) {
//		_dispatch_eventfd_write(main_q_eventfd, 1);
//	}
//#endif
	return NULL;
}
#endif  // DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT

DISPATCH_NOINLINE
static void
_dispatch_queue_wakeup_global_slow(dispatch_queue_t dq, unsigned int n)
{
	static dispatch_once_t pred;
	struct dispatch_root_queue_context_s *qc = dq->do_ctxt;
	int r;
	
	dispatch_debug_queue(dq, __func__);
	dispatch_once_f(&pred, NULL, _dispatch_root_queues_init);
	
#if HAVE_PTHREAD_WORKQUEUES
#if DISPATCH_USE_PTHREAD_POOL
	if (qc->dgq_kworkqueue != (void*)(~0ul))
#endif
	{
		_dispatch_debug("requesting new worker thread");
#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
		if (qc->dgq_kworkqueue) {
			pthread_workitem_handle_t wh;
			unsigned int gen_cnt, i = n;
			do {
				r = pthread_workqueue_additem_np(qc->dgq_kworkqueue,
												 _dispatch_worker_thread3, dq, &wh, &gen_cnt);
				(void)dispatch_assume_zero(r);
			} while (--i);
			return;
		}
#endif // DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
		
#if HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
		r = pthread_workqueue_addthreads_np(qc->dgq_wq_priority,
											qc->dgq_wq_options, n);
		(void)dispatch_assume_zero(r);
#endif
		return;
	}
#endif // HAVE_PTHREAD_WORKQUEUES
	
#if DISPATCH_USE_PTHREAD_POOL
	if (dispatch_semaphore_signal(qc->dgq_thread_mediator)) {
		return;
	}
	
	pthread_t pthr;
	int t_count;
	do {
		t_count = qc->dgq_thread_pool_size;
		if (!t_count) {
			_dispatch_debug("The thread pool is full: %p", dq);
			return;
		}
	} while (!dispatch_atomic_cmpxchg2o(qc, dgq_thread_pool_size, t_count,
										t_count - 1));
	
	while ((r = pthread_create(&pthr, NULL, _dispatch_worker_thread, dq))) {
		if (r != EAGAIN) {// to be wait if not signaled , than called back to this function
			(void)dispatch_assume_zero(r);
		}
		sleep(1);
	}
	r = pthread_detach(pthr);//
	(void)dispatch_assume_zero(r);
#endif // DISPATCH_USE_PTHREAD_POOL
	
}


static inline void
_dispatch_queue_wakeup_global2(dispatch_queue_t dq, unsigned int n)
{
	struct dispatch_root_queue_context_s *qc = dq->do_ctxt;
	
	if (!dq->dq_items_tail) {
		return;
	}
#if HAVE_PTHREAD_WORKQUEUES
	if (
#if DISPATCH_USE_PTHREAD_POOL
		(qc->dgq_kworkqueue != (void*)(~0ul)) &&
#endif
		!dispatch_atomic_cmpxchg2o(qc, dgq_pending, 0, n)) {
		_dispatch_debug("work thread request still pending on global queue: "
						"%p", dq);
		return;
	}
#endif // HAVE_PTHREAD_WORKQUEUES
	return 	_dispatch_queue_wakeup_global_slow(dq, n);
}

static inline void
_dispatch_queue_wakeup_global(dispatch_queue_t dq)
{
	return _dispatch_queue_wakeup_global2(dq, 1);
}
bool
_dispatch_queue_probe_root(dispatch_queue_t dq)
{
	_dispatch_queue_wakeup_global2(dq, 1);
	return false;
}
