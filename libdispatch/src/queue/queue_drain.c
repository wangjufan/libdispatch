#include "queue_drain.h"

#pragma mark -
#pragma mark dispatch_queue_drain

// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
DISPATCH_NOINLINE
void
_dispatch_queue_invoke(dispatch_queue_t dq)
{
	if (!slowpath(DISPATCH_OBJECT_SUSPENDED(dq)) &&
		fastpath(dispatch_atomic_cmpxchg2o(dq, dq_running, 0, 1))) {
		dispatch_atomic_acquire_barrier();
		dispatch_queue_t otq = dq->do_targetq, tq = NULL;
		_dispatch_thread_semaphore_t sema = _dispatch_queue_drain(dq);
		if (dq->do_vtable->do_invoke) {
			// Assume that object invoke checks it is executing on correct queue
			tq = dx_invoke(dq);
		} else if (slowpath(otq != dq->do_targetq)) {
			// An item on the queue changed the target queue
			tq = dq->do_targetq;
		}
		// We do not need to check the result.
		// When the suspend-count lock is dropped, then the check will happen.
		dispatch_atomic_release_barrier();
		(void)dispatch_atomic_dec2o(dq, dq_running);
		if (sema) {
			_dispatch_thread_semaphore_signal(sema);
		} else if (tq) {
			return _dispatch_queue_push(tq, dq);
		}
	}
	
	dq->do_next = DISPATCH_OBJECT_LISTLESS;
	dispatch_atomic_release_barrier();
	if (!dispatch_atomic_sub2o(dq, do_suspend_cnt,
							   DISPATCH_OBJECT_SUSPEND_LOCK)) {
		if (dq->dq_running == 0) {
			_dispatch_wakeup(dq); // verify that the queue is idle
		}
	}
	_dispatch_release(dq); // added when the queue is put on the list
}

static _dispatch_thread_semaphore_t
_dispatch_queue_drain(dispatch_queue_t dq)
{
	dispatch_queue_t orig_tq, old_dq;
	old_dq = _dispatch_thread_getspecific(dispatch_queue_key);
	struct dispatch_object_s *dc = NULL, *next_dc = NULL;
	_dispatch_thread_semaphore_t sema = 0;
	
	// Continue draining sources after target queue change rdar://8928171
	bool check_tq = (dx_type(dq) != DISPATCH_SOURCE_KEVENT_TYPE);
	
	orig_tq = dq->do_targetq;
	
	_dispatch_thread_setspecific(dispatch_queue_key, dq);
	//dispatch_debug_queue(dq, __func__);
	
	while (dq->dq_items_tail) {
		while (!(dc = fastpath(dq->dq_items_head))) {
			_dispatch_hardware_pause();
		}
		dq->dq_items_head = NULL;
		do {
			next_dc = fastpath(dc->do_next);
			if (!next_dc &&
				!dispatch_atomic_cmpxchg2o(dq, dq_items_tail, dc, NULL)) {
				// Enqueue is TIGHTLY controlled, we won't wait long.
				while (!(next_dc = fastpath(dc->do_next))) {
					_dispatch_hardware_pause();
				}
			}
			if (DISPATCH_OBJECT_SUSPENDED(dq)) {
				goto out;
			}
			if (dq->dq_running > dq->dq_width) {
				goto out;
			}
			if (slowpath(orig_tq != dq->do_targetq) && check_tq) {
				goto out;
			}
			if (!fastpath(dq->dq_width == 1)) {
				if (!DISPATCH_OBJ_IS_VTABLE(dc) &&
					(long)dc->do_vtable & DISPATCH_OBJ_BARRIER_BIT) {
					if (dq->dq_running > 1) {
						goto out;
					}
				} else {
					_dispatch_continuation_redirect(dq, dc);
					continue;
				}
			}
			if ((sema = _dispatch_barrier_sync_f_pop(dq, dc, true))) {
				dc = next_dc;
				goto out;
			}
			_dispatch_continuation_pop(dc);
			_dispatch_workitem_inc();
		} while ((dc = next_dc));
	}
	
out:
	// if this is not a complete drain, we must undo some things
	if (slowpath(dc)) {
		// 'dc' must NOT be "popped"
		// 'dc' might be the last item
		if (!next_dc &&
			!dispatch_atomic_cmpxchg2o(dq, dq_items_tail, NULL, dc)) {
			// wait for enqueue slow path to finish
			while (!(next_dc = fastpath(dq->dq_items_head))) {
				_dispatch_hardware_pause();
			}
			dc->do_next = next_dc;
		}
		dq->dq_items_head = dc;
	}
	
	_dispatch_thread_setspecific(dispatch_queue_key, old_dq);
	return sema;
}

static void
_dispatch_queue_serial_drain_till_empty(dispatch_queue_t dq)
{
#if DISPATCH_PERF_MON
	uint64_t start = _dispatch_absolute_time();
#endif
	_dispatch_thread_semaphore_t sema = _dispatch_queue_drain(dq);
	if (sema) {
		dispatch_atomic_barrier();
		_dispatch_thread_semaphore_signal(sema);
	}
#if DISPATCH_PERF_MON
	_dispatch_queue_merge_stats(start);
#endif
	_dispatch_force_cache_cleanup();
}

#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
void
_dispatch_main_queue_drain(void) {
	dispatch_queue_t dq = &_dispatch_main_q;
	if (!dq->dq_items_tail) {
		return;
	}
//#if DISPATCH_LINUX_COMPAT
//	(void)_dispatch_eventfd_read(main_q_eventfd);
//#endif
	struct dispatch_main_queue_drain_marker_s {
		DISPATCH_CONTINUATION_HEADER(main_queue_drain_marker);
//#define DISPATCH_CONTINUATION_HEADER(x) \
//	_OS_OBJECT_HEADER( \
//	const void *do_vtable, \
//	do_ref_cnt, \
//	do_xref_cnt); \
//	struct dispatch_##x##_s *volatile do_next; \
//	dispatch_function_t dc_func; \
//	void *dc_ctxt; \
//	void *dc_data; \
//	void *dc_other;
	} marker = {
		.do_vtable = NULL,
	};
	struct dispatch_object_s *dmarker = (void*)&marker;
	_dispatch_queue_push_notrace(dq, dmarker);
	
#if DISPATCH_PERF_MON
	uint64_t start = _dispatch_absolute_time();
#endif
	dispatch_queue_t old_dq = _dispatch_thread_getspecific(dispatch_queue_key);
	_dispatch_thread_setspecific(dispatch_queue_key, dq);
	
	struct dispatch_object_s *dc = NULL, *next_dc = NULL;
	while (dq->dq_items_tail) {
		while (!(dc = fastpath(dq->dq_items_head))) {
			_dispatch_hardware_pause();
		}
		dq->dq_items_head = NULL;
		do {
			next_dc = fastpath(dc->do_next);
			if (!next_dc &&
				!dispatch_atomic_cmpxchg2o(dq, dq_items_tail, dc, NULL)) {
				// Enqueue is TIGHTLY controlled, we won't wait long.
				while (!(next_dc = fastpath(dc->do_next))) {
					_dispatch_hardware_pause();
				}
			}
			if (dc == dmarker) {
				if (next_dc) {
					dq->dq_items_head = next_dc;
					_dispatch_queue_wakeup_main();
				}
				goto out;
			}
			_dispatch_continuation_pop(dc);
			_dispatch_workitem_inc();
		} while ((dc = next_dc));
	}
	dispatch_assert(dc); // did not encounter marker
	
out:
	_dispatch_thread_setspecific(dispatch_queue_key, old_dq);
#if DISPATCH_PERF_MON
	_dispatch_queue_merge_stats(start);
#endif
	_dispatch_force_cache_cleanup();
}
#endif

DISPATCH_ALWAYS_INLINE_NDEBUG
static inline _dispatch_thread_semaphore_t
_dispatch_queue_drain_one_barrier_sync(dispatch_queue_t dq)
{
	// rdar://problem/8290662 "lock transfer"
	struct dispatch_object_s *dc, *next_dc;
	_dispatch_thread_semaphore_t sema;
	
	// queue is locked, or suspended and not being drained
	dc = dq->dq_items_head;
	if (slowpath(!dc) || !(sema = _dispatch_barrier_sync_f_pop(dq, dc, false))){
		return 0;
	}
	// dequeue dc, it is a barrier sync
	next_dc = fastpath(dc->do_next);
	dq->dq_items_head = next_dc;
	if (!next_dc && !dispatch_atomic_cmpxchg2o(dq, dq_items_tail, dc, NULL)) {
		// Enqueue is TIGHTLY controlled, we won't wait long.
		while (!(next_dc = fastpath(dc->do_next))) {
			_dispatch_hardware_pause();
		}
		dq->dq_items_head = next_dc;
	}
	return sema;
}

#ifndef DISPATCH_HEAD_CONTENTION_SPINS
#define DISPATCH_HEAD_CONTENTION_SPINS 10000
#endif

static struct dispatch_object_s *
_dispatch_queue_concurrent_drain_one(dispatch_queue_t dq)
{
	struct dispatch_object_s *head, *next, *const mediator = (void *)~0ul;
	
start:
	// The mediator value acts both as a "lock" and a signal
	head = dispatch_atomic_xchg2o(dq, dq_items_head, mediator);
	
	if (slowpath(head == NULL)) {
		// The first xchg on the tail will tell the enqueueing thread that it
		// is safe to blindly write out to the head pointer. A cmpxchg honors
		// the algorithm.
		(void)dispatch_atomic_cmpxchg2o(dq, dq_items_head, mediator, NULL);
		_dispatch_debug("no work on global work queue");
		return NULL;
	}
	
	if (slowpath(head == mediator)) {
		// This thread lost the race for ownership of the queue.
		// Spin for a short while in case many threads have started draining at
		// once as part of a dispatch_apply
		unsigned int i = DISPATCH_HEAD_CONTENTION_SPINS;
		do {
			_dispatch_hardware_pause();
			if (dq->dq_items_head != mediator) goto start;
		} while (--i);
		// The ratio of work to libdispatch overhead must be bad. This
		// scenario implies that there are too many threads in the pool.
		// Create a new pending thread and then exit this thread.
		// The kernel will grant a new thread when the load subsides.
		_dispatch_debug("Contention on queue: %p", dq);
		_dispatch_queue_wakeup_global(dq);
#if DISPATCH_PERF_MON
		dispatch_atomic_inc(&_dispatch_bad_ratio);
#endif
		return NULL;
	}
	
	// Restore the head pointer to a sane value before returning.
	// If 'next' is NULL, then this item _might_ be the last item.
	next = fastpath(head->do_next);
	
	if (slowpath(!next)) {
		dq->dq_items_head = NULL;
		
		if (dispatch_atomic_cmpxchg2o(dq, dq_items_tail, head, NULL)) {
			// both head and tail are NULL now
			goto out;
		}
		
		// There must be a next item now. This thread won't wait long.
		while (!(next = head->do_next)) {
			_dispatch_hardware_pause();
		}
	}
	
	dq->dq_items_head = next;
	_dispatch_queue_wakeup_global(dq);
out:
	return head;
}
