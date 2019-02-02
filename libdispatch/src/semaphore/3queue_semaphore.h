//
//  queue_semaphore.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef queue_semaphore_h
#define queue_semaphore_h

#include <stdio.h>

#pragma mark -
#pragma mark dispatch_root_queue


#if DISPATCH_USE_PTHREAD_POOL
static struct dispatch_semaphore_s _dispatch_thread_mediator[] = {
	[DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	[DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	
	[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	
	[DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	[DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	
	[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
	[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY] = {
		.do_vtable = DISPATCH_VTABLE(semaphore),
		.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
		.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	},
};
#endif

#define MAX_THREAD_COUNT 255

#endif /* queue_semaphore_h */
