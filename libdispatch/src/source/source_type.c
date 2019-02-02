#include <stdio.h>

#pragma mark -
#pragma mark dispatch_source_types

static void
dispatch_source_type_timer_init(dispatch_source_t ds,
								dispatch_source_type_t type DISPATCH_UNUSED,
								uintptr_t handle DISPATCH_UNUSED,
								unsigned long mask,
								dispatch_queue_t q DISPATCH_UNUSED)
{
	ds->ds_refs = calloc(1ul, sizeof(struct dispatch_timer_source_refs_s));
	if (slowpath(!ds->ds_refs)) return;
	ds->ds_needs_rearm = true;
	ds->ds_is_timer = true;
	ds_timer(ds->ds_refs).flags = mask;
}
const struct dispatch_source_type_s _dispatch_source_type_timer = {
	.ke = {
		.filter = DISPATCH_EVFILT_TIMER,
	},
	.mask = DISPATCH_TIMER_WALL_CLOCK,
	.init = dispatch_source_type_timer_init,
};


const struct dispatch_source_type_s _dispatch_source_type_read = {
	.ke = {
		.filter = EVFILT_READ,
		.flags = EV_DISPATCH,
	},
};


const struct dispatch_source_type_s _dispatch_source_type_write = {
	.ke = {
		.filter = EVFILT_WRITE,
		.flags = EV_DISPATCH,
	},
};

#if DISPATCH_USE_VM_PRESSURE
#if TARGET_IPHONE_SIMULATOR // rdar://problem/9219483
static int _dispatch_ios_simulator_memory_warnings_fd = -1;
static void
_dispatch_ios_simulator_vm_source_init(void *context DISPATCH_UNUSED)
{
	char *e = getenv("IPHONE_SIMULATOR_MEMORY_WARNINGS");
	if (!e) return;
	_dispatch_ios_simulator_memory_warnings_fd = open(e, O_EVTONLY);
	if (_dispatch_ios_simulator_memory_warnings_fd == -1) {
		(void)dispatch_assume_zero(errno);
	}
}
static void
dispatch_source_type_vm_init(dispatch_source_t ds,
							 dispatch_source_type_t type DISPATCH_UNUSED,
							 uintptr_t handle DISPATCH_UNUSED,
							 unsigned long mask,
							 dispatch_queue_t q DISPATCH_UNUSED)
{
	static dispatch_once_t pred;
	dispatch_once_f(&pred, NULL, _dispatch_ios_simulator_vm_source_init);
	ds->ds_dkev->dk_kevent.ident = (mask & DISPATCH_VM_PRESSURE ?
									_dispatch_ios_simulator_memory_warnings_fd : -1);
}
const struct dispatch_source_type_s _dispatch_source_type_vm = {
	.ke = {
		.filter = EVFILT_VNODE,
		.flags = EV_CLEAR,
	},
	.mask = NOTE_ATTRIB,
	.init = dispatch_source_type_vm_init,
};


#else
static void
dispatch_source_type_vm_init(dispatch_source_t ds,
							 dispatch_source_type_t type DISPATCH_UNUSED,
							 uintptr_t handle DISPATCH_UNUSED,
							 unsigned long mask DISPATCH_UNUSED,
							 dispatch_queue_t q DISPATCH_UNUSED)
{
	ds->ds_is_level = false;
}
const struct dispatch_source_type_s _dispatch_source_type_vm = {
	.ke = {
		.filter = EVFILT_VM,
		.flags = EV_DISPATCH,
	},
	.mask = NOTE_VM_PRESSURE,
	.init = dispatch_source_type_vm_init,
};
#endif
#endif


const struct dispatch_source_type_s _dispatch_source_type_proc = {
	.ke = {
		.filter = EVFILT_PROC,
		.flags = EV_CLEAR,
	},
	.mask = NOTE_EXIT|NOTE_FORK|NOTE_EXEC
#if HAVE_DECL_NOTE_SIGNAL
	|NOTE_SIGNAL
#endif
#if HAVE_DECL_NOTE_REAP
	|NOTE_REAP
#endif
	,
};


const struct dispatch_source_type_s _dispatch_source_type_signal = {
	.ke = {
		.filter = EVFILT_SIGNAL,
	},
};


const struct dispatch_source_type_s _dispatch_source_type_vnode = {
	.ke = {
		.filter = EVFILT_VNODE,
		.flags = EV_CLEAR,
	},
	.mask = NOTE_DELETE|NOTE_WRITE|NOTE_EXTEND|NOTE_ATTRIB|NOTE_LINK|
	NOTE_RENAME
#if HAVE_DECL_NOTE_REVOKE
	|NOTE_REVOKE
#endif
#if HAVE_DECL_NOTE_NONE
	|NOTE_NONE
#endif
	,
};


const struct dispatch_source_type_s _dispatch_source_type_vfs = {
	.ke = {
		.filter = EVFILT_FS,
		.flags = EV_CLEAR,
	},
	.mask = VQ_NOTRESP|VQ_NEEDAUTH|VQ_LOWDISK|VQ_MOUNT|VQ_UNMOUNT|VQ_DEAD|
	VQ_ASSIST|VQ_NOTRESPLOCK
#if HAVE_DECL_VQ_UPDATE
	|VQ_UPDATE
#endif
#if HAVE_DECL_VQ_VERYLOWDISK
	|VQ_VERYLOWDISK
#endif
	,
};


const struct dispatch_source_type_s _dispatch_source_type_data_add = {
	.ke = {
		.filter = DISPATCH_EVFILT_CUSTOM_ADD,
	},
};
const struct dispatch_source_type_s _dispatch_source_type_data_or = {
	.ke = {
		.filter = DISPATCH_EVFILT_CUSTOM_OR,
		.flags = EV_CLEAR,
		.fflags = ~0,
	},
};


//////////////////////////
#if HAVE_MACH

static void
dispatch_source_type_mach_send_init(dispatch_source_t ds,
									dispatch_source_type_t type DISPATCH_UNUSED,
									uintptr_t handle DISPATCH_UNUSED, unsigned long mask,
									dispatch_queue_t q DISPATCH_UNUSED)
{
	static dispatch_once_t pred;
	dispatch_once_f(&pred, NULL, _dispatch_mach_notify_source_init);
	if (!mask) {
		// Preserve legacy behavior that (mask == 0) => DISPATCH_MACH_SEND_DEAD
		ds->ds_dkev->dk_kevent.fflags = DISPATCH_MACH_SEND_DEAD;
		ds->ds_pending_data_mask = DISPATCH_MACH_SEND_DEAD;
	}
}
const struct dispatch_source_type_s _dispatch_source_type_mach_send = {
	.ke = {
		.filter = EVFILT_MACHPORT,
		.flags = EV_CLEAR,
	},
	.mask = DISPATCH_MACH_SEND_DEAD|DISPATCH_MACH_SEND_POSSIBLE,
	.init = dispatch_source_type_mach_send_init,
};


static void
dispatch_source_type_mach_recv_init(dispatch_source_t ds,
									dispatch_source_type_t type DISPATCH_UNUSED,
									uintptr_t handle DISPATCH_UNUSED,
									unsigned long mask DISPATCH_UNUSED,
									dispatch_queue_t q DISPATCH_UNUSED)
{
	ds->ds_is_level = false;
}
const struct dispatch_source_type_s _dispatch_source_type_mach_recv = {
	.ke = {
		.filter = EVFILT_MACHPORT,
		.flags = EV_DISPATCH,
		.fflags = DISPATCH_MACH_RECV_MESSAGE,
	},
	.init = dispatch_source_type_mach_recv_init,
};


const struct dispatch_source_type_s _dispatch_source_type_sock = {
#ifdef EVFILT_SOCK
	.ke = {
		.filter = EVFILT_SOCK,
		.flags = EV_CLEAR,
	},
	.mask = NOTE_CONNRESET |  NOTE_READCLOSED | NOTE_WRITECLOSED |
	NOTE_TIMEOUT | NOTE_NOSRCADDR |  NOTE_IFDENIED | NOTE_SUSPEND |
	NOTE_RESUME | NOTE_KEEPALIVE,
#endif
};
