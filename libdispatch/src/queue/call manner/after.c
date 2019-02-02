//
//  after.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "after.h"

#pragma mark -
#pragma mark dispatch_after

static void
_dispatch_after_timer_callback(void *ctxt)
{
	struct _dispatch_after_time_s *datc = ctxt;
	
	dispatch_assert(datc->datc_func);
	_dispatch_client_callout(datc->datc_ctxt, datc->datc_func);
	
	dispatch_source_t ds = datc->ds;
	free(datc);
	
	dispatch_source_cancel(ds); // Needed until 7287561 gets integrated
	dispatch_release(ds);
}

DISPATCH_NOINLINE void
dispatch_after_f(dispatch_time_t when,
				 dispatch_queue_t queue, void *ctxt,
				 dispatch_function_t func)
{
	uint64_t delta;
	struct _dispatch_after_time_s *datc = NULL;
	dispatch_source_t ds;
	
	if (when == DISPATCH_TIME_FOREVER) {
#if DISPATCH_DEBUG
		DISPATCH_CLIENT_CRASH("dispatch_after_f() called with 'when' == infinity");
#endif
		return;
	}
	
	// this function can and should be optimized to not use a dispatch source
	delta = _dispatch_timeout(when);
	if (delta == 0) {
		return dispatch_async_f(queue, ctxt, func);//no delay
	}
	// on successful creation, source owns malloc-ed context (which it frees in
	// the event handler)
	ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
	dispatch_assert(ds);
	
	datc = malloc(sizeof(*datc));
	dispatch_assert(datc);
	datc->datc_ctxt = ctxt;
	datc->datc_func = func;
	datc->ds = ds;
	
	dispatch_set_context(ds, datc);
	dispatch_source_set_event_handler_f(ds, _dispatch_after_timer_callback);
	dispatch_source_set_timer(ds, when, DISPATCH_TIME_FOREVER, 0);
	dispatch_resume(ds);
}

#ifdef __BLOCKS__
void
dispatch_after(dispatch_time_t when,
			   dispatch_queue_t queue,
			   dispatch_block_t work)
{
	// test before the copy of the block
	if (when == DISPATCH_TIME_FOREVER) {
#if DISPATCH_DEBUG
		DISPATCH_CLIENT_CRASH("dispatch_after() called with 'when' == infinity");
#endif
		return;
	}
	dispatch_after_f(when, queue,
					 _dispatch_Block_copy(work),
					 _dispatch_call_block_and_release);
}
#endif
