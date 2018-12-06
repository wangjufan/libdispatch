//
//  source_handler.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#include "source_handler.h"

#pragma mark -
#pragma mark dispatch_source_handler

#ifdef __BLOCKS__
// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
static void
_dispatch_source_set_event_handler2(void *context) {
	struct Block_layout *bl = context;
	dispatch_source_t ds = (dispatch_source_t)_dispatch_queue_get_current();
	dispatch_assert(dx_type(ds) == DISPATCH_SOURCE_KEVENT_TYPE);
	dispatch_source_refs_t dr = ds->ds_refs;
	
	if (ds->ds_handler_is_block && dr->ds_handler_ctxt) {
		Block_release(dr->ds_handler_ctxt);
	}
	dr->ds_handler_func = bl ? (void *)bl->invoke : NULL;
	dr->ds_handler_ctxt = bl;
	ds->ds_handler_is_block = true;
}
void
dispatch_source_set_event_handler(dispatch_source_t ds,
								  dispatch_block_t handler) {
	handler = _dispatch_Block_copy(handler);
	dispatch_barrier_async_f((dispatch_queue_t)ds, handler,
							 _dispatch_source_set_event_handler2);
}
#endif /* __BLOCKS__ */


static void
_dispatch_source_set_event_handler_f(void *context)
{
	dispatch_source_t ds = (dispatch_source_t)_dispatch_queue_get_current();
	dispatch_assert(dx_type(ds) == DISPATCH_SOURCE_KEVENT_TYPE);
	dispatch_source_refs_t dr = ds->ds_refs;
	
#ifdef __BLOCKS__
	if (ds->ds_handler_is_block && dr->ds_handler_ctxt) {
		Block_release(dr->ds_handler_ctxt);
	}
#endif
	dr->ds_handler_func = context;
	dr->ds_handler_ctxt = ds->do_ctxt;
	ds->ds_handler_is_block = false;
}
void
dispatch_source_set_event_handler_f(dispatch_source_t ds,
									dispatch_function_t handler)
{
	dispatch_barrier_async_f((dispatch_queue_t)ds, handler,
							 _dispatch_source_set_event_handler_f);
}


#ifdef __BLOCKS__
// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
static void
_dispatch_source_set_cancel_handler2(void *context)
{
	dispatch_source_t ds = (dispatch_source_t)_dispatch_queue_get_current();
	dispatch_assert(dx_type(ds) == DISPATCH_SOURCE_KEVENT_TYPE);
	dispatch_source_refs_t dr = ds->ds_refs;
	
	if (ds->ds_cancel_is_block && dr->ds_cancel_handler) {
		Block_release(dr->ds_cancel_handler);
	}
	dr->ds_cancel_handler = context;
	ds->ds_cancel_is_block = true;
}
void
dispatch_source_set_cancel_handler(dispatch_source_t ds,
								   dispatch_block_t handler)
{
	handler = _dispatch_Block_copy(handler);
	dispatch_barrier_async_f((dispatch_queue_t)ds, handler,
							 _dispatch_source_set_cancel_handler2);
}
#endif /* __BLOCKS__ */


static void
_dispatch_source_set_cancel_handler_f(void *context)
{
	dispatch_source_t ds = (dispatch_source_t)_dispatch_queue_get_current();
	dispatch_assert(dx_type(ds) == DISPATCH_SOURCE_KEVENT_TYPE);
	dispatch_source_refs_t dr = ds->ds_refs;
	
#ifdef __BLOCKS__
	if (ds->ds_cancel_is_block && dr->ds_cancel_handler) {
		Block_release(dr->ds_cancel_handler);
	}
#endif
	dr->ds_cancel_handler = context;
	ds->ds_cancel_is_block = false;
}
void
dispatch_source_set_cancel_handler_f(dispatch_source_t ds,
									 dispatch_function_t handler)
{
	dispatch_barrier_async_f((dispatch_queue_t)ds, handler,
							 _dispatch_source_set_cancel_handler_f);
}


#ifdef __BLOCKS__
static void
_dispatch_source_set_registration_handler2(void *context)
{
	dispatch_source_t ds = (dispatch_source_t)_dispatch_queue_get_current();
	dispatch_assert(dx_type(ds) == DISPATCH_SOURCE_KEVENT_TYPE);
	dispatch_source_refs_t dr = ds->ds_refs;
	
	if (ds->ds_registration_is_block && dr->ds_registration_handler) {
		Block_release(dr->ds_registration_handler);
	}
	dr->ds_registration_handler = context;
	ds->ds_registration_is_block = true;
}
void
dispatch_source_set_registration_handler(dispatch_source_t ds,
										 dispatch_block_t handler)
{
	handler = _dispatch_Block_copy(handler);
	dispatch_barrier_async_f((dispatch_queue_t)ds, handler,
							 _dispatch_source_set_registration_handler2);
}
#endif /* __BLOCKS__ */


static void
_dispatch_source_set_registration_handler_f(void *context)
{
	dispatch_source_t ds = (dispatch_source_t)_dispatch_queue_get_current();
	dispatch_assert(dx_type(ds) == DISPATCH_SOURCE_KEVENT_TYPE);
	dispatch_source_refs_t dr = ds->ds_refs;
	
#ifdef __BLOCKS__
	if (ds->ds_registration_is_block && dr->ds_registration_handler) {
		Block_release(dr->ds_registration_handler);
	}
#endif
	dr->ds_registration_handler = context;
	ds->ds_registration_is_block = false;
}
void
dispatch_source_set_registration_handler_f(dispatch_source_t ds,
										   dispatch_function_t handler)
{
	dispatch_barrier_async_f((dispatch_queue_t)ds, handler,
							 _dispatch_source_set_registration_handler_f);
}
