//
//  barrier_sync.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "barrier_sync.h"

#pragma mark -
#pragma mark dispatch_barrier_sync

struct dispatch_barrier_sync_slow_s {
	DISPATCH_CONTINUATION_HEADER(barrier_sync_slow);
};

struct dispatch_barrier_sync_slow2_s {
	dispatch_queue_t dbss2_dq;
#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
	dispatch_function_t dbss2_func;
	void *dbss2_ctxt;
#endif
	_dispatch_thread_semaphore_t dbss2_sema;
};

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
DISPATCH_ALWAYS_INLINE_NDEBUG
static inline _dispatch_thread_semaphore_t
_dispatch_barrier_sync_f_pop(dispatch_queue_t dq,
							 dispatch_object_t dou,
							 bool lock)
{
	dispatch_continuation_t dc = dou._dc;
	
	if (DISPATCH_OBJ_IS_VTABLE(dc)
		|| ((long)dc->do_vtable &(DISPATCH_OBJ_BARRIER_BIT |
								  DISPATCH_OBJ_SYNC_SLOW_BIT)
			) != (DISPATCH_OBJ_BARRIER_BIT | DISPATCH_OBJ_SYNC_SLOW_BIT)
		) {
		return 0;
	}
	_dispatch_trace_continuation_pop(dq, dc);
	_dispatch_workitem_inc();
	
	struct dispatch_barrier_sync_slow_s *dbssp = (void *)dc;
	struct dispatch_barrier_sync_slow2_s *dbss2 = dbssp->dc_ctxt;
	if (lock) {
		(void)dispatch_atomic_add2o(dbss2->dbss2_dq, do_suspend_cnt,
									DISPATCH_OBJECT_SUSPEND_INTERVAL);
		// rdar://problem/9032024 running lock must be held until sync_f_slow
		// returns
		(void)dispatch_atomic_add2o(dbss2->dbss2_dq, dq_running, 2);
	}
	return dbss2->dbss2_sema ? dbss2->dbss2_sema :
#if HAVE_MACH
	MACH_PORT_DEAD;
#else
	(~0u);   /* The same value as MACH_PORT_DEAD */
#endif
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
static void
_dispatch_barrier_sync_f_slow_invoke(void *ctxt)
{
	struct dispatch_barrier_sync_slow2_s *dbss2 = ctxt;
	
	dispatch_assert(dbss2->dbss2_dq == _dispatch_queue_get_current());
#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
	// When the main queue is bound to the main thread
	if (dbss2->dbss2_dq == &_dispatch_main_q && pthread_main_np()) {
		dbss2->dbss2_func(dbss2->dbss2_ctxt);
		dbss2->dbss2_func = NULL;
		dispatch_atomic_barrier();
		_dispatch_thread_semaphore_signal(dbss2->dbss2_sema);
		return;
	}
#endif
	(void)dispatch_atomic_add2o(dbss2->dbss2_dq, do_suspend_cnt,
								DISPATCH_OBJECT_SUSPEND_INTERVAL);
	// rdar://9032024 running lock must be held until sync_f_slow returns
	(void)dispatch_atomic_add2o(dbss2->dbss2_dq, dq_running, 2);
	dispatch_atomic_barrier();
	_dispatch_thread_semaphore_signal(dbss2->dbss2_sema);
}
DISPATCH_NOINLINE
static void
_dispatch_barrier_sync_f_slow(dispatch_queue_t dq, void *ctxt,
							  dispatch_function_t func)
{
	// It's preferred to execute synchronous blocks on the current thread
	// due to thread-local side effects, garbage collection, etc. However,
	// blocks submitted to the main thread MUST be run on the main thread
	
	struct dispatch_barrier_sync_slow2_s dbss2 = {
		.dbss2_dq = dq,
#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
		.dbss2_func = func,
		.dbss2_ctxt = ctxt,
#endif
		.dbss2_sema = _dispatch_get_thread_semaphore(),
	};
	struct dispatch_barrier_sync_slow_s dbss = {
		.do_vtable = (void *)(DISPATCH_OBJ_BARRIER_BIT |
							  DISPATCH_OBJ_SYNC_SLOW_BIT),
		.dc_func = _dispatch_barrier_sync_f_slow_invoke,
		.dc_ctxt = &dbss2,
	};
	_dispatch_queue_push(dq, (void *)&dbss);
	
	_dispatch_thread_semaphore_wait(dbss2.dbss2_sema);
	_dispatch_put_thread_semaphore(dbss2.dbss2_sema);
	
#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
	// Main queue bound to main thread
	if (dbss2.dbss2_func == NULL) {
		return;
	}
#endif
	dispatch_atomic_acquire_barrier();
	if (slowpath(dq->do_targetq) && slowpath(dq->do_targetq->do_targetq)) {
		_dispatch_function_recurse(dq, ctxt, func);
	} else {
		_dispatch_function_invoke(dq, ctxt, func);
	}
	dispatch_atomic_release_barrier();
	if (fastpath(dq->do_suspend_cnt < 2 * DISPATCH_OBJECT_SUSPEND_INTERVAL) &&
		dq->dq_running == 2) {
		// rdar://problem/8290662 "lock transfer"
		_dispatch_thread_semaphore_t sema;
		sema = _dispatch_queue_drain_one_barrier_sync(dq);
		if (sema) {
			_dispatch_thread_semaphore_signal(sema);
			return;
		}
	}
	(void)dispatch_atomic_sub2o(dq, do_suspend_cnt,
								DISPATCH_OBJECT_SUSPEND_INTERVAL);
	if (slowpath(dispatch_atomic_sub2o(dq, dq_running, 2) == 0)) {
		_dispatch_wakeup(dq);
	}
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
DISPATCH_NOINLINE
static void
_dispatch_barrier_sync_f2(dispatch_queue_t dq)
{
	if (!slowpath(DISPATCH_OBJECT_SUSPENDED(dq))) {
		// rdar://problem/8290662 "lock transfer"
		_dispatch_thread_semaphore_t sema;
		sema = _dispatch_queue_drain_one_barrier_sync(dq);
		if (sema) {
			(void)dispatch_atomic_add2o(dq, do_suspend_cnt,
										DISPATCH_OBJECT_SUSPEND_INTERVAL);
			// rdar://9032024 running lock must be held until sync_f_slow
			// returns: increment by 2 and decrement by 1
			(void)dispatch_atomic_inc2o(dq, dq_running);
			_dispatch_thread_semaphore_signal(sema);
			return;
		}
	}
	if (slowpath(dispatch_atomic_dec2o(dq, dq_running) == 0)) {
		_dispatch_wakeup(dq);
	}
}
DISPATCH_NOINLINE
static void
_dispatch_barrier_sync_f_invoke(dispatch_queue_t dq, void *ctxt,
								dispatch_function_t func)
{
	dispatch_atomic_acquire_barrier();
	_dispatch_function_invoke(dq, ctxt, func);////////////////////////
	dispatch_atomic_release_barrier();
	if (slowpath(dq->dq_items_tail)) {
		return _dispatch_barrier_sync_f2(dq);//////////////////////
	}
	if (slowpath(dispatch_atomic_dec2o(dq, dq_running) == 0)) {
		_dispatch_wakeup(dq);//////////////////////
	}
}
DISPATCH_NOINLINE
static void
_dispatch_barrier_sync_f_recurse(dispatch_queue_t dq, void *ctxt,
								 dispatch_function_t func)
{
	dispatch_atomic_acquire_barrier();
	_dispatch_function_recurse(dq, ctxt, func);////////////// remove barrier wjf to dispatch_asyn 
	dispatch_atomic_release_barrier();
	if (slowpath(dq->dq_items_tail)) {
		return _dispatch_barrier_sync_f2(dq);////////////
	}
	if (slowpath(dispatch_atomic_dec2o(dq, dq_running) == 0)) {
		_dispatch_wakeup(dq);////////////
	}
}


DISPATCH_NOINLINE
void
dispatch_barrier_sync_f(dispatch_queue_t dq, void *ctxt,
						dispatch_function_t func)
{
	// 1) ensure that this thread hasn't enqueued anything ahead of this call
	// 2) the queue is not suspended
	if (slowpath(dq->dq_items_tail) || slowpath(DISPATCH_OBJECT_SUSPENDED(dq))){
		return _dispatch_barrier_sync_f_slow(dq, ctxt, func);
	}
	if (slowpath(!dispatch_atomic_cmpxchg2o(dq, dq_running, 0, 1))) {
		// global queues and main queue bound to main thread always falls into
		// the slow case
//		dispatch_atomic_cmpxchg(&(p)->f, (e), (n))
		return _dispatch_barrier_sync_f_slow(dq, ctxt, func);
	}
	if (slowpath(dq->do_targetq->do_targetq)) {
		return _dispatch_barrier_sync_f_recurse(dq, ctxt, func);
	}
	_dispatch_barrier_sync_f_invoke(dq, ctxt, func);
}
#ifdef __BLOCKS__
#if DISPATCH_COCOA_COMPAT
DISPATCH_NOINLINE
static void
_dispatch_barrier_sync_slow(dispatch_queue_t dq, void (^work)(void))
{
	// Blocks submitted to the main queue MUST be run on the main thread,
	// therefore under GC we must Block_copy in order to notify the thread-local
	// garbage collector that the objects are transferring to the main thread
	// rdar://problem/7176237&7181849&7458685
//	if (dispatch_begin_thread_4GC) {
//		dispatch_block_t block = _dispatch_Block_copy(work);
//		return dispatch_barrier_sync_f(dq, block,
//									   _dispatch_call_block_and_release);
//	}
	struct Block_basic *bb = (void *)work;
	dispatch_barrier_sync_f(dq, work, (dispatch_function_t)bb->Block_invoke);
}
#endif


void
dispatch_barrier_sync(dispatch_queue_t dq, void (^work)(void))
{
#if DISPATCH_COCOA_COMPAT
	if (slowpath(dq == &_dispatch_main_q)) {
		return _dispatch_barrier_sync_slow(dq, work);
	}
#endif
	struct Block_basic *bb = (void *)work;
	dispatch_barrier_sync_f(dq, work, (dispatch_function_t)bb->Block_invoke);
}
#endif
