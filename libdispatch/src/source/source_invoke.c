//
//  source_invoke.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#include "source_invoke.h"

#pragma mark -
#pragma mark dispatch_source_invoke

static void
_dispatch_source_registration_callout(dispatch_source_t ds)
{
	dispatch_source_refs_t dr = ds->ds_refs;
	
	if ((ds->ds_atomic_flags & DSF_CANCELED) || (ds->do_xref_cnt == -1)) {
		// no registration callout if source is canceled rdar://problem/8955246
#ifdef __BLOCKS__
		if (ds->ds_registration_is_block) {
			Block_release(dr->ds_registration_handler);
		}
	} else if (ds->ds_registration_is_block) {
		dispatch_block_t b = dr->ds_registration_handler;
		_dispatch_client_callout_block(b);
		Block_release(dr->ds_registration_handler);
#endif
	} else {
		dispatch_function_t f = dr->ds_registration_handler;
		_dispatch_client_callout(ds->do_ctxt, f);
	}
	ds->ds_registration_is_block = false;
	dr->ds_registration_handler = NULL;
}

static void
_dispatch_source_cancel_callout(dispatch_source_t ds)
{
	dispatch_source_refs_t dr = ds->ds_refs;
	
	ds->ds_pending_data_mask = 0;
	ds->ds_pending_data = 0;
	ds->ds_data = 0;
	
#ifdef __BLOCKS__
	if (ds->ds_handler_is_block) {
		Block_release(dr->ds_handler_ctxt);
		ds->ds_handler_is_block = false;
		dr->ds_handler_func = NULL;
		dr->ds_handler_ctxt = NULL;
	}
	if (ds->ds_registration_is_block) {
		Block_release(dr->ds_registration_handler);
		ds->ds_registration_is_block = false;
		dr->ds_registration_handler = NULL;
	}
#endif
	
	if (!dr->ds_cancel_handler) {
		return;
	}
	if (ds->ds_cancel_is_block) {
#ifdef __BLOCKS__
		dispatch_block_t b = dr->ds_cancel_handler;
		if (ds->ds_atomic_flags & DSF_CANCELED) {
			_dispatch_client_callout_block(b);
		}
		Block_release(dr->ds_cancel_handler);
		ds->ds_cancel_is_block = false;
#endif
	} else {
		dispatch_function_t f = dr->ds_cancel_handler;
		if (ds->ds_atomic_flags & DSF_CANCELED) {
			_dispatch_client_callout(ds->do_ctxt, f);
		}
	}
	dr->ds_cancel_handler = NULL;
}

static void
_dispatch_source_latch_and_call(dispatch_source_t ds)
{
	unsigned long prev;
	
	if ((ds->ds_atomic_flags & DSF_CANCELED) || (ds->do_xref_cnt == -1)) {
		return;
	}
	dispatch_source_refs_t dr = ds->ds_refs;
	prev = dispatch_atomic_xchg2o(ds, ds_pending_data, 0);
	if (ds->ds_is_level) {
		ds->ds_data = ~prev;
	} else if (ds->ds_is_timer && ds_timer(dr).target && prev) {
		ds->ds_data = _dispatch_source_timer_data(dr, prev);
	} else {
		ds->ds_data = prev;
	}
	if (dispatch_assume(prev) && dr->ds_handler_func) {
		_dispatch_client_callout(dr->ds_handler_ctxt, dr->ds_handler_func);
	}
}

static void
_dispatch_source_kevent_resume(dispatch_source_t ds, uint32_t new_flags)
{
	switch (ds->ds_dkev->dk_kevent.filter) {
		case DISPATCH_EVFILT_TIMER:
			// called on manager queue only
			return _dispatch_timer_list_update(ds);
#if HAVE_MACH
		case EVFILT_MACHPORT:
			if (ds->ds_pending_data_mask & DISPATCH_MACH_RECV_MESSAGE) {
				new_flags |= DISPATCH_MACH_RECV_MESSAGE; // emulate EV_DISPATCH
			}
			break;
#endif /* HAVE_MACH */
	}
	if (_dispatch_kevent_resume(ds->ds_dkev, new_flags, 0)) {
		_dispatch_kevent_unregister(ds);
	}
}

dispatch_queue_t
_dispatch_source_invoke(dispatch_source_t ds)
{
	// This function performs all source actions. Each action is responsible
	// for verifying that it takes place on the appropriate queue. If the
	// current queue is not the correct queue for this action, the correct queue
	// will be returned and the invoke will be re-driven on that queue.
	
	// The order of tests here in invoke and in probe should be consistent.
	
	dispatch_queue_t dq = _dispatch_queue_get_current();
	dispatch_source_refs_t dr = ds->ds_refs;
	
	if (!ds->ds_is_installed) {
		// The source needs to be installed on the manager queue.
		if (dq != &_dispatch_mgr_q) {
			return &_dispatch_mgr_q;
		}
		_dispatch_kevent_register(ds);//////////////////
		if (dr->ds_registration_handler) {
			return ds->do_targetq;
		}
		if (slowpath(ds->do_xref_cnt == -1)) {
			return &_dispatch_mgr_q; // rdar://problem/9558246
		}
	} else if (slowpath(DISPATCH_OBJECT_SUSPENDED(ds))) {
		// Source suspended by an item drained from the source queue.
		return NULL;
	} else if (dr->ds_registration_handler) {
		// The source has been registered and the registration handler needs
		// to be delivered on the target queue.
		if (dq != ds->do_targetq) {
			return ds->do_targetq;
		}
		// clears ds_registration_handler
		_dispatch_source_registration_callout(ds);//////////////////
		if (slowpath(ds->do_xref_cnt == -1)) {
			return &_dispatch_mgr_q; // rdar://problem/9558246
		}
	} else if ((ds->ds_atomic_flags & DSF_CANCELED) || (ds->do_xref_cnt == -1)){
		// The source has been cancelled and needs to be uninstalled from the
		// manager queue. After uninstallation, the cancellation handler needs
		// to be delivered to the target queue.
		if (ds->ds_dkev) {
			if (dq != &_dispatch_mgr_q) {
				return &_dispatch_mgr_q;
			}
			_dispatch_kevent_unregister(ds);//////////////////
		}
		if (dr->ds_cancel_handler || ds->ds_handler_is_block ||
			ds->ds_registration_is_block) {
			if (dq != ds->do_targetq) {
				return ds->do_targetq;
			}
		}
		_dispatch_source_cancel_callout(ds);//////////////////
	} else if (ds->ds_pending_data) {
		// The source has pending data to deliver via the event handler callback
		// on the target queue. Some sources need to be rearmed on the manager
		// queue after event delivery.
		if (dq != ds->do_targetq) {
			return ds->do_targetq;
		}
		_dispatch_source_latch_and_call(ds);//////////////////
		if (ds->ds_needs_rearm) {
			return &_dispatch_mgr_q;
		}
	} else if (ds->ds_needs_rearm && !(ds->ds_atomic_flags & DSF_ARMED)) {
		// The source needs to be rearmed on the manager queue.
		if (dq != &_dispatch_mgr_q) {
			return &_dispatch_mgr_q;
		}
		_dispatch_source_kevent_resume(ds, 0);//////////////////
		(void)dispatch_atomic_or2o(ds, ds_atomic_flags, DSF_ARMED);
	}
	
	return NULL;
}

bool
_dispatch_source_probe(dispatch_source_t ds)
{
	// This function determines whether the source needs to be invoked.
	// The order of tests here in probe and in invoke should be consistent.
	dispatch_source_refs_t dr = ds->ds_refs;
	if (!ds->ds_is_installed) {
		// The source needs to be installed on the manager queue.
		return true;
	} else if (dr->ds_registration_handler) {
		// The registration handler needs to be delivered to the target queue.
		return true;
	} else if ((ds->ds_atomic_flags & DSF_CANCELED) || (ds->do_xref_cnt == -1)){
		// The source needs to be uninstalled from the manager queue, or the
		// cancellation handler needs to be delivered to the target queue.
		// Note: cancellation assumes installation.
		if (ds->ds_dkev || dr->ds_cancel_handler
#ifdef __BLOCKS__
			|| ds->ds_handler_is_block || ds->ds_registration_is_block
#endif
			) {
			return true;
		}
	} else if (ds->ds_pending_data) {
		// The source has pending data to deliver to the target queue.
		return true;
	} else if (ds->ds_needs_rearm && !(ds->ds_atomic_flags & DSF_ARMED)) {
		// The source needs to be rearmed on the manager queue.
		return true;
	}
	// Nothing to do.
	return false;
}
