//
//  manager_queue.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef manager_queue_h
#define manager_queue_h

#include <stdio.h>

// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
DISPATCH_CACHELINE_ALIGN
struct dispatch_queue_s _dispatch_mgr_q = {
	.do_vtable = DISPATCH_VTABLE(queue_mgr),
	.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	.do_suspend_cnt = DISPATCH_OBJECT_SUSPEND_LOCK,
	.do_targetq = &_dispatch_root_queues[
						DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY],
	.dq_label = "com.apple.libdispatch-manager",
	.dq_width = 1,
	.dq_serialnum = 2,
};

bool
_dispatch_mgr_wakeup(dispatch_queue_t dq);// public
long
_dispatch_update_kq(const struct kevent *kev);
static int
_dispatch_get_kq(void);
static void
_dispatch_get_kq_init(void *context DISPATCH_UNUSED);


////////////////////
DISPATCH_NORETURN
dispatch_queue_t
_dispatch_mgr_thread(dispatch_queue_t dq DISPATCH_UNUSED);// public
 
#endif /* manager_queue_h */

