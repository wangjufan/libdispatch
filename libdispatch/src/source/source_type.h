//
//  source_type.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#ifndef source_type_h
#define source_type_h


// Source state which may contain references to the source object
// Separately allocated so that 'leaks' can see sources <rdar://problem/9050566>
struct dispatch_source_refs_s {
	TAILQ_ENTRY(dispatch_source_refs_s) dr_list;
	uintptr_t dr_source_wref; // "weak" backref to dispatch_source_t
	dispatch_function_t ds_handler_func;
	void *ds_handler_ctxt;
	void *ds_cancel_handler;
	void *ds_registration_handler;
};

//#define	TAILQ_ENTRY(type)						\
//struct {								\
//struct type *tqe_next;	/* next element */			\
//struct type **tqe_prev;	/* address of previous next element */	\
//TRACEBUF							\
//}

typedef struct dispatch_source_refs_s *dispatch_source_refs_t;


DISPATCH_CLASS_DECL(source);
struct dispatch_source_s {
	DISPATCH_STRUCT_HEADER(source);
	DISPATCH_QUEUE_HEADER;
	// Instruments always copies DISPATCH_QUEUE_MIN_LABEL_SIZE, which is 64,
	// so the remainder of the structure must be big enough
	union {
		char _ds_pad[DISPATCH_QUEUE_MIN_LABEL_SIZE];
		struct {
			char dq_label[8];
			dispatch_kevent_t ds_dkev;
			dispatch_source_refs_t ds_refs;
			unsigned int ds_atomic_flags;
			unsigned int
		ds_is_level:1,
		ds_is_adder:1,
		ds_is_installed:1,
		ds_needs_rearm:1,
		ds_is_timer:1,
		ds_cancel_is_block:1,
		ds_handler_is_block:1,
		ds_registration_is_block:1;
			unsigned long ds_data;
			unsigned long ds_pending_data;
			unsigned long ds_pending_data_mask;
			unsigned long ds_ident_hack;
		};
	};
};

struct dispatch_source_type_s {
	struct kevent ke;
	uint64_t mask;
	void (*init)( dispatch_source_t ds,
				 dispatch_source_type_t type,
				 uintptr_t handle,
				 unsigned long mask,
				 dispatch_queue_t q);
};

/*!
 * @header
 * The dispatch framework provides a suite of interfaces for monitoring low-
 * level system objects (file descriptors, Mach ports, signals, VFS nodes, etc.)
 * for activity and automatically submitting event handler blocks to dispatch
 * queues when such activity occurs.
 *
 * This suite of interfaces is known as the Dispatch Source API.
 */

/*!
 * @typedef dispatch_source_t
 *
 * @abstract
 * Dispatch sources are used to automatically submit event handler blocks to
 * dispatch queues in response to external events.
 */
DISPATCH_DECL(dispatch_source);

/*!
 * @typedef dispatch_source_type_t
 *
 * @abstract
 * Constants of this type represent the class of low-level system object that
 * is being monitored by the dispatch source. Constants of this type are
 * passed as a parameter to dispatch_source_create() and determine how the
 * handle argument is interpreted (i.e. as a file descriptor, mach port,
 * signal number, process identifer, etc.), and how the mask arugment is
 * interpreted.
 */
typedef const struct dispatch_source_type_s *dispatch_source_type_t;

/*!
 * @const DISPATCH_SOURCE_TYPE_DATA_ADD
 * @discussion A dispatch source that coalesces data obtained via calls to
 * dispatch_source_merge_data(). An ADD is used to coalesce the data.
 * The handle is unused (pass zero for now).
 * The mask is unused (pass zero for now).
 */
#define DISPATCH_SOURCE_TYPE_DATA_ADD (&_dispatch_source_type_data_add)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_data_add;

/*!
 * @const DISPATCH_SOURCE_TYPE_DATA_OR
 * @discussion A dispatch source that coalesces data obtained via calls to
 * dispatch_source_merge_data(). A logical OR is used to coalesce the data.
 * The handle is unused (pass zero for now).
 * The mask is used to perform a logical AND with the value passed to
 * dispatch_source_merge_data().
 */
#define DISPATCH_SOURCE_TYPE_DATA_OR (&_dispatch_source_type_data_or)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_data_or;

/*!
 * @const DISPATCH_SOURCE_TYPE_MACH_SEND
 * @discussion A dispatch source that monitors a Mach port for dead name
 * notifications (send right no longer has any corresponding receive right).
 * The handle is a Mach port with a send or send-once right (mach_port_t).
 * The mask is a mask of desired events from dispatch_source_mach_send_flags_t.
 */
#define DISPATCH_SOURCE_TYPE_MACH_SEND (&_dispatch_source_type_mach_send)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_mach_send;

/*!
 * @const DISPATCH_SOURCE_TYPE_MACH_RECV
 * @discussion A dispatch source that monitors a Mach port for pending messages.
 * The handle is a Mach port with a receive right (mach_port_t).
 * The mask is unused (pass zero for now).
 */
#define DISPATCH_SOURCE_TYPE_MACH_RECV (&_dispatch_source_type_mach_recv)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_mach_recv;

/*!
 * @const DISPATCH_SOURCE_TYPE_PROC
 * @discussion A dispatch source that monitors an external process for events
 * defined by dispatch_source_proc_flags_t.
 * The handle is a process identifier (pid_t).
 * The mask is a mask of desired events from dispatch_source_proc_flags_t.
 */
#define DISPATCH_SOURCE_TYPE_PROC (&_dispatch_source_type_proc)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_proc;

/*!
 * @const DISPATCH_SOURCE_TYPE_READ
 * @discussion A dispatch source that monitors a file descriptor for pending
 * bytes available to be read.
 * The handle is a file descriptor (int).
 * The mask is unused (pass zero for now).
 */
#define DISPATCH_SOURCE_TYPE_READ (&_dispatch_source_type_read)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_read;

/*!
 * @const DISPATCH_SOURCE_TYPE_SIGNAL
 * @discussion A dispatch source that monitors the current process for signals.
 * The handle is a signal number (int).
 * The mask is unused (pass zero for now).
 */
#define DISPATCH_SOURCE_TYPE_SIGNAL (&_dispatch_source_type_signal)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_signal;

/*!
 * @const DISPATCH_SOURCE_TYPE_TIMER
 * @discussion A dispatch source that submits the event handler block based
 * on a timer.
 * The handle is unused (pass zero for now).
 * The mask is unused (pass zero for now).
 */
#define DISPATCH_SOURCE_TYPE_TIMER (&_dispatch_source_type_timer)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_timer;

/*!
 * @const DISPATCH_SOURCE_TYPE_VNODE
 * @discussion A dispatch source that monitors a file descriptor for events
 * defined by dispatch_source_vnode_flags_t.
 * The handle is a file descriptor (int).
 * The mask is a mask of desired events from dispatch_source_vnode_flags_t.
 */
#define DISPATCH_SOURCE_TYPE_VNODE (&_dispatch_source_type_vnode)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_vnode;

/*!
 * @const DISPATCH_SOURCE_TYPE_WRITE
 * @discussion A dispatch source that monitors a file descriptor for available
 * buffer space to write bytes.
 * The handle is a file descriptor (int).
 * The mask is unused (pass zero for now).
 */
#define DISPATCH_SOURCE_TYPE_WRITE (&_dispatch_source_type_write)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT
const struct dispatch_source_type_s _dispatch_source_type_write;

/*!
 * @typedef dispatch_source_mach_send_flags_t
 * Type of dispatch_source_mach_send flags
 *
 * @constant DISPATCH_MACH_SEND_DEAD
 * The receive right corresponding to the given send right was destroyed.
 */
#define DISPATCH_MACH_SEND_DEAD	0x1

typedef unsigned long dispatch_source_mach_send_flags_t;

/*!
 * @typedef dispatch_source_proc_flags_t
 * Type of dispatch_source_proc flags
 *
 * @constant DISPATCH_PROC_EXIT
 * The process has exited (perhaps cleanly, perhaps not).
 *
 * @constant DISPATCH_PROC_FORK
 * The process has created one or more child processes.
 *
 * @constant DISPATCH_PROC_EXEC
 * The process has become another executable image via
 * exec*() or posix_spawn*().
 *
 * @constant DISPATCH_PROC_SIGNAL
 * A Unix signal was delivered to the process.
 */
#define DISPATCH_PROC_EXIT		0x80000000
#define DISPATCH_PROC_FORK		0x40000000
#define DISPATCH_PROC_EXEC		0x20000000
#define DISPATCH_PROC_SIGNAL	0x08000000

typedef unsigned long dispatch_source_proc_flags_t;

/*!
 * @typedef dispatch_source_vnode_flags_t
 * Type of dispatch_source_vnode flags
 *
 * @constant DISPATCH_VNODE_DELETE
 * The filesystem object was deleted from the namespace.
 *
 * @constant DISPATCH_VNODE_WRITE
 * The filesystem object data changed.
 *
 * @constant DISPATCH_VNODE_EXTEND
 * The filesystem object changed in size.
 *
 * @constant DISPATCH_VNODE_ATTRIB
 * The filesystem object metadata changed.
 *
 * @constant DISPATCH_VNODE_LINK
 * The filesystem object link count changed.
 *
 * @constant DISPATCH_VNODE_RENAME
 * The filesystem object was renamed in the namespace.
 *
 * @constant DISPATCH_VNODE_REVOKE
 * The filesystem object was revoked.
 */

#define DISPATCH_VNODE_DELETE	0x1
#define DISPATCH_VNODE_WRITE	0x2
#define DISPATCH_VNODE_EXTEND	0x4
#define DISPATCH_VNODE_ATTRIB	0x8
#define DISPATCH_VNODE_LINK		0x10
#define DISPATCH_VNODE_RENAME	0x20
#define DISPATCH_VNODE_REVOKE	0x40

typedef unsigned long dispatch_source_vnode_flags_t;
#endif /* source_type_h */
