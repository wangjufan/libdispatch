//
//  worker_thread.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "worker_thread.h"

#pragma mark -
#pragma mark dispatch_worker_thread

static void
_dispatch_worker_thread4(dispatch_queue_t dq)
{
	struct dispatch_object_s *item;
	
#if DISPATCH_DEBUG
	if (_dispatch_thread_getspecific(dispatch_queue_key)) {
		DISPATCH_CRASH("Premature thread recycling");
	}
#endif
	_dispatch_thread_setspecific(dispatch_queue_key, dq);
	
#if DISPATCH_COCOA_COMPAT
	(void)dispatch_atomic_inc(&_dispatch_worker_threads);
	// ensure that high-level memory management techniques do not leak/crash
	if (dispatch_begin_thread_4GC) {
		dispatch_begin_thread_4GC();
	}
	void *pool = _dispatch_autorelease_pool_push();
#endif // DISPATCH_COCOA_COMPAT
	
#if DISPATCH_PERF_MON
	uint64_t start = _dispatch_absolute_time();
#endif
	
	while ((item = fastpath(_dispatch_queue_concurrent_drain_one(dq)))) {
		_dispatch_continuation_pop(item);
	}
	
#if DISPATCH_PERF_MON
	_dispatch_queue_merge_stats(start);
#endif
	
#if DISPATCH_COCOA_COMPAT
	_dispatch_autorelease_pool_pop(pool);
	if (dispatch_end_thread_4GC) {
		dispatch_end_thread_4GC();
	}
	if (!dispatch_atomic_dec(&_dispatch_worker_threads) &&
		dispatch_no_worker_threads_4GC) {
		dispatch_no_worker_threads_4GC();
	}
#endif // DISPATCH_COCOA_COMPAT
	
	_dispatch_thread_setspecific(dispatch_queue_key, NULL);
	
	_dispatch_force_cache_cleanup();
	
}

#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
static void
_dispatch_worker_thread3(void *context)
{
	dispatch_queue_t dq = context;
	struct dispatch_root_queue_context_s *qc = dq->do_ctxt;
	
	(void)dispatch_atomic_dec2o(qc, dgq_pending);
	_dispatch_worker_thread4(dq);
}
#endif

//#if HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
//// 6618342 Contact the team that owns the Instrument DTrace probe before
////         renaming this symbol
//static void
//_dispatch_worker_thread2(int priority, int options,
//						 void *context DISPATCH_UNUSED)
//{
//	dispatch_assert(priority >= 0 && priority < WORKQ_NUM_PRIOQUEUE);
//	dispatch_assert(!(options & ~DISPATCH_WORKQ_OPTION_OVERCOMMIT));
//	dispatch_queue_t dq = _dispatch_wq2root_queues[priority][options];
//	struct dispatch_root_queue_context_s *qc = dq->do_ctxt;
//
//	(void)dispatch_atomic_dec2o(qc, dgq_pending);
//	_dispatch_worker_thread4(dq);
//}
//#endif

#if DISPATCH_USE_PTHREAD_POOL
// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
static void *
_dispatch_worker_thread(void *context)
{
	dispatch_queue_t dq = context;
	struct dispatch_root_queue_context_s *qc = dq->do_ctxt;
	sigset_t mask;
	int r;
	
	// workaround tweaks the kernel workqueue does for us
	r = sigfillset(&mask);
	(void)dispatch_assume_zero(r);
	r = _dispatch_pthread_sigmask(SIG_BLOCK, &mask, NULL);
	(void)dispatch_assume_zero(r);
	
	do {
		_dispatch_worker_thread4(dq);
		// we use 65 seconds in case there are any timers that run once a minute
	} while (dispatch_semaphore_wait(
							qc->dgq_thread_mediator,
							dispatch_time(0, 65ull * NSEC_PER_SEC))
			 == 0);
	
	(void)dispatch_atomic_inc2o(qc, dgq_thread_pool_size);
	if (dq->dq_items_tail) {
		_dispatch_queue_wakeup_global(dq);
	}
	
	return NULL;
}
int
_dispatch_pthread_sigmask(int how, sigset_t *set, sigset_t *oset)
{
	int r;
	/* Workaround: 6269619 Not all signals can be delivered on any thread */
	r = sigdelset(set, SIGILL);
	(void)dispatch_assume_zero(r);
	r = sigdelset(set, SIGTRAP);
	(void)dispatch_assume_zero(r);
#if HAVE_DECL_SIGEMT
	r = sigdelset(set, SIGEMT);
	(void)dispatch_assume_zero(r);
#endif
	r = sigdelset(set, SIGFPE);
	(void)dispatch_assume_zero(r);
	r = sigdelset(set, SIGBUS);
	(void)dispatch_assume_zero(r);
	r = sigdelset(set, SIGSEGV);
	(void)dispatch_assume_zero(r);
	r = sigdelset(set, SIGSYS);
	(void)dispatch_assume_zero(r);
	r = sigdelset(set, SIGPIPE);
	(void)dispatch_assume_zero(r);
	
	return pthread_sigmask(how, set, oset);
}
#endif
