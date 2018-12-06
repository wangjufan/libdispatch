/*
 * Copyright (c) 2008-2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

// Contains exported global data and initialization & other routines that must
// only exist once in the shared library even when resolvers are used.

#include "internal.h"

#if HAVE_MACH
#include "protocolServer.h"
#endif

#pragma mark -
#pragma mark dispatch_init

#if USE_LIBDISPATCH_INIT_CONSTRUCTOR
DISPATCH_NOTHROW __attribute__((constructor))
void
_libdispatch_init(void);

DISPATCH_EXPORT DISPATCH_NOTHROW
void
_libdispatch_init(void)
{
	libdispatch_init();
}
#endif


void
dummy_function(void)
{
}

long
dummy_function_r0(void)
{
	return 0;
}

#pragma mark -
#pragma mark dispatch_globals

#if DISPATCH_COCOA_COMPAT
void (*dispatch_begin_thread_4GC)(void);
void (*dispatch_end_thread_4GC)(void);
void (*dispatch_no_worker_threads_4GC)(void);
void *(*_dispatch_begin_NSAutoReleasePool)(void);
void (*_dispatch_end_NSAutoReleasePool)(void *);
#endif

#if !DISPATCH_USE_DIRECT_TSD
pthread_key_t dispatch_queue_key;
pthread_key_t dispatch_sema4_key;
pthread_key_t dispatch_cache_key;
pthread_key_t dispatch_io_key;
pthread_key_t dispatch_apply_key;
#if DISPATCH_PERF_MON
pthread_key_t dispatch_bcounter_key;
#endif
#endif // !DISPATCH_USE_DIRECT_TSD

struct _dispatch_hw_config_s _dispatch_hw_config;
bool _dispatch_safe_fork = true;

DISPATCH_NOINLINE
bool
_dispatch_is_multithreaded(void)
{
	return !_dispatch_safe_fork;
}

const struct dispatch_queue_offsets_s dispatch_queue_offsets = {
	.dqo_version = 3,
	.dqo_label = offsetof(struct dispatch_queue_s, dq_label),
	.dqo_label_size = sizeof(((dispatch_queue_t)NULL)->dq_label),
	.dqo_flags = 0,
	.dqo_flags_size = 0,
	.dqo_width = offsetof(struct dispatch_queue_s, dq_width),
	.dqo_width_size = sizeof(((dispatch_queue_t)NULL)->dq_width),
	.dqo_serialnum = offsetof(struct dispatch_queue_s, dq_serialnum),
	.dqo_serialnum_size = sizeof(((dispatch_queue_t)NULL)->dq_serialnum),
	.dqo_running = offsetof(struct dispatch_queue_s, dq_running),
	.dqo_running_size = sizeof(((dispatch_queue_t)NULL)->dq_running),
};



struct dispatch_queue_attr_s _dispatch_queue_attr_concurrent = {
	.do_vtable = DISPATCH_VTABLE(queue_attr),
	.do_ref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	.do_xref_cnt = DISPATCH_OBJECT_GLOBAL_REFCNT,
	.do_next = DISPATCH_OBJECT_LISTLESS,
};


#pragma mark -
#pragma mark dispatch_bug

static char _dispatch_build[16];

static void
_dispatch_build_init(void *context DISPATCH_UNUSED)
{
#ifdef __APPLE__
	int mib[] = { CTL_KERN, KERN_OSVERSION };
	size_t bufsz = sizeof(_dispatch_build);

	sysctl(mib, 2, _dispatch_build, &bufsz, NULL, 0);
#else
	/*
	 * XXXRW: What to do here for !Mac OS X?
	 */
	memset(_dispatch_build, 0, sizeof(_dispatch_build));
#endif
}

#define _dispatch_bug_log(msg, ...) do { \
	static void *last_seen; \
	void *ra = __builtin_return_address(0); \
	if (last_seen != ra) { \
		last_seen = ra; \
		_dispatch_log((msg), ##__VA_ARGS__); \
	} \
} while(0)

void
_dispatch_bug(size_t line, long val)
{
	static dispatch_once_t pred;

	dispatch_once_f(&pred, NULL, _dispatch_build_init);
	_dispatch_bug_log("BUG in libdispatch: %s - %lu - 0x%lx",
			_dispatch_build, (unsigned long)line, val);
}

void
_dispatch_bug_client(const char* msg)
{
	_dispatch_bug_log("BUG in libdispatch client: %s", msg);
}

#if HAVE_MACH
void
_dispatch_bug_mach_client(const char* msg, mach_msg_return_t kr)
{
	_dispatch_bug_log("BUG in libdispatch client: %s %s - 0x%x", msg,
			mach_error_string(kr), kr);
}
#endif

void
_dispatch_abort(size_t line, long val)
{
	_dispatch_bug(line, val);
	abort();
}

#pragma mark -
#pragma mark dispatch_log

static FILE *dispatch_logfile;
static bool dispatch_log_disabled;
static dispatch_once_t _dispatch_logv_pred;

static void
_dispatch_logv_init(void *context DISPATCH_UNUSED)
{
#if DISPATCH_DEBUG
	bool log_to_file = true;
#else
	bool log_to_file = false;
#endif
	char *e = getenv("LIBDISPATCH_LOG");
	if (e) {
		if (strcmp(e, "YES") == 0) {
			// default
		} else if (strcmp(e, "NO") == 0) {
			dispatch_log_disabled = true;
		} else if (strcmp(e, "syslog") == 0) {
			log_to_file = false;
		} else if (strcmp(e, "file") == 0) {
			log_to_file = true;
		} else if (strcmp(e, "stderr") == 0) {
			log_to_file = true;
			dispatch_logfile = stderr;
		}
	}
	if (!dispatch_log_disabled) {
		if (log_to_file && !dispatch_logfile) {
			char path[PATH_MAX];
			snprintf(path, sizeof(path), "/var/tmp/libdispatch.%d.log",
					getpid());
			dispatch_logfile = fopen(path, "a");
		}
		if (dispatch_logfile) {
			struct timeval tv;
			gettimeofday(&tv, NULL);
			fprintf(dispatch_logfile, "=== log file opened for %s[%u] at "
					"%ld.%06ld ===\n", getprogname() ?: "", getpid(),
					tv.tv_sec, (long)tv.tv_usec);
			fflush(dispatch_logfile);
		}
	}
}

DISPATCH_NOINLINE
static void
_dispatch_logv_file(const char *msg, va_list ap)
{
	char *buf;
	size_t len;

	len = vasprintf(&buf, msg, ap);
	buf[len++] = '\n';
	fwrite(buf, 1, len, dispatch_logfile);
	fflush(dispatch_logfile);
	free(buf);
}

static inline void
_dispatch_logv(const char *msg, va_list ap)
{
	dispatch_once_f(&_dispatch_logv_pred, NULL, _dispatch_logv_init);
	if (slowpath(dispatch_log_disabled)) {
		return;
	}
	if (slowpath(dispatch_logfile)) {
		return _dispatch_logv_file(msg, ap);
	}
	vsyslog(LOG_NOTICE, msg, ap);
}

DISPATCH_NOINLINE
void
_dispatch_log(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	_dispatch_logv(msg, ap);
	va_end(ap);
}

#pragma mark -
#pragma mark dispatch_debug

DISPATCH_NOINLINE
void
dispatch_debugv(dispatch_object_t dou, const char *msg, va_list ap)
{
	char buf[4096];
	size_t offs;

	if (dou._do && dou._do->do_vtable->do_debug) {
		offs = dx_debug(dou._do, buf, sizeof(buf));
	} else {
		offs = snprintf(buf, sizeof(buf), "NULL vtable slot");
	}

	snprintf(buf + offs, sizeof(buf) - offs, ": %s", msg);
	_dispatch_logv(buf, ap);
}

DISPATCH_NOINLINE
void
dispatch_debug(dispatch_object_t dou, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	dispatch_debugv(dou._do, msg, ap);
	va_end(ap);
}

#pragma mark -
#pragma mark dispatch_block_t

#ifdef __BLOCKS__

#undef _dispatch_Block_copy
dispatch_block_t
_dispatch_Block_copy(dispatch_block_t db)
{
	dispatch_block_t rval;

	if (fastpath(db)) { 
		while (!fastpath(rval = Block_copy(db))) {
			sleep(1);
		}
		return rval;
	}
	DISPATCH_CLIENT_CRASH("NULL was passed where a block should have been");
}

void
_dispatch_call_block_and_release(void *block)
{
	void (^b)(void) = block;
	b();
	Block_release(b);
}

#endif // __BLOCKS__

#pragma mark -
#pragma mark dispatch_client_callout

// Abort on uncaught exceptions thrown from client callouts rdar://8577499
#if DISPATCH_USE_CLIENT_CALLOUT && (__arm__ || !USE_OBJC)
// On platforms with SjLj exceptions, avoid the SjLj overhead on every callout
// by clearing the unwinder's TSD pointer to the handler stack around callouts

#define _dispatch_get_tsd_base()
#define _dispatch_get_unwind_tsd() (NULL)
#define _dispatch_set_unwind_tsd(u) do {(void)(u);} while (0)
#define _dispatch_free_unwind_tsd()

#undef _dispatch_client_callout
DISPATCH_NOINLINE
void
_dispatch_client_callout(void *ctxt, dispatch_function_t f)
{
	_dispatch_get_tsd_base();
	void *u = _dispatch_get_unwind_tsd();
	_dispatch_set_unwind_tsd(NULL);
	f(ctxt);
	_dispatch_free_unwind_tsd();
	_dispatch_set_unwind_tsd(u);
}

#undef _dispatch_client_callout2
DISPATCH_NOINLINE
void
_dispatch_client_callout2(void *ctxt, size_t i, void (*f)(void *, size_t))
{
	_dispatch_get_tsd_base();
	void *u = _dispatch_get_unwind_tsd();
	_dispatch_set_unwind_tsd(NULL);
	f(ctxt, i);
	_dispatch_free_unwind_tsd();
	_dispatch_set_unwind_tsd(u);
}

#endif // DISPATCH_USE_CLIENT_CALLOUT

#pragma mark -
#pragma mark _os_object_t no_objc

#if !USE_OBJC

static const _os_object_class_s _os_object_class;

void
_os_object_init(void)
{
	return;
}

_os_object_t
_os_object_alloc(const void *cls, size_t size)
{
	_os_object_t obj;
	dispatch_assert(size >= sizeof(struct _os_object_s));
	if (!cls) cls = &_os_object_class;
	while (!fastpath(obj = calloc(1u, size))) {
		sleep(1); // Temporary resource shortage
	}
	obj->os_obj_isa = cls;
	return obj;
}

void
_os_object_dealloc(_os_object_t obj)
{
	*((void *volatile*)&obj->os_obj_isa) = (void *)0x200;
	return free(obj);
}

void
_os_object_xref_dispose(_os_object_t obj)
{
	if (fastpath(obj->os_obj_isa->_os_obj_xref_dispose)) {
		return obj->os_obj_isa->_os_obj_xref_dispose(obj);
	}
	return _os_object_release_internal(obj);
}

void
_os_object_dispose(_os_object_t obj)
{
	if (fastpath(obj->os_obj_isa->_os_obj_dispose)) {
		return obj->os_obj_isa->_os_obj_dispose(obj);
	}
	return _os_object_dealloc(obj);
}

#pragma mark -
#pragma mark dispatch_autorelease_pool no_objc

#if DISPATCH_COCOA_COMPAT

void *_dispatch_autorelease_pool_push(void) {
	void *pool = NULL;
	if (_dispatch_begin_NSAutoReleasePool) {
		pool = _dispatch_begin_NSAutoReleasePool();
	}
	return pool;
}

void _dispatch_autorelease_pool_pop(void *pool) {
	if (_dispatch_end_NSAutoReleasePool) {
		_dispatch_end_NSAutoReleasePool(pool);
	}
}

#endif // DISPATCH_COCOA_COMPAT
#endif // !USE_OBJC





#pragma mark -
#pragma mark dispatch_mig

void *
dispatch_mach_msg_get_context(mach_msg_header_t *msg)
{
	mach_msg_context_trailer_t *tp;
	void *context = NULL;

	tp = (mach_msg_context_trailer_t *)((uint8_t *)msg +
			round_msg(msg->msgh_size));
	if (tp->msgh_trailer_size >=
			(mach_msg_size_t)sizeof(mach_msg_context_trailer_t)) {
		context = (void *)(uintptr_t)tp->msgh_context;
	}
	return context;
}

kern_return_t
_dispatch_wakeup_main_thread(mach_port_t mp DISPATCH_UNUSED)
{
	// dummy function just to pop out the main thread out of mach_msg()
	return 0;
}

kern_return_t
_dispatch_consume_send_once_right(mach_port_t mp DISPATCH_UNUSED)
{
	// dummy function to consume a send-once right
	return 0;
}

kern_return_t
_dispatch_mach_notify_port_destroyed(mach_port_t notify DISPATCH_UNUSED,
		mach_port_t name)
{
	kern_return_t kr;
	// this function should never be called
	(void)dispatch_assume_zero(name);
	kr = mach_port_mod_refs(mach_task_self(), name, MACH_PORT_RIGHT_RECEIVE,-1);
	DISPATCH_VERIFY_MIG(kr);
	(void)dispatch_assume_zero(kr);
	return KERN_SUCCESS;
}

kern_return_t
_dispatch_mach_notify_no_senders(mach_port_t notify,
		mach_port_mscount_t mscnt DISPATCH_UNUSED)
{
	// this function should never be called
	(void)dispatch_assume_zero(notify);
	return KERN_SUCCESS;
}

kern_return_t
_dispatch_mach_notify_send_once(mach_port_t notify DISPATCH_UNUSED)
{
	// we only register for dead-name notifications
	// some code deallocated our send-once right without consuming it
#if DISPATCH_DEBUG
	_dispatch_log("Corruption: An app/library deleted a libdispatch "
			"dead-name notification");
#endif
	return KERN_SUCCESS;
}

#endif // HAVE_MACH
