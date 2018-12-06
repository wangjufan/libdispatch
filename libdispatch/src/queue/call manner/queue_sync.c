//
//  queue_sync.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "queue_sync.h"

#pragma mark -
#pragma mark dispatch_sync

DISPATCH_NOINLINE
static void
_dispatch_sync_f_slow(dispatch_queue_t dq, void *ctxt, dispatch_function_t func)
{
	_dispatch_thread_semaphore_t sema = _dispatch_get_thread_semaphore();
	struct dispatch_sync_slow_s {
		DISPATCH_CONTINUATION_HEADER(sync_slow);
	} dss = {
		.do_vtable = (void*)DISPATCH_OBJ_SYNC_SLOW_BIT,
		.dc_ctxt = (void*)sema,
	};
	_dispatch_queue_push(dq, (void *)&dss);
	
	_dispatch_thread_semaphore_wait(sema);
	_dispatch_put_thread_semaphore(sema);
	
	if (slowpath(dq->do_targetq->do_targetq)) {
		_dispatch_function_recurse(dq, ctxt, func);
	} else {
		_dispatch_function_invoke(dq, ctxt, func);
	}
	if (slowpath(dispatch_atomic_sub2o(dq, dq_running, 2) == 0)) {
		_dispatch_wakeup(dq);// a new task is coming , try to wakeup some thread
	}
}

DISPATCH_NOINLINE
static void
_dispatch_sync_f_slow2(dispatch_queue_t dq, void *ctxt,
					   dispatch_function_t func)
{
	if (slowpath(dispatch_atomic_sub2o(dq, dq_running, 2) == 0)) {
		_dispatch_wakeup(dq);
	}
	_dispatch_sync_f_slow(dq, ctxt, func);
}

DISPATCH_NOINLINE
static void
_dispatch_sync_f_invoke(dispatch_queue_t dq, void *ctxt,
						dispatch_function_t func)
{
	_dispatch_function_invoke(dq, ctxt, func);
	if (slowpath(dispatch_atomic_sub2o(dq, dq_running, 2) == 0)) {
		_dispatch_wakeup(dq);
	}
}

DISPATCH_NOINLINE
static void
_dispatch_sync_f_recurse(dispatch_queue_t dq, void *ctxt,
						 dispatch_function_t func)
{
	_dispatch_function_recurse(dq, ctxt, func);
	if (slowpath(dispatch_atomic_sub2o(dq, dq_running, 2) == 0)) {
		_dispatch_wakeup(dq);
	}
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

DISPATCH_NOINLINE
static void
_dispatch_sync_f2(dispatch_queue_t dq, void *ctxt, dispatch_function_t func)
{
	// 1) ensure that this thread hasn't enqueued anything ahead of this call
	// 2) the queue is not suspended
	if (slowpath(dq->dq_items_tail) || slowpath(DISPATCH_OBJECT_SUSPENDED(dq))){
		return _dispatch_sync_f_slow(dq, ctxt, func);
	}
	if (slowpath(dispatch_atomic_add2o(dq, dq_running, 2) & 1)) {
		return _dispatch_sync_f_slow2(dq, ctxt, func);
	}
	if (slowpath(dq->do_targetq->do_targetq)) {
		return _dispatch_sync_f_recurse(dq, ctxt, func);
	}
	_dispatch_sync_f_invoke(dq, ctxt, func);
}
DISPATCH_NOINLINE
void
dispatch_sync_f(dispatch_queue_t dq, void *ctxt, dispatch_function_t func)
{
	if (fastpath(dq->dq_width == 1)) {
		return dispatch_barrier_sync_f(dq, ctxt, func);//wjf to barrier
	}
	if (slowpath(!dq->do_targetq)) {
		// the global root queues do not need strict ordering
		(void)dispatch_atomic_add2o(dq, dq_running, 2);
		return _dispatch_sync_f_invoke(dq, ctxt, func);
	}
	_dispatch_sync_f2(dq, ctxt, func);
}

#ifdef __BLOCKS__
#if DISPATCH_COCOA_COMPAT
DISPATCH_NOINLINE
static void
_dispatch_sync_slow(dispatch_queue_t dq, void (^work)(void))
{
	// Blocks submitted to the main queue MUST be run on the main thread,
	// therefore under GC we must Block_copy in order to notify the thread-local
	// garbage collector that the objects are transferring to the main thread
	// rdar://problem/7176237&7181849&7458685
//	if (dispatch_begin_thread_4GC) {
//		dispatch_block_t block = _dispatch_Block_copy(work);
//		return dispatch_sync_f(dq, block, _dispatch_call_block_and_release);
//	}
	struct Block_basic *bb = (void *)work;
	dispatch_sync_f(dq, work, (dispatch_function_t)bb->Block_invoke);
}
#endif
void
dispatch_sync(dispatch_queue_t dq, void (^work)(void))
{
#if DISPATCH_COCOA_COMPAT
	if (slowpath(dq == &_dispatch_main_q)) {
		return _dispatch_sync_slow(dq, work);
	}
#endif
	struct Block_basic *bb = (void *)work;
	dispatch_sync_f(dq, work, (dispatch_function_t)bb->Block_invoke);
}
#endif
