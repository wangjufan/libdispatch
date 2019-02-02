
#include "internal.h"
#if HAVE_MACH
#include "protocol.h"
#include "protocolServer.h"
#endif
#include <sys/mount.h>

#pragma mark -
#pragma mark dispatch_source_t

dispatch_source_t
dispatch_source_create(dispatch_source_type_t type,
					   uintptr_t handle,
					   unsigned long mask,
					   dispatch_queue_t q)
{
	const struct kevent *proto_kev = &type->ke;
	dispatch_source_t ds = NULL;
	dispatch_kevent_t dk = NULL;
//拷贝模式，
	// input validation
	if (type == NULL || (mask & ~type->mask)) {
		return NULL;
	}

	switch (type->ke.filter) {
	case EVFILT_SIGNAL:
		if (handle >= NSIG) {
			return NULL;
		}
		break;
	case EVFILT_FS:
#if DISPATCH_USE_VM_PRESSURE
	case EVFILT_VM:
#endif
	case DISPATCH_EVFILT_CUSTOM_ADD:
	case DISPATCH_EVFILT_CUSTOM_OR:
	case DISPATCH_EVFILT_TIMER:
		if (handle) {
			return NULL;
		}
		break;
	default:
		break;
	}

	dk = calloc(1ul, sizeof(struct dispatch_kevent_s));
	dk->dk_kevent = *proto_kev;
	dk->dk_kevent.ident = handle;
	dk->dk_kevent.flags |= EV_ADD|EV_ENABLE;
	dk->dk_kevent.fflags |= (uint32_t)mask;
	dk->dk_kevent.udata = dk;
	TAILQ_INIT(&dk->dk_sources);

	ds = _dispatch_alloc(DISPATCH_VTABLE(source),
			sizeof(struct dispatch_source_s));
	// Initialize as a queue first, then override some settings below.
	_dispatch_queue_init((dispatch_queue_t)ds);
	strlcpy(ds->dq_label, "source", sizeof(ds->dq_label));
	// Dispatch Object
	ds->do_ref_cnt++; // the reference the manger queue holds
	ds->do_ref_cnt++; // since source is created suspended
	ds->do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_INTERVAL;
	// The initial target queue is the manager queue, in order to get
	// the source installed. <rdar://problem/8928171>
	ds->do_targetq = &_dispatch_mgr_q;
	// Dispatch Source
	ds->ds_ident_hack = dk->dk_kevent.ident;
	ds->ds_dkev = dk;
	ds->ds_pending_data_mask = dk->dk_kevent.fflags;
	if ((EV_DISPATCH|EV_ONESHOT) & proto_kev->flags) {
		ds->ds_is_level = true;
		ds->ds_needs_rearm = true;
	} else if (!(EV_CLEAR & proto_kev->flags)) {
		// we cheat and use EV_CLEAR to mean a "flag thingy"
		ds->ds_is_adder = true;
	}

	// Some sources require special processing
	if (type->init != NULL) {
		type->init(ds, type, handle, mask, q);
	}
	if (fastpath(!ds->ds_refs)) {
		ds->ds_refs = calloc(1ul, sizeof(struct dispatch_source_refs_s));
		if (slowpath(!ds->ds_refs)) {
			goto out_bad;
		}
	}
	ds->ds_refs->dr_source_wref = _dispatch_ptr2wref(ds);
	dispatch_assert(!(ds->ds_is_level && ds->ds_is_adder));

	// First item on the queue sets the user-specified target queue
	dispatch_set_target_queue(ds, q);
#if DISPATCH_DEBUG
	dispatch_debug(ds, "%s", __func__);
#endif
	return ds;

out_bad:
	free(ds);
	free(dk);
	return NULL;
}


void
_dispatch_source_dispose(dispatch_source_t ds)
{
	free(ds->ds_refs);
	_dispatch_queue_dispose((dispatch_queue_t)ds);
}
void
_dispatch_source_xref_dispose(dispatch_source_t ds)
{
	_dispatch_wakeup(ds);
}

void
dispatch_source_cancel(dispatch_source_t ds)
{
#if DISPATCH_DEBUG
	dispatch_debug(ds, "%s", __func__);
#endif
	// Right after we set the cancel flag, someone else
	// could potentially invoke the source, do the cancelation,
	// unregister the source, and deallocate it. We would
	// need to therefore retain/release before setting the bit

	_dispatch_retain(ds);
	(void)dispatch_atomic_or2o(ds, ds_atomic_flags, DSF_CANCELED);
	_dispatch_wakeup(ds);
	_dispatch_release(ds);
}
long
dispatch_source_testcancel(dispatch_source_t ds)
{
	return (bool)(ds->ds_atomic_flags & DSF_CANCELED);
}


unsigned long
dispatch_source_get_mask(dispatch_source_t ds)
{
	return ds->ds_pending_data_mask;
}
uintptr_t
dispatch_source_get_handle(dispatch_source_t ds)
{
	return (int)ds->ds_ident_hack;
}
unsigned long
dispatch_source_get_data(dispatch_source_t ds)
{
	return ds->ds_data;
}
void
dispatch_source_merge_data(dispatch_source_t ds, unsigned long val)
{
	struct kevent kev = {
		.fflags = (typeof(kev.fflags))val,
		.data = val,
	};
	dispatch_assert(
			ds->ds_dkev->dk_kevent.filter == DISPATCH_EVFILT_CUSTOM_ADD ||
			ds->ds_dkev->dk_kevent.filter == DISPATCH_EVFILT_CUSTOM_OR);
	_dispatch_source_merge_kevent(ds, &kev);
}
