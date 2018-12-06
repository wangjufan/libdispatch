#include "queue_async.h"

#pragma mark -
#pragma mark dispatch_async


DISPATCH_NOINLINE
static void
_dispatch_async_f2_slow(dispatch_queue_t dq, dispatch_continuation_t dc)
{
	_dispatch_wakeup(dq);
	_dispatch_queue_push(dq, dc);
}
static void
_dispatch_async_f_redirect_invoke(void *_ctxt)
{
	struct dispatch_continuation_s *dc = _ctxt;
	struct dispatch_continuation_s *other_dc = dc->dc_other;
	dispatch_queue_t old_dq, dq = dc->dc_data, rq;
	
	old_dq = _dispatch_thread_getspecific(dispatch_queue_key);
	_dispatch_thread_setspecific(dispatch_queue_key, dq);
	_dispatch_continuation_pop(other_dc);
	_dispatch_thread_setspecific(dispatch_queue_key, old_dq);
	
	rq = dq->do_targetq;
	while (slowpath(rq->do_targetq) && rq != old_dq) {
		if (dispatch_atomic_sub2o(rq, dq_running, 2) == 0) {
			_dispatch_wakeup(rq);
		}
		rq = rq->do_targetq;
	}
	
	if (dispatch_atomic_sub2o(dq, dq_running, 2) == 0) {
		_dispatch_wakeup(dq);
	}
	_dispatch_release(dq);
}


DISPATCH_NOINLINE
static void
_dispatch_async_f_redirect(dispatch_queue_t dq,
						   dispatch_continuation_t other_dc)///////
{
	dispatch_continuation_t dc;
	dispatch_queue_t rq;
	
	_dispatch_retain(dq);
	
	dc = _dispatch_continuation_alloc();
	dc->do_vtable = (void *)DISPATCH_OBJ_ASYNC_BIT;
	dc->dc_func = _dispatch_async_f_redirect_invoke;
	dc->dc_ctxt = dc;
	dc->dc_data = dq;
	dc->dc_other = other_dc;
	
	// Find the queue to redirect to
	rq = dq->do_targetq;
	while (slowpath(rq->do_targetq)) {
		uint32_t running;
		
		if (slowpath(rq->dq_items_tail) ||
			slowpath(DISPATCH_OBJECT_SUSPENDED(rq)) ||
			slowpath(rq->dq_width == 1)) {
			break;
		}
		running = dispatch_atomic_add2o(rq, dq_running, 2) - 2;
		if (slowpath(running & 1) || slowpath(running + 2 > rq->dq_width)) {
			if (slowpath(dispatch_atomic_sub2o(rq, dq_running, 2) == 0)) {
				return _dispatch_async_f2_slow(rq, dc);
			}
			break;
		}
		rq = rq->do_targetq;
	}
	_dispatch_queue_push(rq, dc);
}
DISPATCH_NOINLINE
static void
_dispatch_async_f2(dispatch_queue_t dq, dispatch_continuation_t dc)
{
	uint32_t running;
	bool locked;
	
	do {
		if (slowpath(dq->dq_items_tail)
			|| slowpath(DISPATCH_OBJECT_SUSPENDED(dq))) {
			break;
		}
		running = dispatch_atomic_add2o(dq, dq_running, 2);
		if (slowpath(running > dq->dq_width)) {
			if (slowpath(dispatch_atomic_sub2o(dq, dq_running, 2) == 0)) {
				return _dispatch_async_f2_slow(dq, dc);
			}
			break;
		}
		locked = running & 1;
		if (fastpath(!locked)) {
			return _dispatch_async_f_redirect(dq, dc);
		}
		locked = dispatch_atomic_sub2o(dq, dq_running, 2) & 1;
		// We might get lucky and find that the barrier has ended by now
	} while (!locked);
	
	_dispatch_queue_push(dq, dc);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

DISPATCH_NOINLINE
static void
_dispatch_async_f_slow(dispatch_queue_t dq, void *ctxt,
					   dispatch_function_t func)
{
	dispatch_continuation_t dc = _dispatch_continuation_alloc_from_heap();
	
	dc->do_vtable = (void *)DISPATCH_OBJ_ASYNC_BIT;
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->do_targetq) {
		return _dispatch_async_f2(dq, dc);
	}
	
	_dispatch_queue_push(dq, dc);
}
DISPATCH_NOINLINE
void
dispatch_async_f(dispatch_queue_t dq,
				 void *ctxt,
				 dispatch_function_t func)
{
	dispatch_continuation_t dc;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->dq_width == 1) {//
		return dispatch_barrier_async_f(dq, ctxt, func);
	}
	
	dc = fastpath(_dispatch_continuation_alloc_cacheonly());
	if (!dc) {
		return _dispatch_async_f_slow(dq, ctxt, func);
	}
	
	dc->do_vtable = (void *)DISPATCH_OBJ_ASYNC_BIT;
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->do_targetq) {
		return _dispatch_async_f2(dq, dc);
	}
	
	_dispatch_queue_push(dq, dc);
}

#ifdef __BLOCKS__
void
dispatch_async(dispatch_queue_t dq, void (^work)(void))
{
	dispatch_async_f(dq,
					 _dispatch_Block_copy(work),
					 _dispatch_call_block_and_release);
}
#endif

#pragma mark -
#pragma mark dispatch_group_async
