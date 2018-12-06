
/*
 * IMPORTANT: This header file describes INTERNAL interfaces to libdispatch
 * which are subject to change in future releases of Mac OS X. Any applications
 * relying on these interfaces WILL break.
 */

#ifndef __DISPATCH_SOURCE_INTERNAL__
#define __DISPATCH_SOURCE_INTERNAL__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include <dispatch/base.h> // for HeaderDoc
#endif

// NOTE: dispatch_source_mach_send_flags_t and dispatch_source_mach_recv_flags_t
//       bit values must not overlap as they share the same kevent fflags !

/*!
 * @enum dispatch_source_mach_send_flags_t
 *
 * @constant DISPATCH_MACH_SEND_DELETED
 * Port-deleted notification. Disabled for source registration.
 */
enum {
	DISPATCH_MACH_SEND_DELETED = 0x4,
};
/*!
 * @enum dispatch_source_mach_recv_flags_t
 *
 * @constant DISPATCH_MACH_RECV_MESSAGE
 * Receive right has pending messages
 *
 * @constant DISPATCH_MACH_RECV_NO_SENDERS
 * Receive right has no more senders. TODO <rdar://problem/8132399>
 */
enum {
	DISPATCH_MACH_RECV_MESSAGE = 0x2,
	DISPATCH_MACH_RECV_NO_SENDERS = 0x10,
};

enum {
	DISPATCH_TIMER_WALL_CLOCK = 0x4,
};

#define DISPATCH_EVFILT_TIMER		(-EVFILT_SYSCOUNT - 1)
#define DISPATCH_EVFILT_CUSTOM_ADD	(-EVFILT_SYSCOUNT - 2)
#define DISPATCH_EVFILT_CUSTOM_OR	(-EVFILT_SYSCOUNT - 3)
#define DISPATCH_EVFILT_SYSCOUNT	( EVFILT_SYSCOUNT + 3)

#define DISPATCH_TIMER_INDEX_WALL	0
#define DISPATCH_TIMER_INDEX_MACH	1
#define DISPATCH_TIMER_INDEX_DISARM	2


#define _dispatch_ptr2wref(ptr) (~(uintptr_t)(ptr))
#define _dispatch_wref2ptr(ref) ((void*)~(ref))
#define _dispatch_source_from_refs(dr) \
		((dispatch_source_t)_dispatch_wref2ptr((dr)->dr_source_wref))
#define ds_timer(dr) \
		(((struct dispatch_timer_source_refs_s *)(dr))->_ds_timer)

// ds_atomic_flags bits
#define DSF_CANCELED 1u // cancellation has been requested
#define DSF_ARMED 2u // source is armed


void _dispatch_source_xref_dispose(dispatch_source_t ds);
void _dispatch_mach_notify_source_init(void *context);
dispatch_queue_t _dispatch_source_invoke(dispatch_source_t ds);
void _dispatch_source_dispose(dispatch_source_t ds);
bool _dispatch_source_probe(dispatch_source_t ds);
size_t _dispatch_source_debug(dispatch_source_t ds, char* buf, size_t bufsiz);

#endif /* __DISPATCH_SOURCE_INTERNAL__ */

