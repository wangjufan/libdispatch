#include <stdio.h>

dispatch_group_t
dispatch_group_create(void)
{
	dispatch_group_t dg = _dispatch_alloc(DISPATCH_VTABLE(group),
										  sizeof(struct dispatch_semaphore_s)
										  );
	_dispatch_semaphore_init(LONG_MAX, dg);
	return dg;
}


DISPATCH_NOINLINE
void
dispatch_group_async_f(dispatch_group_t dg, dispatch_queue_t dq, void *ctxt,
					   dispatch_function_t func)
{
	dispatch_continuation_t dc;
	
	_dispatch_retain(dg);
	dispatch_group_enter(dg);//leave in _dispatch_continuation_pop
	
	dc = _dispatch_continuation_alloc();
	dc->do_vtable = (void *)(DISPATCH_OBJ_ASYNC_BIT | DISPATCH_OBJ_GROUP_BIT);
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	dc->dc_data = dg;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->dq_width != 1 && dq->do_targetq) {
		return _dispatch_async_f2(dq, dc);
		//may on main thread , the main thread will be interrupted
	}/////////////wjf
	
	_dispatch_queue_push(dq, dc);
}

#ifdef __BLOCKS__
void
dispatch_group_async(dispatch_group_t dg, dispatch_queue_t dq,
					 dispatch_block_t db)
{
	dispatch_group_async_f(dg, dq, _dispatch_Block_copy(db),
						   _dispatch_call_block_and_release);
}
#endif


////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////
void
dispatch_group_enter(dispatch_group_t dg)
{
	dispatch_semaphore_t dsema = (dispatch_semaphore_t)dg;
	(void)dispatch_semaphore_wait(dsema, DISPATCH_TIME_FOREVER);
}
