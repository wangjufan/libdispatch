//
//  source_kevent.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#include "source_kevent.h"



#pragma mark -
#pragma mark dispatch_source_kevent

static void
_dispatch_source_merge_kevent(dispatch_source_t ds, const struct kevent *ke)
{
	struct kevent fake;
	
	if ((ds->ds_atomic_flags & DSF_CANCELED) || (ds->do_xref_cnt == -1)) {
		return;
	}
	
	// EVFILT_PROC may fail with ESRCH when the process exists but is a zombie
	// <rdar://problem/5067725>. As a workaround, we simulate an exit event for
	// any EVFILT_PROC with an invalid pid <rdar://problem/6626350>.
	if (ke->flags & EV_ERROR) {
		if (ke->filter == EVFILT_PROC && ke->data == ESRCH) {
			fake = *ke;
			fake.flags &= ~EV_ERROR;
			fake.fflags = NOTE_EXIT;
			fake.data = 0;
			ke = &fake;
#if DISPATCH_USE_VM_PRESSURE
		} else if (ke->filter == EVFILT_VM && ke->data == ENOTSUP) {
			// Memory pressure kevent is not supported on all platforms
			// <rdar://problem/8636227>
			return;
#endif
		} else {
			// log the unexpected error
			(void)dispatch_assume_zero(ke->data);
			return;
		}
	}
	
	if (ds->ds_is_level) {
		// ke->data is signed and "negative available data" makes no sense
		// zero bytes happens when EV_EOF is set
		// 10A268 does not fail this assert with EVFILT_READ and a 10 GB file
		dispatch_assert(ke->data >= 0l);
		ds->ds_pending_data = ~ke->data;
	} else if (ds->ds_is_adder) {
		(void)dispatch_atomic_add2o(ds, ds_pending_data, ke->data);
	} else if (ke->fflags & ds->ds_pending_data_mask) {
		(void)dispatch_atomic_or2o(ds, ds_pending_data,
								   ke->fflags & ds->ds_pending_data_mask);
	}
	
	// EV_DISPATCH and EV_ONESHOT sources are no longer armed after delivery
	if (ds->ds_needs_rearm) {
		(void)dispatch_atomic_and2o(ds, ds_atomic_flags, ~DSF_ARMED);
	}
	
	_dispatch_wakeup(ds);
}

void
_dispatch_source_drain_kevent(struct kevent *ke)
{
	dispatch_kevent_t dk = ke->udata;
	dispatch_source_refs_t dri;
	
#if DISPATCH_DEBUG
	static dispatch_once_t pred;
	dispatch_once_f(&pred, NULL, _dispatch_kevent_debugger);
#endif
	
	dispatch_debug_kevents(ke, 1, __func__);
	
#if HAVE_MACH
	if (ke->filter == EVFILT_MACHPORT) {
		return _dispatch_drain_mach_messages(ke);
	}
#endif
	dispatch_assert(dk);
	
	if (ke->flags & EV_ONESHOT) {
		dk->dk_kevent.flags |= EV_ONESHOT;
	}
	
	TAILQ_FOREACH(dri, &dk->dk_sources, dr_list) {
		_dispatch_source_merge_kevent(_dispatch_source_from_refs(dri), ke);
	}
}



#pragma mark -
#pragma mark dispatch_kevent_t

static struct dispatch_kevent_s _dispatch_kevent_data_or = {
	.dk_kevent = {
		.filter = DISPATCH_EVFILT_CUSTOM_OR,
		.flags = EV_CLEAR,
		.udata = &_dispatch_kevent_data_or,
	},
	.dk_sources = TAILQ_HEAD_INITIALIZER(_dispatch_kevent_data_or.dk_sources),
};
static struct dispatch_kevent_s _dispatch_kevent_data_add = {
	.dk_kevent = {
		.filter = DISPATCH_EVFILT_CUSTOM_ADD,
		.udata = &_dispatch_kevent_data_add,
	},
	.dk_sources = TAILQ_HEAD_INITIALIZER(_dispatch_kevent_data_add.dk_sources),
};

#if TARGET_OS_EMBEDDED
#define DSL_HASH_SIZE  64u // must be a power of two
#else
#define DSL_HASH_SIZE 256u // must be a power of two
#endif
#define DSL_HASH(x) ((x) & (DSL_HASH_SIZE - 1))

DISPATCH_CACHELINE_ALIGN
static TAILQ_HEAD(, dispatch_kevent_s) _dispatch_sources[DSL_HASH_SIZE];
//#define	TAILQ_HEAD(name, type)						\
//struct name {								\
	//struct type *tqh_first;	/* first element */			\
	//struct type **tqh_last;	/* addr of last next element */		\
	//TRACEBUF							\
//}
static dispatch_once_t __dispatch_kevent_init_pred;

static void
_dispatch_kevent_init(void *context DISPATCH_UNUSED)
{
	unsigned int i;
	for (i = 0; i < DSL_HASH_SIZE; i++) {
		TAILQ_INIT(&_dispatch_sources[i]);
	}
	
	TAILQ_INSERT_TAIL(&_dispatch_sources[0],
					  &_dispatch_kevent_data_or, dk_list);
	TAILQ_INSERT_TAIL(&_dispatch_sources[0],
					  &_dispatch_kevent_data_add, dk_list);
	
	_dispatch_source_timer_init();
}

static inline uintptr_t
_dispatch_kevent_hash(uintptr_t ident, short filter)
{
	uintptr_t value;
#if HAVE_MACH
	value = (filter == EVFILT_MACHPORT ? MACH_PORT_INDEX(ident) : ident);
#else
	value = ident;
#endif
	return DSL_HASH(value);
}

static dispatch_kevent_t
_dispatch_kevent_find(uintptr_t ident, short filter)
{
	uintptr_t hash = _dispatch_kevent_hash(ident, filter);
	dispatch_kevent_t dki;
	
	TAILQ_FOREACH(dki, &_dispatch_sources[hash], dk_list) {
		if (dki->dk_kevent.ident == ident && dki->dk_kevent.filter == filter) {
			break;
		}
	}
	return dki;
}

static void
_dispatch_kevent_insert(dispatch_kevent_t dk)
{
	uintptr_t hash = _dispatch_kevent_hash(dk->dk_kevent.ident,
										   dk->dk_kevent.filter);
	
	TAILQ_INSERT_TAIL(&_dispatch_sources[hash], dk, dk_list);
}

// Find existing kevents, and merge any new flags if necessary
static void
_dispatch_kevent_register(dispatch_source_t ds)
{
	dispatch_kevent_t dk;
	typeof(dk->dk_kevent.fflags) new_flags;
	bool do_resume = false;
	
	if (ds->ds_is_installed) {
		return;
	}
	ds->ds_is_installed = true;
	
	dispatch_once_f(&__dispatch_kevent_init_pred,
					NULL, _dispatch_kevent_init);
	
	dk = _dispatch_kevent_find(ds->ds_dkev->dk_kevent.ident,
							   ds->ds_dkev->dk_kevent.filter);
	
	if (dk) {
		// If an existing dispatch kevent is found, check to see if new flags
		// need to be added to the existing kevent
		new_flags = ~dk->dk_kevent.fflags & ds->ds_dkev->dk_kevent.fflags;
		dk->dk_kevent.fflags |= ds->ds_dkev->dk_kevent.fflags;
		free(ds->ds_dkev);
		ds->ds_dkev = dk;
		do_resume = new_flags;
	} else {
		dk = ds->ds_dkev;
		_dispatch_kevent_insert(dk);
		new_flags = dk->dk_kevent.fflags;
		do_resume = true;
	}
	
	TAILQ_INSERT_TAIL(&dk->dk_sources, ds->ds_refs, dr_list);
	
	// Re-register the kevent with the kernel if new flags were added
	// by the dispatch kevent
	if (do_resume) {
		dk->dk_kevent.flags |= EV_ADD;
	}
	if (do_resume || ds->ds_needs_rearm) {
		_dispatch_source_kevent_resume(ds, new_flags);
	}
	(void)dispatch_atomic_or2o(ds, ds_atomic_flags, DSF_ARMED);
}

static bool
_dispatch_kevent_resume(dispatch_kevent_t dk, uint32_t new_flags,
						uint32_t del_flags)
{
	long r;
	switch (dk->dk_kevent.filter) {
		case DISPATCH_EVFILT_TIMER:
		case DISPATCH_EVFILT_CUSTOM_ADD:
		case DISPATCH_EVFILT_CUSTOM_OR:
			// these types not registered with kevent
			return 0;
#if HAVE_MACH
		case EVFILT_MACHPORT:
			return _dispatch_kevent_machport_resume(dk, new_flags, del_flags);
#endif
		case EVFILT_PROC:
			if (dk->dk_kevent.flags & EV_ONESHOT) {
				return 0;
			}
			// fall through
		default:
			r = _dispatch_update_kq(&dk->dk_kevent);
			if (dk->dk_kevent.flags & EV_DISPATCH) {
				dk->dk_kevent.flags &= ~EV_ADD;
			}
			return r;
	}
}


static void
_dispatch_kevent_dispose(dispatch_kevent_t dk)
{
	uintptr_t hash;
	
	switch (dk->dk_kevent.filter) {
		case DISPATCH_EVFILT_TIMER:
		case DISPATCH_EVFILT_CUSTOM_ADD:
		case DISPATCH_EVFILT_CUSTOM_OR:
			// these sources live on statically allocated lists
			return;
#if HAVE_MACH
		case EVFILT_MACHPORT:
			_dispatch_kevent_machport_resume(dk, 0, dk->dk_kevent.fflags);
			break;
#endif
		case EVFILT_PROC:
			if (dk->dk_kevent.flags & EV_ONESHOT) {
				break; // implicitly deleted
			}
			// fall through
		default:
			if (~dk->dk_kevent.flags & EV_DELETE) {
				dk->dk_kevent.flags |= EV_DELETE;
				_dispatch_update_kq(&dk->dk_kevent);
			}
			break;
	}
	
	hash = _dispatch_kevent_hash(dk->dk_kevent.ident,
								 dk->dk_kevent.filter);
	TAILQ_REMOVE(&_dispatch_sources[hash], dk, dk_list);
	free(dk);
}
static void
_dispatch_kevent_unregister(dispatch_source_t ds)
{
	dispatch_kevent_t dk = ds->ds_dkev;
	dispatch_source_refs_t dri;
	uint32_t del_flags, fflags = 0;
	
	ds->ds_dkev = NULL;
	
	TAILQ_REMOVE(&dk->dk_sources, ds->ds_refs, dr_list);
	
	if (TAILQ_EMPTY(&dk->dk_sources)) {
		_dispatch_kevent_dispose(dk);
	} else {
		TAILQ_FOREACH(dri, &dk->dk_sources, dr_list) {
			dispatch_source_t dsi = _dispatch_source_from_refs(dri);
			fflags |= (uint32_t)dsi->ds_pending_data_mask;
		}
		del_flags = (uint32_t)ds->ds_pending_data_mask & ~fflags;
		if (del_flags) {
			dk->dk_kevent.flags |= EV_ADD;
			dk->dk_kevent.fflags = fflags;
			_dispatch_kevent_resume(dk, 0, del_flags);
		}
	}
	
	(void)dispatch_atomic_and2o(ds, ds_atomic_flags, ~DSF_ARMED);
	ds->ds_needs_rearm = false; // re-arm is pointless and bad now
	_dispatch_release(ds); // the retain is done at creation time
}
