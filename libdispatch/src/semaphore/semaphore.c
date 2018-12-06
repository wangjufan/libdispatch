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

#include "internal.h"

// semaphores are too fundamental to use the dispatch_assume*() macros
#if USE_MACH_SEM
#define DISPATCH_SEMAPHORE_VERIFY_KR(x) do { \
		if (slowpath(x)) { \
			DISPATCH_CRASH("flawed group/semaphore logic"); \
		} \
	} while (0)
#else
#define DISPATCH_SEMAPHORE_VERIFY_RET(x) do { \
		if (slowpath((x) == -1)) { \
			DISPATCH_CRASH("flawed group/semaphore logic"); \
		} \
	} while (0)
#endif

DISPATCH_WEAK // rdar://problem/8503746
long _dispatch_semaphore_signal_slow(dispatch_semaphore_t dsema);

static long _dispatch_group_wake(dispatch_semaphore_t dsema);

#pragma mark -
#pragma mark dispatch_semaphore_t

static void
_dispatch_semaphore_init(long value, dispatch_object_t dou)
{
	dispatch_semaphore_t dsema = dou._dsema;

	dsema->do_next = DISPATCH_OBJECT_LISTLESS;
	dsema->do_targetq = dispatch_get_global_queue(
			DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dsema->dsema_value = value;
	dsema->dsema_orig = value;
#if USE_POSIX_SEM
	int ret = sem_init(&dsema->dsema_sem, 0, 0);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#elif USE_FUTEX_SEM
	int ret = _dispatch_futex_init(&dsema->dsema_futex);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
}

dispatch_semaphore_t
dispatch_semaphore_create(long value)
{
	dispatch_semaphore_t dsema;

	// If the internal value is negative, then the absolute of the value is
	// equal to the number of waiting threads. Therefore it is bogus to
	// initialize the semaphore with a negative value.
	if (value < 0) {
		return NULL;
	}
//#define DISPATCH_VTABLE(name) &_dispatch_##name##_vtable
	dsema = _dispatch_alloc(DISPATCH_VTABLE(semaphore),
			sizeof(struct dispatch_semaphore_s));
	_dispatch_semaphore_init(value, dsema);
	return dsema;
}


void
_dispatch_semaphore_dispose(dispatch_object_t dou)
{
	dispatch_semaphore_t dsema = dou._dsema;
//#define dx_dispose(x) (x)->do_vtable->do_dispose(x)
//	void
//	_dispatch_dispose(dispatch_object_t dou)
//	{
//		if (slowpath(dou._do->do_next != DISPATCH_OBJECT_LISTLESS)) {
//			DISPATCH_CRASH("Release while enqueued");
//		}
//		dx_dispose(dou._do);
//		return _dispatch_dealloc(dou);
//	}
	if (dsema->dsema_value < dsema->dsema_orig) {
		DISPATCH_CLIENT_CRASH(
				"Semaphore/group object deallocated while in use");
	}

//	mach_port_t   mach_task_self (void)
//	The mach_task_self function
//	returns send rights
//	to the task's kernel port.
	
#if USE_MACH_SEM
	kern_return_t kr;
	if (dsema->dsema_port) {
		kr = semaphore_destroy(mach_task_self(),
							   dsema->dsema_port);
		DISPATCH_SEMAPHORE_VERIFY_KR(kr);
	}
	if (dsema->dsema_waiter_port) {
		kr = semaphore_destroy(mach_task_self(),
							   dsema->dsema_waiter_port);
		DISPATCH_SEMAPHORE_VERIFY_KR(kr);
	}
#elif USE_POSIX_SEM
	int ret = sem_destroy(&dsema->dsema_sem);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#elif USE_FUTEX_SEM
	int ret = _dispatch_futex_dispose(&dsema->dsema_futex);
	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif
}


size_t
_dispatch_semaphore_debug(dispatch_object_t dou, char *buf, size_t bufsiz)
{
	dispatch_semaphore_t dsema = dou._dsema;

	size_t offset = 0;
	offset += snprintf(&buf[offset], bufsiz - offset, "%s[%p] = { ",
			dx_kind(dsema), dsema);
	offset += _dispatch_object_debug_attr(dsema, &buf[offset], bufsiz - offset);
#if USE_MACH_SEM
	offset += snprintf(&buf[offset], bufsiz - offset, "port = 0x%u, ",
			dsema->dsema_port);
#endif
	offset += snprintf(&buf[offset], bufsiz - offset,
			"value = %ld, orig = %ld }", dsema->dsema_value, dsema->dsema_orig);
	return offset;
}




DISPATCH_NOINLINE
long
_dispatch_semaphore_signal_slow(dispatch_semaphore_t dsema)
{
	// Before dsema_sent_ksignals is incremented we can rely on the reference
	// held by the waiter. However, once this value is incremented the waiter
	// may return between the atomic increment and the semaphore_signal(),
	// therefore an explicit reference must be held in order to safely access
	// dsema after the atomic increment.
	_dispatch_retain(dsema);

#if USE_MACH_SEM
	(void)dispatch_atomic_inc2o(dsema, dsema_sent_ksignals);
#endif

#if USE_MACH_SEM
	_dispatch_semaphore_create_port(&dsema->dsema_port);
	kern_return_t kr = semaphore_signal(dsema->dsema_port);
	DISPATCH_SEMAPHORE_VERIFY_KR(kr);
//#elif USE_POSIX_SEM
//	// POSIX semaphore is created in _dispatch_semaphore_init, not lazily.
//	int ret = sem_post(&dsema->dsema_sem);
//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
//#elif USE_FUTEX_SEM
//	// dispatch_futex_t is created in _dispatch_semaphore_init, not lazily.
//	int ret = _dispatch_futex_signal(&dsema->dsema_futex);
//	DISPATCH_SEMAPHORE_VERIFY_RET(ret);
#endif

	_dispatch_release(dsema);
	return 1;
}
long
dispatch_semaphore_signal(dispatch_semaphore_t dsema)
{
	dispatch_atomic_release_barrier();
	long value = dispatch_atomic_inc2o(dsema, dsema_value);
	if (fastpath(value > 0)) {
		return 0;
	}
	if (slowpath(value == LONG_MIN)) {
//#define LONG_MIN  (-__LONG_MAX__ -1L)
//#define LONG_MAX  __LONG_MAX__
		DISPATCH_CLIENT_CRASH("Unbalanced call to dispatch_semaphore_signal()");
	}
	return _dispatch_semaphore_signal_slow(dsema);
}



DISPATCH_NOINLINE
static long
_dispatch_semaphore_wait_slow(dispatch_semaphore_t dsema,
		dispatch_time_t timeout)
{
	long orig;

#if USE_MACH_SEM
	mach_timespec_t _timeout;
	kern_return_t kr;
#elif USE_POSIX_SEM || USE_FUTEX_SEM
	struct timespec _timeout;
	int ret;
#endif
	
#if USE_MACH_SEM
again:
	// Mach semaphores appear to sometimes spuriously wake up. Therefore,
	// we keep a parallel count of the number of times a Mach semaphore is
	// signaled (6880961).
	while ((orig = dsema->dsema_sent_ksignals)) {
		if (dispatch_atomic_cmpxchg2o(dsema, dsema_sent_ksignals,
									  orig,  orig - 1)) {
			return 0;
		}
	}
#endif

#if USE_MACH_SEM
	_dispatch_semaphore_create_port(&dsema->dsema_port);
#elif USE_POSIX_SEM || USE_FUTEX_SEM
	// Created in _dispatch_semaphore_init.
#endif

	// From xnu/osfmk/kern/sync_sema.c:
	// wait_semaphore->count = -1; /* we don't keep an actual count */
	//
	// The code above does not match the documentation, and that fact is
	// not surprising. The documented semantics are clumsy to use in any
	// practical way. The above hack effectively tricks the rest of the
	// Mach semaphore logic to behave like the libdispatch algorithm.

	switch (timeout) {
	default:
#if USE_MACH_SEM
		do {
			uint64_t nsec = _dispatch_timeout(timeout);
			_timeout.tv_sec = (typeof(_timeout.tv_sec))(nsec / NSEC_PER_SEC);
			_timeout.tv_nsec = (typeof(_timeout.tv_nsec))(nsec % NSEC_PER_SEC);
			kr = slowpath(semaphore_timedwait(dsema->dsema_port, _timeout));
		} while (kr == KERN_ABORTED);

		if (kr != KERN_OPERATION_TIMED_OUT) {
			DISPATCH_SEMAPHORE_VERIFY_KR(kr);
			break;
		}
#endif
		// Fall through and try to undo what the fast path did to
		// dsema->dsema_value
	case DISPATCH_TIME_NOW:
		while ((orig = dsema->dsema_value) < 0) {
			if (dispatch_atomic_cmpxchg2o(dsema, dsema_value, orig, orig + 1)) {
#if USE_MACH_SEM
				return KERN_OPERATION_TIMED_OUT;
#endif
			}
		}
		// Another thread called semaphore_signal().
		// Fall through and drain the wakeup.
	case DISPATCH_TIME_FOREVER:
#if USE_MACH_SEM
		do {
			kr = semaphore_wait(dsema->dsema_port);
		} while (kr == KERN_ABORTED);
		DISPATCH_SEMAPHORE_VERIFY_KR(kr);
#endif
		break;
	}
	
#if USE_MACH_SEM
	goto again;
#else
	return 0;
#endif
}

long
dispatch_semaphore_wait(dispatch_semaphore_t dsema, dispatch_time_t timeout)
{
	long value = dispatch_atomic_dec2o(dsema, dsema_value);
	dispatch_atomic_acquire_barrier();
	if (fastpath(value >= 0)) {
		return 0;
	}
	return _dispatch_semaphore_wait_slow(dsema, timeout);
}


