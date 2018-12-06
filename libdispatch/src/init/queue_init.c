#include "queue_init.h"


#pragma mark -
#pragma mark dispatch_init


static inline bool
_dispatch_root_queues_init_workq(void)
{
	bool result = false;
#if HAVE_PTHREAD_WORKQUEUES
	bool disable_wq = false;
#if DISPATCH_USE_PTHREAD_POOL
	disable_wq = slowpath(getenv("LIBDISPATCH_DISABLE_KWQ"));
#endif
	int r;
#if HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
	if (!disable_wq) {
		r = pthread_workqueue_setdispatch_np(_dispatch_worker_thread2);
#if !DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
		(void)dispatch_assume_zero(r);
#endif
		result = !r;
	}
#endif // HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK || DISPATCH_USE_PTHREAD_POOL
	if (!result) {
#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
		pthread_workqueue_attr_t pwq_attr;
		if (!disable_wq) {
			r = pthread_workqueue_attr_init_np(&pwq_attr);
			(void)dispatch_assume_zero(r);
		}
#endif
		for (int i = 0; i < DISPATCH_ROOT_QUEUE_COUNT; i++) {
			pthread_workqueue_t pwq = NULL;
			struct dispatch_root_queue_context_s *qc =
			&_dispatch_root_queue_contexts[i];
#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
			if (!disable_wq
#if DISPATCH_NO_BG_PRIORITY
				&& (qc->dgq_wq_priority != DISPATCH_WORKQ_BG_PRIOQUEUE)
#endif
				) {
				r = pthread_workqueue_attr_setqueuepriority_np(&pwq_attr,
															   qc->dgq_wq_priority);
				(void)dispatch_assume_zero(r);
				r = pthread_workqueue_attr_setovercommit_np(&pwq_attr,
															qc->dgq_wq_options & DISPATCH_WORKQ_OPTION_OVERCOMMIT);
				(void)dispatch_assume_zero(r);
				r = pthread_workqueue_create_np(&pwq, &pwq_attr);
				(void)dispatch_assume_zero(r);
				result = result || dispatch_assume(pwq);
			}
#endif
			qc->dgq_kworkqueue = pwq ? pwq : (void*)(~0ul);
		}
#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
		if (!disable_wq) {
			r = pthread_workqueue_attr_destroy_np(&pwq_attr);
			(void)dispatch_assume_zero(r);
		}
#endif
	}
#endif // DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK || DISPATCH_USE_PTHREAD_POOL
#endif // HAVE_PTHREAD_WORKQUEUES
	return result;
}
static inline void
_dispatch_root_queues_init_thread_pool(void)
{
#if DISPATCH_USE_PTHREAD_POOL
	int i;
	for (i = 0; i < DISPATCH_ROOT_QUEUE_COUNT; i++) {
#if TARGET_OS_EMBEDDED
		// some software hangs if the non-overcommitting queues do not
		// overcommit when threads block. Someday, this behavior should apply
		// to all platforms
		if (!(i & 1)) {
			_dispatch_root_queue_contexts[i].dgq_thread_pool_size =
			_dispatch_hw_config.cc_max_active;
		}
#endif
#if USE_MACH_SEM
		// override the default FIFO behavior for the pool semaphores
		kern_return_t kr = semaphore_create(
								mach_task_self(),
								&_dispatch_thread_mediator[i].dsema_port,
								SYNC_POLICY_LIFO, 0);
		DISPATCH_VERIFY_MIG(kr);
		(void)dispatch_assume_zero(kr);
		(void)dispatch_assume(_dispatch_thread_mediator[i].dsema_port);
//#elif USE_POSIX_SEM
//		/* XXXRW: POSIX semaphores don't support LIFO? */
//		int ret = sem_init(&_dispatch_thread_mediator[i].dsema_sem, 0, 0);
//		(void)dispatch_assume_zero(ret);
//#elif USE_FUTEX_SEM
//		int ret = _dispatch_futex_init(
//									   &_dispatch_thread_mediator[i].dsema_futex);
//		(void)dispatch_assume_zero(ret);
#endif
	}
#else
	DISPATCH_CRASH("Thread pool creation failed");
#endif // DISPATCH_USE_PTHREAD_POOL
}
static void
_dispatch_root_queues_init(void *context DISPATCH_UNUSED)
{
	_dispatch_safe_fork = false;
	if (!_dispatch_root_queues_init_workq()) {
		_dispatch_root_queues_init_thread_pool();
	}
}

#define countof(x) (sizeof(x) / sizeof(x[0]))

DISPATCH_EXPORT DISPATCH_NOTHROW
void
libdispatch_init(void)
{
	dispatch_assert(DISPATCH_QUEUE_PRIORITY_COUNT == 4);
	dispatch_assert(DISPATCH_ROOT_QUEUE_COUNT == 8);
	
	dispatch_assert(DISPATCH_QUEUE_PRIORITY_LOW ==
						-DISPATCH_QUEUE_PRIORITY_HIGH);
	dispatch_assert(countof(_dispatch_root_queues) ==
						DISPATCH_ROOT_QUEUE_COUNT);
	dispatch_assert(countof(_dispatch_root_queue_contexts) ==
						DISPATCH_ROOT_QUEUE_COUNT);
#if HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
	dispatch_assert(sizeof(_dispatch_wq2root_queues) /
					sizeof(_dispatch_wq2root_queues[0][0]) ==
					DISPATCH_ROOT_QUEUE_COUNT);
#endif
	
#if DISPATCH_USE_PTHREAD_POOL
	dispatch_assert(countof(_dispatch_thread_mediator) ==
					DISPATCH_ROOT_QUEUE_COUNT);
#endif
	
	dispatch_assert(sizeof(struct dispatch_apply_s) <=
					   ROUND_UP_TO_CACHELINE_SIZE(
									sizeof(struct dispatch_continuation_s)));
	dispatch_assert(sizeof(struct dispatch_source_s) ==
					sizeof(struct dispatch_queue_s) - DISPATCH_QUEUE_CACHELINE_PAD);
	dispatch_assert(sizeof(struct dispatch_queue_s) % DISPATCH_CACHELINE_SIZE
					== 0);
	dispatch_assert(sizeof(struct dispatch_root_queue_context_s) %
					DISPATCH_CACHELINE_SIZE == 0);
	
	
	_dispatch_thread_key_create(&dispatch_queue_key,
								_dispatch_queue_cleanup);
	_dispatch_thread_key_create(&dispatch_sema4_key,
								(void (*)(void *))_dispatch_thread_semaphore_dispose);
	_dispatch_thread_key_create(&dispatch_cache_key,
								_dispatch_cache_cleanup);
	_dispatch_thread_key_create(&dispatch_io_key, NULL);
	_dispatch_thread_key_create(&dispatch_apply_key, NULL);
//	清理函数，用来在线程释放该线程存储的时候被调用。
#if DISPATCH_PERF_MON
	_dispatch_thread_key_create(&dispatch_bcounter_key, NULL);
#endif
	
	//////////////////////////////////////////////////
#if DISPATCH_USE_RESOLVERS // rdar://problem/8541707
	_dispatch_main_q.do_targetq =
				&_dispatch_root_queues[
						DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY];
#endif
	
	_dispatch_thread_setspecific(dispatch_queue_key, &_dispatch_main_q);
	
#if DISPATCH_USE_PTHREAD_ATFORK
	(void)dispatch_assume_zero(pthread_atfork(dispatch_atfork_prepare,
											  dispatch_atfork_parent, dispatch_atfork_child));
//	int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
//	pthread_atfork()在fork()之前调用，当调用fork时，
//	内部创建子进程前在父进程中会调用prepare，
//	内部创建子进程成功后，父进程会调用parent ，
//	子进程会调用child。
#endif
	
	_dispatch_hw_config_init();
	_dispatch_vtable_init();
	_os_object_init();
}


static void
_dispatch_hw_config_init(void)
{
	_dispatch_hw_config.cc_max_active = _dispatch_get_activecpu();
	_dispatch_hw_config.cc_max_logical = _dispatch_get_logicalcpu_max();
	_dispatch_hw_config.cc_max_physical = _dispatch_get_physicalcpu_max();
}

/////////////////////////
DISPATCH_EXPORT DISPATCH_NOTHROW
void
dispatch_atfork_child(void)
{
	void *crash = (void *)0x100;
	size_t i;
	
	if (_dispatch_safe_fork) {
		return;
	}
	
	_dispatch_main_q.dq_items_head = crash;
	_dispatch_main_q.dq_items_tail = crash;
	
	_dispatch_mgr_q.dq_items_head = crash;
	_dispatch_mgr_q.dq_items_tail = crash;
	
	for (i = 0; i < DISPATCH_ROOT_QUEUE_COUNT; i++) {
		_dispatch_root_queues[i].dq_items_head = crash;
		_dispatch_root_queues[i].dq_items_tail = crash;
	}
}
DISPATCH_EXPORT DISPATCH_NOTHROW
void
dispatch_atfork_prepare(void)
{
}

DISPATCH_EXPORT DISPATCH_NOTHROW
void
dispatch_atfork_parent(void)
{
}
