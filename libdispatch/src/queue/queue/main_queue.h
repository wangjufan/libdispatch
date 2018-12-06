//
//  main_queue.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef main_queue_h
#define main_queue_h

#include <stdio.h>

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
