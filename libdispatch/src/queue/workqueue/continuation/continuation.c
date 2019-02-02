#include "continuation.h"

dispatch_continuation_t
_dispatch_continuation_alloc_from_heap(void)
{
	static dispatch_once_t pred;
	dispatch_continuation_t dc;
	
	dispatch_once_f(&pred, NULL, _dispatch_ccache_init);
	
	// This is also used for allocating struct dispatch_apply_s. If the
	// ROUND_UP behavior is changed, adjust the assert in libdispatch_init
	while (!(dc = fastpath(malloc_zone_calloc(
								_dispatch_ccache_zone, 1,
								ROUND_UP_TO_CACHELINE_SIZE(sizeof(*dc)))))) {
		sleep(1);
	}
	
	return dc;
}
DISPATCH_ALWAYS_INLINE
static inline dispatch_continuation_t
_dispatch_continuation_alloc_cacheonly(void)
{
	dispatch_continuation_t dc;
	dc = fastpath((dispatch_continuation_t)
				  _dispatch_thread_getspecific(dispatch_cache_key));
	if (dc) {
		_dispatch_thread_setspecific(dispatch_cache_key, dc->do_next);
	}
	return dc;
}
DISPATCH_ALWAYS_INLINE
static inline dispatch_continuation_t
_dispatch_continuation_alloc(void)
{
	dispatch_continuation_t dc;
	
	dc = fastpath(_dispatch_continuation_alloc_cacheonly());
	if(!dc) {
		return _dispatch_continuation_alloc_from_heap();
	}
	return dc;
}


DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_continuation_free(dispatch_continuation_t dc)
{
	dispatch_continuation_t prev_dc;
	prev_dc = (dispatch_continuation_t)
	_dispatch_thread_getspecific(dispatch_cache_key);
	dc->do_next = prev_dc;
	_dispatch_thread_setspecific(dispatch_cache_key, dc);
}

///////////////////////////////////////////////////////

#pragma mark -
#pragma mark dispatch_continuation_t

static malloc_zone_t *_dispatch_ccache_zone;

static void
_dispatch_ccache_init(void *context DISPATCH_UNUSED)
{
	_dispatch_ccache_zone = malloc_create_zone(0, 0);//wjf a zone for continuation
	dispatch_assert(_dispatch_ccache_zone);
	malloc_set_zone_name(_dispatch_ccache_zone, "DispatchContinuations");
}

/////////////////////////////////////////////////////////////

static void
_dispatch_force_cache_cleanup(void)
{
	dispatch_continuation_t dc;
	dc = _dispatch_thread_getspecific(dispatch_cache_key);
	if (dc) {
		_dispatch_thread_setspecific(dispatch_cache_key, NULL);
		_dispatch_cache_cleanup(dc);
	}
}
// rdar://problem/11500155
void
dispatch_flush_continuation_cache(void)
{
	_dispatch_force_cache_cleanup();
}
DISPATCH_NOINLINE
static void
_dispatch_cache_cleanup(void *value)
{//wjf
	dispatch_continuation_t dc, next_dc = value;
	while ((dc = next_dc)) {
		next_dc = dc->do_next;
		malloc_zone_free(_dispatch_ccache_zone, dc);
	}
}

/////////////////////////////////////////////////////////////

DISPATCH_ALWAYS_INLINE_NDEBUG
static inline void
_dispatch_continuation_redirect(dispatch_queue_t dq, dispatch_object_t dou)
{
	dispatch_continuation_t dc = dou._dc;
	
	_dispatch_trace_continuation_pop(dq, dou);
	(void)dispatch_atomic_add2o(dq, dq_running, 2);
	if (!DISPATCH_OBJ_IS_VTABLE(dc) &&
		(long)dc->do_vtable & DISPATCH_OBJ_SYNC_SLOW_BIT) {
		dispatch_atomic_barrier();
		_dispatch_thread_semaphore_signal(
								(_dispatch_thread_semaphore_t)dc->dc_ctxt);
	} else {
		_dispatch_async_f_redirect(dq, dc);
	}
}

/////////////////////////////////////////////////////////////

DISPATCH_ALWAYS_INLINE_NDEBUG
static inline void
_dispatch_continuation_pop(dispatch_object_t dou)
{
	dispatch_continuation_t dc = dou._dc;
	dispatch_group_t dg;
	
	_dispatch_trace_continuation_pop(_dispatch_queue_get_current(), dou);
	if (DISPATCH_OBJ_IS_VTABLE(dou._do)) {
		return _dispatch_queue_invoke(dou._dq);
	}
	
	// Add the item back to the cache before calling the function. This
	// allows the 'hot' continuation to be used for a quick callback.
	//
	// The ccache version is per-thread.
	// Therefore, the object has not been reused yet.
	// This generates better assembly.
	if ((long)dc->do_vtable & DISPATCH_OBJ_ASYNC_BIT) {
		_dispatch_continuation_free(dc);
	}
	if ((long)dc->do_vtable & DISPATCH_OBJ_GROUP_BIT) {
		dg = dc->dc_data;
	} else {
		dg = NULL;
	}
	_dispatch_client_callout(dc->dc_ctxt, dc->dc_func);
	if (dg) {
		dispatch_group_leave(dg);
		_dispatch_release(dg);
	}
}

