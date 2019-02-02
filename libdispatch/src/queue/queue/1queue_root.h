//
//  queue_root.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef queue_root_h
#define queue_root_h

#include <stdio.h>

#pragma mark -
#pragma mark dispatch_root_queue


/*!
 * @function dispatch_get_current_queue
 *
 * @abstract
 * Returns the queue on which the currently executing block is running.
 *
 * @discussion
 * Returns the queue on which the currently executing block is running.
 *
 * When dispatch_get_current_queue() is called outside of the context of a
 * submitted block, it will return the default concurrent queue.
 *
 * Recommended for debugging and logging purposes only:
 * The code must not make any assumptions about the queue returned, unless it
 * is one of the global queues or a queue the code has itself created.
 * The code must not assume that synchronous execution onto a queue is safe
 * from deadlock if that queue is not the one returned by
 * dispatch_get_current_queue().
 *
 * When dispatch_get_current_queue() is called on the main thread, it may
 * or may not return the same value as dispatch_get_main_queue(). Comparing
 * the two is not a valid way to test whether code is executing on the
 * main thread.
 *
 * @result
 * Returns the current queue.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_PURE DISPATCH_WARN_RESULT DISPATCH_NOTHROW
dispatch_queue_t
dispatch_get_current_queue(void);


typedef long dispatch_queue_priority_t;

/*!
 * @function dispatch_get_global_queue
 *
 * @abstract
 * Returns a well-known global concurrent queue of a given priority level.
 *
 * @discussion
 * The well-known global concurrent queues may not be modified. Calls to
 * dispatch_suspend(), dispatch_resume(), dispatch_set_context(), etc., will
 * have no effect when used with queues returned by this function.
 *
 * @param priority
 * A priority defined in dispatch_queue_priority_t
 *
 * @param flags
 * Reserved for future use. Passing any value other than zero may result in
 * a NULL return value.
 *
 * @result
 * Returns the requested global queue.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_CONST DISPATCH_WARN_RESULT DISPATCH_NOTHROW
dispatch_queue_t
dispatch_get_global_queue(dispatch_queue_priority_t priority,
						  unsigned long flags);


// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
// dq_running is set to 2 so that barrier operations go through the slow path
DISPATCH_CACHELINE_ALIGN
struct dispatch_queue_s _dispatch_root_queues[] = {
	[DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
									DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY],
		.dq_label = "com.apple.root.low-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 4,
	},
	[DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
								DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY],
		.dq_label = "com.apple.root.low-overcommit-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 5,
	},
	[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
								DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY],
		.dq_label = "com.apple.root.default-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 6,
	},
	[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,//INT_MAX
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
							DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY],
		.dq_label = "com.apple.root.default-overcommit-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 7,
	},
	[DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
										DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY],
		.dq_label = "com.apple.root.high-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 8,
	},
	[DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
							DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY],
		.dq_label = "com.apple.root.high-overcommit-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 9,
	},
	[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
							DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY],
		.dq_label = "com.apple.root.background-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 10,
	},
	[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(queue_root),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
		.do_ctxt = &_dispatch_root_queue_contexts[
						DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY],
		.dq_label = "com.apple.root.background-overcommit-priority",
		.dq_running = 2,
		.dq_width = UINT32_MAX,
		.dq_serialnum = 11,
	},
};

//#if HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
//static const dispatch_queue_t _dispatch_wq2root_queues[][2] = {
//	[DISPATCH_WORKQ_LOW_PRIOQUEUE]
//	[0] =&_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY],
//
//	[DISPATCH_WORKQ_LOW_PRIOQUEUE]
//	[DISPATCH_WORKQ_ADDTHREADS_OPTION_OVERCOMMIT]
//		= &_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY],
//
//
//	[DISPATCH_WORKQ_DEFAULT_PRIOQUEUE]
//	[0] = &_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY],
//
//	[DISPATCH_WORKQ_DEFAULT_PRIOQUEUE]
//	[DISPATCH_WORKQ_ADDTHREADS_OPTION_OVERCOMMIT] =
//		&_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY],
//
//
//	[DISPATCH_WORKQ_HIGH_PRIOQUEUE]
//	[0] = &_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY],
//
//	[DISPATCH_WORKQ_HIGH_PRIOQUEUE]
//	[DISPATCH_WORKQ_ADDTHREADS_OPTION_OVERCOMMIT] =
//	&_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY],
//
//
//	[DISPATCH_WORKQ_BG_PRIOQUEUE][0] =
//	&_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY],
//
//	[DISPATCH_WORKQ_BG_PRIOQUEUE]
//	[DISPATCH_WORKQ_ADDTHREADS_OPTION_OVERCOMMIT] =
//	&_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY],
//};
//#endif // HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
//
//#endif /* queue_root_h */
