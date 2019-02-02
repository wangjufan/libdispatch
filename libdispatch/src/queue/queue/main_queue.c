//
//  main_queue.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "main_queue.h"

#pragma mark -
#pragma mark dispatch_main_queue

static bool _dispatch_program_is_probably_callback_driven;

#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
static bool main_q_is_draining;

// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
DISPATCH_NOINLINE
static void
_dispatch_queue_set_mainq_drain_state(bool arg)
{
	main_q_is_draining = arg;
}
#endif

#if DISPATCH_COCOA_COMPAT
static void
_dispatch_main_q_port_init(void *ctxt DISPATCH_UNUSED)
{
	kern_return_t kr;
	_dispatch_safe_fork = false;

	kr = mach_port_allocate(mach_task_self(),
							MACH_PORT_RIGHT_RECEIVE,
							&main_q_port);
	DISPATCH_VERIFY_MIG(kr);
	(void)dispatch_assume_zero(kr);
	
	kr = mach_port_insert_right(mach_task_self(), main_q_port, main_q_port, MACH_MSG_TYPE_MAKE_SEND);
	DISPATCH_VERIFY_MIG(kr);
	(void)dispatch_assume_zero(kr);
	
	_dispatch_program_is_probably_callback_driven = true;
}
mach_port_t
_dispatch_get_main_queue_port_4CF(void)
{
	dispatch_once_f(&_dispatch_main_q_port_pred, NULL,
					_dispatch_main_q_port_init);
	return main_q_port;
}
void
_dispatch_main_queue_callback_4CF(mach_msg_header_t *msg DISPATCH_UNUSED)
{
	if (main_q_is_draining) {
		return;
	}
	_dispatch_queue_set_mainq_drain_state(true);
	_dispatch_main_queue_drain();
	_dispatch_queue_set_mainq_drain_state(false);
}
#endif

//#if DISPATCH_LINUX_COMPAT
//int
//dispatch_get_main_queue_handle_np()
//{
//	dispatch_once_f(&_dispatch_main_q_eventfd_pred, NULL,
//					_dispatch_main_q_eventfd_init);
//	return main_q_eventfd;
//}
//void
//dispatch_main_queue_drain_np()
//{
//	if (!pthread_main_np()) {
//		DISPATCH_CLIENT_CRASH("dispatch_main_queue_drain_np() must be called on "
//							  "the main thread");
//	}
//
//	if (main_q_is_draining) {
//		return;
//	}
//	_dispatch_queue_set_mainq_drain_state(true);
//	_dispatch_main_queue_drain();
//	_dispatch_queue_set_mainq_drain_state(false);
//}
//
//static void
//_dispatch_eventfd_write(int fd, uint64_t value)
//{
//	ssize_t result;
//	do {
//		result = write(fd, &value, sizeof(value));
//	} while (result == -1 && errno == EINTR);
//	dispatch_assert(result == sizeof(value) ||
//					(result == -1 && errno == EAGAIN));
//}
//
//static uint64_t
//_dispatch_eventfd_read(int fd)
//{
//	uint64_t value = 0;
//	ssize_t result;
//	do {
//		result = read(fd, &value, sizeof(value));
//	} while (result == -1 && errno == EINTR);
//	dispatch_assert(result == sizeof(value) ||
//					(result == -1 && errno == EAGAIN));
//	return value;
//}
//
//static
//void _dispatch_main_q_eventfd_init(void *ctxt DISPATCH_UNUSED)
//{
//	_dispatch_safe_fork = false;
//	main_q_eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
//	(void)dispatch_assume(main_q_eventfd != -1);
//	_dispatch_program_is_probably_callback_driven = true;
//}
//#endif
//
//#if DISPATCH_LINUX_COMPAT
//static void
//_dispatch_queue_cleanup_tsd(void)
//{
//	struct tsd_entry_s {
//		pthread_key_t thread_key;
//		void (*destructor)(void *);
//	} entries[] = {
//		{dispatch_queue_key, _dispatch_queue_cleanup},
//		{dispatch_sema4_key,
//			(void (*)(void *))_dispatch_thread_semaphore_dispose},
//		{dispatch_cache_key, _dispatch_cache_cleanup},
//	};
//	for (unsigned i = 0; i < countof(entries); ++i) {
//		void *val = _dispatch_thread_getspecific(entries[i].thread_key);
//		if (val) {
//			entries[i].destructor(val);
//			_dispatch_thread_setspecific(entries[i].thread_key, NULL);
//		}
//	}
//}
//#endif  // DISPATCH_LINUX_COMPAT

void
dispatch_main(void)
{
	if (pthread_main_np()) {
		_dispatch_program_is_probably_callback_driven = true;
#if !DISPATCH_LINUX_COMPAT
		pthread_exit(NULL);
		DISPATCH_CRASH("pthread_exit() returned");
#else
		_dispatch_queue_cleanup_tsd();
		sigset_t mask;
		(void)dispatch_assume_zero(sigfillset(&mask));
		sigsuspend(&mask);
		DISPATCH_CRASH("sigsuspend() returned");
#endif
	}
	DISPATCH_CLIENT_CRASH("dispatch_main() must be called on the main thread");
}

DISPATCH_NOINLINE DISPATCH_NORETURN
static void
_dispatch_sigsuspend(void)
{
	static sigset_t mask;
	sigemptyset(&mask);
	
#if DISPATCH_COCOA_COMPAT
	// Do not count the signal handling thread as a worker thread
	(void)dispatch_atomic_dec(&_dispatch_worker_threads);
#endif
	for (;;) {
		sigsuspend(&mask);
	}
}

DISPATCH_NORETURN
static void
_dispatch_sig_thread(void *ctxt DISPATCH_UNUSED)
{
	// never returns, so burn bridges behind us
	_dispatch_clear_stack(0);
	_dispatch_sigsuspend();
}

DISPATCH_NOINLINE
static void
_dispatch_queue_cleanup2(void)
{
	(void)dispatch_atomic_dec(&_dispatch_main_q.dq_running);
	
	dispatch_atomic_release_barrier();
	if (dispatch_atomic_sub2o(&_dispatch_main_q, do_suspend_cnt,
							  DISPATCH_OBJECT_SUSPEND_LOCK) == 0) {
		_dispatch_wakeup(&_dispatch_main_q);
	}
	
	// overload the "probably" variable to mean that dispatch_main() or
	// similar non-POSIX API was called
	// this has to run before the DISPATCH_COCOA_COMPAT below
	if (_dispatch_program_is_probably_callback_driven) {
		dispatch_async_f(_dispatch_get_root_queue(0, true), NULL,
						 _dispatch_sig_thread);
		sleep(1); // workaround 6778970
	}
	
#if DISPATCH_COCOA_COMPAT
	dispatch_once_f(&_dispatch_main_q_port_pred, NULL,
					_dispatch_main_q_port_init);
	
	mach_port_t mp = main_q_port;
	kern_return_t kr;
	main_q_port = 0;
	
	if (mp) {
		kr = mach_port_deallocate(mach_task_self(), mp);
//		Decrement the target port right's user reference count.
//	The mach_port_deallocate function releases a user reference for a right. It is an alternate form of mach_port_mod_refs that allows a task to release a user reference for a send or send-once right without failing if the port has died and the right is now actually a dead name.
//
//	If name denotes a dead name, send right, or send-once right, then the right loses one user reference. If it only had one user reference, then the right is destroyed. If name does not denote an element in the port name space, the function returns success.
		DISPATCH_VERIFY_MIG(kr);
		(void)dispatch_assume_zero(kr);
		kr = mach_port_mod_refs(mach_task_self(), mp,
								MACH_PORT_RIGHT_RECEIVE, -1);
		DISPATCH_VERIFY_MIG(kr);
		(void)dispatch_assume_zero(kr);
	}
#endif
//#if DISPATCH_LINUX_COMPAT
//	dispatch_once_f(&_dispatch_main_q_eventfd_pred, NULL,
//					_dispatch_main_q_eventfd_init);
//	int fd = main_q_eventfd;
//	main_q_eventfd = -1;
//
//	if (fd != -1) {
//		close(fd);
//	}
//#endif
}

static void
_dispatch_queue_cleanup(void *ctxt)
{
	if (ctxt == &_dispatch_main_q) {
		return _dispatch_queue_cleanup2();
	}
	// POSIX defines that destructors are only called if 'ctxt' is non-null
	DISPATCH_CRASH("Premature thread exit while a dispatch queue is running");
}


