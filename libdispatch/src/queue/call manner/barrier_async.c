#include "barrier_async.h"

#pragma mark -
#pragma mark dispatch_barrier_async

DISPATCH_NOINLINE
static void
_dispatch_barrier_async_f_slow(dispatch_queue_t dq, void *ctxt,
							   dispatch_function_t func)
{
	dispatch_continuation_t dc = _dispatch_continuation_alloc_from_heap();
	
	dc->do_vtable = (void *)(DISPATCH_OBJ_ASYNC_BIT | DISPATCH_OBJ_BARRIER_BIT);
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	
	_dispatch_queue_push(dq, dc);
}

DISPATCH_NOINLINE
void
dispatch_barrier_async_f(dispatch_queue_t dq, void *ctxt,
						 dispatch_function_t func)
{
	dispatch_continuation_t dc;
	
	dc = fastpath(_dispatch_continuation_alloc_cacheonly());
	if (!dc) {//slowpath
		return _dispatch_barrier_async_f_slow(dq, ctxt, func);
	}
	
	dc->do_vtable = (void *)(DISPATCH_OBJ_ASYNC_BIT | DISPATCH_OBJ_BARRIER_BIT);
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	
	_dispatch_queue_push(dq, dc);//延迟分发到 目标队列，或是l不理会目标队列
}
#ifdef __BLOCKS__
void
dispatch_barrier_async(dispatch_queue_t dq, void (^work)(void))
{
	dispatch_barrier_async_f(dq, _dispatch_Block_copy(work),
							 _dispatch_call_block_and_release);
}

#endif

