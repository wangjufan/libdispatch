//
//  manager_queue.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "manager_queue.h"

#pragma mark -
#pragma mark dispatch_manager_queue

static unsigned int _dispatch_select_workaround;

static fd_set _dispatch_rfds;
static fd_set _dispatch_wfds;

static void **_dispatch_rfd_ptrs;
static void **_dispatch_wfd_ptrs;

static int _dispatch_kq;


//const static int FD_NUM = 2 // 要监视多少个文件描述符
//// kqueue的事件结构体，不需要直接操作
//struct kevent changes[FD_NUM]; // 要监视的事件列表
//struct kevent events[FD_NUM]; // kevent返回的事件列表（参考后面的kevent函数）
//
//int stdin_fd = STDIN_FILENO;
//int stdout_fd = STDOUT_FILENO;

//int kq = kqueue(); // kqueue对象

//// 在changes列表中注册标准输入流的读事件 以及 标准输出流的写事件
//// 最后一个参数可以是任意的附加数据（void * 类型），在这里给事件附上了当前的文件描述符，后面会用到
//EV_SET(&changes[0], stdin_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &stdin_fd);
//EV_SET(&changes[1], stdout_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &stdin_fd);
//
//// 进行kevent函数调用，如果changes列表里有任何就绪的fd，则把该事件对应的结构体放进events列表里面
//// 返回值是这次调用得到了几个就绪的事件 (nev = number of events)
//int nev = kevent(kq, changes, FD_NUM, events, FD_NUM, NULL); // 已经就绪的文件描述符数量
//for(int i=0; i<nev; i++){
//	struct kevent event = events[i]; // 一个个取出已经就绪的事件
//
//	int ready_fd = *((int *)event.udata); // 从附加数据里面取回文件描述符的值
//	if( ready_fd == stdin_fd ){
//		// 读取ready_fd
//	}else if( ready_fd == stdin_fd ){
//		// 写入ready_fd
//	}
//}

static void
_dispatch_get_kq_init(void *context DISPATCH_UNUSED)
{
	static const struct kevent kev = {
		.ident = 1,
		.filter = EVFILT_USER,
		.flags = EV_ADD|EV_CLEAR,
	};
	
	_dispatch_safe_fork = false;
	_dispatch_kq = kqueue();
	if (_dispatch_kq == -1) {
		DISPATCH_CLIENT_CRASH("kqueue() create failed: "
							  "probably out of file descriptors");
	} else if (dispatch_assume(_dispatch_kq < FD_SETSIZE)) {
		// in case we fall back to select()
		FD_SET(_dispatch_kq, &_dispatch_rfds);
	}
	
	(void)dispatch_assume_zero(kevent(_dispatch_kq, &kev, 1, NULL, 0, NULL));
	_dispatch_queue_push(_dispatch_mgr_q.do_targetq, &_dispatch_mgr_q);
}
static int
_dispatch_get_kq(void) {
	static dispatch_once_t pred;
	dispatch_once_f(&pred, NULL, _dispatch_get_kq_init);
	return _dispatch_kq;
}


long
_dispatch_update_kq(const struct kevent *kev)
{
	int rval;
	struct kevent kev_copy = *kev;
	// This ensures we don't get a pending kevent back while registering
	// a new kevent
	kev_copy.flags |= EV_RECEIPT;
	
	if (_dispatch_select_workaround && (kev_copy.flags & EV_DELETE)) {
		// Only executed on manager queue
		switch (kev_copy.filter) {
			case EVFILT_READ:
				if (kev_copy.ident < FD_SETSIZE &&
					FD_ISSET((int)kev_copy.ident, &_dispatch_rfds)) {
					FD_CLR((int)kev_copy.ident, &_dispatch_rfds);
					_dispatch_rfd_ptrs[kev_copy.ident] = 0;
					(void)dispatch_atomic_dec(&_dispatch_select_workaround);
					return 0;
				}
				break;
			case EVFILT_WRITE:
				if (kev_copy.ident < FD_SETSIZE &&
					FD_ISSET((int)kev_copy.ident, &_dispatch_wfds)) {
					FD_CLR((int)kev_copy.ident, &_dispatch_wfds);
					_dispatch_wfd_ptrs[kev_copy.ident] = 0;
					(void)dispatch_atomic_dec(&_dispatch_select_workaround);
					return 0;
				}
				break;
			default:
				break;
		}
	}
	
retry:
	
//	int
//	kevent(int kq, const struct kevent *changelist, size_t nchanges,
//		   struct kevent *eventlist, size_t nevents,
//		   const struct timespec *timeout);
//	kevent() is used to register events with the queue, and return any pending events to the user.
	
//	changelist is a pointer to an array of kevent
//	structures, as defined in <sys/event.h>.  All changes contained in the
//	changelist are applied before any pending events are read from the queue.
//	nchanges gives the size of changelist.
	
//	eventlist is a pointer to an array of kevent structures.
//	nevents determines the size of eventlist.
	
//	If timeout is a non-NULL pointer, it specifies a maximum interval to wait
//	for an event, which will be interpreted as a struct timespec.  If timeout
//		is a NULL pointer, kevent() waits indefinitely.  To effect a poll, the
//		timeout argument should be non-NULL, pointing to a zero-valued
//		timespec(3) structure.  The same array may be used for the changelist and
//			eventlist.
//	kevent() 提供向内核注册 / 反注册事件和返回就绪事件或错误事件。
//	struct kevent 就是kevent()操作的最基本的事件结构。
//	复制代码
//	struct kevent {
//		uintptr_t ident;       /* 事件 ID */
//		short     filter;       /* 事件过滤器 */
//		u_short   flags;        /* 行为标识 */
//		u_int     fflags;       /* 过滤器标识值 */
//		intptr_t  data;         /* 过滤器数据 */
//		void      *udata;       /* 应用透传数据 */
//	};
	rval = kevent(_dispatch_get_kq(),
				  &kev_copy, 1,
				  &kev_copy, 1, NULL);
	if (rval == -1) {
		// If we fail to register with kevents, for other reasons aside from
		// changelist elements.
		int err = errno;
		switch (err) {
			case EINTR:
				goto retry;
			case EBADF:
				_dispatch_bug_client("Do not close random Unix descriptors");
				break;
			default:
				(void)dispatch_assume_zero(err);
				break;
		}
		//kev_copy.flags |= EV_ERROR;
		//kev_copy.data = err;
		return err;
	}
	
	// The following select workaround only applies to adding kevents
	if ((kev->flags & (EV_DISABLE|EV_DELETE)) ||
		!(kev->flags & (EV_ADD|EV_ENABLE))) {
		return 0;
	}
	
	// Only executed on manager queue
	switch (kev_copy.data) {
		case 0:
			return 0;
		case EBADF:
			break;
		default:
			// If an error occurred while registering with kevent, and it was
			// because of a kevent changelist processing && the kevent involved
			// either doing a read or write, it would indicate we were trying
			// to register a /dev/* port; fall back to select
			switch (kev_copy.filter) {
				case EVFILT_READ:
					if (dispatch_assume(kev_copy.ident < FD_SETSIZE)) {
						if (!_dispatch_rfd_ptrs) {
							_dispatch_rfd_ptrs = calloc(FD_SETSIZE, sizeof(void*));
						}
						_dispatch_rfd_ptrs[kev_copy.ident] = kev_copy.udata;
						FD_SET((int)kev_copy.ident, &_dispatch_rfds);
						(void)dispatch_atomic_inc(&_dispatch_select_workaround);
						_dispatch_debug("select workaround used to read fd %d: 0x%lx",
										(int)kev_copy.ident, (long)kev_copy.data);
						return 0;
					}
					break;
				case EVFILT_WRITE:
					if (dispatch_assume(kev_copy.ident < FD_SETSIZE)) {
						if (!_dispatch_wfd_ptrs) {
							_dispatch_wfd_ptrs = calloc(FD_SETSIZE, sizeof(void*));
						}
						_dispatch_wfd_ptrs[kev_copy.ident] = kev_copy.udata;
						FD_SET((int)kev_copy.ident, &_dispatch_wfds);
						(void)dispatch_atomic_inc(&_dispatch_select_workaround);
						_dispatch_debug("select workaround used to write fd %d: 0x%lx",
										(int)kev_copy.ident, (long)kev_copy.data);
						return 0;
					}
					break;
				default:
					// kevent error, _dispatch_source_merge_kevent() will handle it
					_dispatch_source_drain_kevent(&kev_copy);
					break;
			}
			break;
	}
	return kev_copy.data;
}

bool
_dispatch_mgr_wakeup(dispatch_queue_t dq)
{
	static const struct kevent kev = {
		.ident = 1,
		.filter = EVFILT_USER,
		.fflags = NOTE_TRIGGER,
	};
	
	_dispatch_debug("waking up the _dispatch_mgr_q: %p", dq);
	
	_dispatch_update_kq(&kev);
	
	return false;
}

////////////////////////////////////////////////
static void
_dispatch_mgr_thread2(struct kevent *kev, size_t cnt)
{
	size_t i;
	for (i = 0; i < cnt; i++) {
		// EVFILT_USER isn't used by sources
		if (kev[i].filter == EVFILT_USER) {
			// If _dispatch_mgr_thread2() ever is changed to return to the
			// caller, then this should become _dispatch_queue_drain()
			_dispatch_queue_serial_drain_till_empty(&_dispatch_mgr_q);
		} else {
			_dispatch_source_drain_kevent(&kev[i]);
		}
	}
}

#if DISPATCH_USE_VM_PRESSURE && DISPATCH_USE_MALLOC_VM_PRESSURE_SOURCE
// VM Pressure source for malloc <rdar://problem/7805121>
static dispatch_source_t _dispatch_malloc_vm_pressure_source;

static void
_dispatch_malloc_vm_pressure_handler(void *context DISPATCH_UNUSED)
{
	malloc_zone_pressure_relief(0,0);
}

static void
_dispatch_malloc_vm_pressure_setup(void)
{
	_dispatch_malloc_vm_pressure_source = dispatch_source_create(
																 DISPATCH_SOURCE_TYPE_VM, 0, DISPATCH_VM_PRESSURE,
																 _dispatch_get_root_queue(0, true));
	dispatch_source_set_event_handler_f(_dispatch_malloc_vm_pressure_source,
										_dispatch_malloc_vm_pressure_handler);
	dispatch_resume(_dispatch_malloc_vm_pressure_source);
}
#else
#define _dispatch_malloc_vm_pressure_setup()
#endif

DISPATCH_NOINLINE DISPATCH_NORETURN
static void
_dispatch_mgr_invoke(void)
{
	static const struct timespec timeout_immediately = { 0, 0 };
	struct timespec timeout;
	const struct timespec *timeoutp;
	struct timeval sel_timeout, *sel_timeoutp;
	fd_set tmp_rfds, tmp_wfds;
	struct kevent kev[1];
	int k_cnt, err, i, r;
	
	_dispatch_thread_setspecific(dispatch_queue_key, &_dispatch_mgr_q);
#if DISPATCH_COCOA_COMPAT
	// Do not count the manager thread as a worker thread
	(void)dispatch_atomic_dec(&_dispatch_worker_threads);
#endif
	_dispatch_malloc_vm_pressure_setup();
	
	for (;;) {
		_dispatch_run_timers();
		
		timeoutp = _dispatch_get_next_timer_fire(&timeout);
		
		if (_dispatch_select_workaround) {
			FD_COPY(&_dispatch_rfds, &tmp_rfds);
			FD_COPY(&_dispatch_wfds, &tmp_wfds);
			if (timeoutp) {
				sel_timeout.tv_sec = timeoutp->tv_sec;
				sel_timeout.tv_usec = (typeof(sel_timeout.tv_usec))
				(timeoutp->tv_nsec / 1000u);
				sel_timeoutp = &sel_timeout;
			} else {
				sel_timeoutp = NULL;
			}
			
			r = select(FD_SETSIZE, &tmp_rfds, &tmp_wfds, NULL, sel_timeoutp);
			if (r == -1) {
				err = errno;
				if (err != EBADF) {
					if (err != EINTR) {
						(void)dispatch_assume_zero(err);
					}
					continue;
				}
				for (i = 0; i < FD_SETSIZE; i++) {
					if (i == _dispatch_kq) {
						continue;
					}
					if (!FD_ISSET(i, &_dispatch_rfds)
						&& !FD_ISSET(i, &_dispatch_wfds)) {
						continue;
					}
					r = dup(i);
					if (r != -1) {
						close(r);
					} else {
						if (FD_ISSET(i, &_dispatch_rfds)) {
							FD_CLR(i, &_dispatch_rfds);
							_dispatch_rfd_ptrs[i] = 0;
							(void)dispatch_atomic_dec(
												&_dispatch_select_workaround);
						}
						if (FD_ISSET(i, &_dispatch_wfds)) {
							FD_CLR(i, &_dispatch_wfds);
							_dispatch_wfd_ptrs[i] = 0;
							(void)dispatch_atomic_dec(
											&_dispatch_select_workaround);
						}
					}
				}
				continue;
			}
			
			if (r > 0) {
				for (i = 0; i < FD_SETSIZE; i++) {
					if (i == _dispatch_kq) {
						continue;
					}
					if (FD_ISSET(i, &tmp_rfds)) {
						FD_CLR(i, &_dispatch_rfds); // emulate EV_DISABLE
						EV_SET(&kev[0], i, EVFILT_READ,
							   EV_ADD|EV_ENABLE|EV_DISPATCH, 0, 1,
							   _dispatch_rfd_ptrs[i]);
						_dispatch_rfd_ptrs[i] = 0;
						(void)dispatch_atomic_dec(&_dispatch_select_workaround);
						_dispatch_mgr_thread2(kev, 1);
					}
					if (FD_ISSET(i, &tmp_wfds)) {
						FD_CLR(i, &_dispatch_wfds); // emulate EV_DISABLE
						EV_SET(&kev[0], i, EVFILT_WRITE,
							   EV_ADD|EV_ENABLE|EV_DISPATCH, 0, 1,
							   _dispatch_wfd_ptrs[i]);
						_dispatch_wfd_ptrs[i] = 0;
						(void)dispatch_atomic_dec(&_dispatch_select_workaround);
						_dispatch_mgr_thread2(kev, 1);
					}
				}
			}
			
			timeoutp = &timeout_immediately;
		}
		
		k_cnt = kevent(_dispatch_kq,
					   NULL, 0,
					   kev, sizeof(kev) / sizeof(kev[0]),
					   timeoutp);
		err = errno;
		switch (k_cnt) {
			case -1:
				if (err == EBADF) {
					DISPATCH_CLIENT_CRASH("Do not close random Unix descriptors");
				}
				if (err != EINTR) {
					(void)dispatch_assume_zero(err);
				}
				continue;
			default:
				_dispatch_mgr_thread2(kev, (size_t)k_cnt);
				// fall through
			case 0:
				_dispatch_force_cache_cleanup();
				continue;
		}
	}
}

DISPATCH_NORETURN
dispatch_queue_t
_dispatch_mgr_thread(dispatch_queue_t dq DISPATCH_UNUSED)
{
	// never returns, so burn bridges behind us & clear stack 2k ahead
	_dispatch_clear_stack(2048);
	_dispatch_mgr_invoke();
}


