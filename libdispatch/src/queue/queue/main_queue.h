//
//  main_queue.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef main_queue_h
#define main_queue_h

#include <stdio.h>


#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
static dispatch_queue_t _dispatch_queue_wakeup_main(void);
static void _dispatch_main_queue_drain(void);
#endif

#if DISPATCH_COCOA_COMPAT
static unsigned int _dispatch_worker_threads;
static dispatch_once_t _dispatch_main_q_port_pred;
static mach_port_t main_q_port;  //主线程 接收port

static void _dispatch_main_q_port_init(void *ctxt);
#endif


DISPATCH_EXPORT DISPATCH_CONST DISPATCH_WARN_RESULT DISPATCH_NOTHROW
mach_port_t
_dispatch_get_main_queue_port_4CF(void);  //used in cf cfrunloop

__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NOTHROW
void
_dispatch_main_queue_callback_4CF(mach_msg_header_t *msg);


// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
DISPATCH_CACHELINE_ALIGN
struct dispatch_queue_s _dispatch_main_q = {
	.do_vtable = DISPATCH_VTABLE(queue),
#if !DISPATCH_USE_RESOLVERS
	.do_targetq = &_dispatch_root_queues[
						DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY],
#endif
	.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
	.dq_label = "com.apple.main-thread",
	.dq_running = 1,
	.dq_width = 1,
	.dq_serialnum = 1,
};

#endif /* main_queue_h */
