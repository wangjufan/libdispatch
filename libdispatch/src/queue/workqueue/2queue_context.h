//
//  queue_context.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef queue_context_h
#define queue_context_h

#include <stdio.h>

#pragma mark -
#pragma mark dispatch_root_queue

struct dispatch_root_queue_context_s {
	union {
		struct {
			unsigned int volatile dgq_pending;
#if HAVE_PTHREAD_WORKQUEUES
			int dgq_wq_priority, dgq_wq_options;
#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK || DISPATCH_USE_PTHREAD_POOL
			pthread_workqueue_t dgq_kworkqueue;
#endif
#endif // HAVE_PTHREAD_WORKQUEUES
			
#if DISPATCH_USE_PTHREAD_POOL
			dispatch_semaphore_t dgq_thread_mediator;
			uint32_t dgq_thread_pool_size;
#endif
		};
		char _dgq_pad[DISPATCH_CACHELINE_SIZE];
	};
};

DISPATCH_CACHELINE_ALIGN
static struct dispatch_root_queue_context_s _dispatch_root_queue_contexts[] = {
	[DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_LOW_PRIOQUEUE,
		.dgq_wq_options = 0,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
														  DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	[DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_LOW_PRIOQUEUE,
		.dgq_wq_options = DISPATCH_WORKQ_OPTION_OVERCOMMIT,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
														  DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	
	
	[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_DEFAULT_PRIOQUEUE,
		.dgq_wq_options = 0,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
							DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_DEFAULT_PRIOQUEUE,
		.dgq_wq_options = DISPATCH_WORKQ_OPTION_OVERCOMMIT,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
					DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	
	
	[DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_HIGH_PRIOQUEUE,
		.dgq_wq_options = 0,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
														  DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	[DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_HIGH_PRIOQUEUE,
		.dgq_wq_options = DISPATCH_WORKQ_OPTION_OVERCOMMIT,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
														  DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	
	
	[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_BG_PRIOQUEUE,
		.dgq_wq_options = 0,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
														  DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
	[DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY] = {{{
#if HAVE_PTHREAD_WORKQUEUES
		.dgq_wq_priority = DISPATCH_WORKQ_BG_PRIOQUEUE,
		.dgq_wq_options = DISPATCH_WORKQ_OPTION_OVERCOMMIT,
#endif
#if DISPATCH_USE_PTHREAD_POOL
		.dgq_thread_mediator = &_dispatch_thread_mediator[
														  DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY],
		.dgq_thread_pool_size = MAX_THREAD_COUNT,
#endif
	}}},
};

#endif /* queue_context_h */
