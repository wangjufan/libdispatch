//
//  queue_init.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef queue_init_h
#define queue_init_h

#include <stdio.h>

void
libdispatch_init(void);// entrance for lib
/* pthreads magic */

static void
_dispatch_root_queues_init(void *context DISPATCH_UNUSED)
{
	_dispatch_safe_fork = false;
	if (!_dispatch_root_queues_init_workq()) {
		_dispatch_root_queues_init_thread_pool();
	}
} //called by _dispatch_queue_wakeup_global_slow

DISPATCH_EXPORT DISPATCH_NOTHROW void dispatch_atfork_prepare(void);
DISPATCH_EXPORT DISPATCH_NOTHROW void dispatch_atfork_parent(void);
DISPATCH_EXPORT DISPATCH_NOTHROW void dispatch_atfork_child(void);


#endif /* queue_init_h */
