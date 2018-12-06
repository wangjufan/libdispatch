//
//  invoke.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "invoke.h"

#pragma mark -
#pragma mark dispatch_function_invoke

DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_function_invoke(dispatch_queue_t dq, void *ctxt,
						  dispatch_function_t func)
{
	dispatch_queue_t old_dq = _dispatch_thread_getspecific(dispatch_queue_key);
	_dispatch_thread_setspecific(dispatch_queue_key, dq);
	_dispatch_client_callout(ctxt, func);///wjf
	_dispatch_workitem_inc();//dispatch_bcounter_key+1
	_dispatch_thread_setspecific(dispatch_queue_key, old_dq);
}

static void
_dispatch_function_recurse_invoke(void *ctxt)
{
	struct dispatch_function_recurse_s *dfr = ctxt;
	_dispatch_function_invoke(dfr->dfr_dq, dfr->dfr_ctxt, dfr->dfr_func);
}
DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_function_recurse(dispatch_queue_t dq, void *ctxt,
						   dispatch_function_t func)
{
	struct dispatch_function_recurse_s dfr = {
		.dfr_dq = dq,
		.dfr_func = func,
		.dfr_ctxt = ctxt,
	};
	dispatch_sync_f(dq->do_targetq, &dfr, _dispatch_function_recurse_invoke);
}


