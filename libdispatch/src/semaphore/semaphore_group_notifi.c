#include <stdio.h>

void
dispatch_group_leave(dispatch_group_t dg)
{
	dispatch_semaphore_t dsema = (dispatch_semaphore_t)dg;
	dispatch_atomic_release_barrier();
	long value = dispatch_atomic_inc2o(dsema, dsema_value);
	if (slowpath(value == LONG_MIN)) {
		DISPATCH_CLIENT_CRASH("Unbalanced call to dispatch_group_leave()");
	}
	if (slowpath(value == dsema->dsema_orig)) {
		(void)_dispatch_group_wake(dsema);/////////////wjf
	}
}

DISPATCH_NOINLINE
void
dispatch_group_notify_f(dispatch_group_t dg,
						dispatch_queue_t dq,
						void *ctxt,
						void (*func)(void *))
{
	dispatch_semaphore_t dsema = (dispatch_semaphore_t)dg;
	struct dispatch_sema_notify_s *dsn, *prev;
	
	// FIXME -- this should be updated to use the continuation cache
	while (!(dsn = calloc(1, sizeof(*dsn)))) {
		sleep(1);
	}
	dsn->dsn_queue = dq;
	dsn->dsn_ctxt = ctxt;
	dsn->dsn_func = func;
	
	_dispatch_retain(dq);
	dispatch_atomic_store_barrier();
	prev = dispatch_atomic_xchg2o(dsema, dsema_notify_tail, dsn);
	if (fastpath(prev)) {
		prev->dsn_next = dsn;
	} else {
		_dispatch_retain(dg);
		(void)dispatch_atomic_xchg2o(dsema, dsema_notify_head, dsn);
		if (dsema->dsema_value == dsema->dsema_orig) {
			_dispatch_group_wake(dsema);
		}
	}
}
#ifdef __BLOCKS__
void
dispatch_group_notify(dispatch_group_t dg, dispatch_queue_t dq,
					  dispatch_block_t db)
{
	dispatch_group_notify_f(dg, dq, _dispatch_Block_copy(db),
							_dispatch_call_block_and_release);
}
#endif

