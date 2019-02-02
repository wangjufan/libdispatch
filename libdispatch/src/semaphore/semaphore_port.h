//
//  semaphore_port.h
//  libdispatch
//
//  Created by 王举范 on 2018/11/27.
//

#ifndef semaphore_port_h
#define semaphore_port_h

#include <stdio.h>



#if USE_FUTEX_SEM
#define DISPATCH_FUTEX_NUM_SPINS 100
#define DISPATCH_FUTEX_VALUE_MAX INT_MAX
#define DISPATCH_FUTEX_NWAITERS_SHIFT 32
#define DISPATCH_FUTEX_VALUE_MASK ((1ull << DISPATCH_FUTEX_NWAITERS_SHIFT) - 1)

static int _dispatch_futex_trywait(dispatch_futex_t *dfx);
static int _dispatch_futex_wait_slow(dispatch_futex_t *dfx,
									 const struct timespec* timeout);
static int _dispatch_futex_syscall(dispatch_futex_t *dfx, int op, int val,
								   const struct timespec *timeout);

int
_dispatch_futex_init(dispatch_futex_t *dfx)
{
	dfx->dfx_data = 0;
	return 0;
}

int
_dispatch_futex_dispose(dispatch_futex_t *dfx)
{
	(void)dfx;
	return 0;
}

// Increments semaphore value and reads wait count in a single atomic
// operation, with release MO. If wait count is nonzero, issues a FUTEX_WAKE.
int
_dispatch_futex_signal(dispatch_futex_t *dfx)
{
	uint64_t orig;
	do {
		orig = dfx->dfx_data;
		if (slowpath((orig & DISPATCH_FUTEX_VALUE_MASK) ==
					 DISPATCH_FUTEX_VALUE_MAX)) {
			DISPATCH_CRASH("semaphore overflow");
		}
	} while (!dispatch_atomic_cmpxchg2o(dfx, dfx_data, orig, orig + 1));
	if (slowpath(orig >> DISPATCH_FUTEX_NWAITERS_SHIFT)) {
		int ret = _dispatch_futex_syscall(dfx, FUTEX_WAKE, 1, NULL);
		DISPATCH_SEMAPHORE_VERIFY_RET(ret);
	}
	return 0;
}

// `timeout` is relative, and can be NULL for an infinite timeout--see futex(2).
int
_dispatch_futex_wait(dispatch_futex_t *dfx, const struct timespec *timeout)
{
	if (fastpath(!_dispatch_futex_trywait(dfx))) {
		return 0;
	}
	return _dispatch_futex_wait_slow(dfx, timeout);
}

// Atomic-decrement-if-positive on the semaphore value, with acquire MO if
// successful.
int
_dispatch_futex_trywait(dispatch_futex_t *dfx)
{
	uint64_t orig;
	while ((orig = dfx->dfx_data) & DISPATCH_FUTEX_VALUE_MASK) {
		if (dispatch_atomic_cmpxchg2o(dfx, dfx_data, orig, orig - 1)) {
			return 0;
		}
	}
	return -1;
}

DISPATCH_NOINLINE
static int
_dispatch_futex_wait_slow(dispatch_futex_t *dfx, const struct timespec *timeout)
{
	int spins = DISPATCH_FUTEX_NUM_SPINS;
	// Spin for a short time (if there are no waiters).
	while (spins-- && !dfx->dfx_data) {
		_dispatch_hardware_pause();
	}
	while (_dispatch_futex_trywait(dfx)) {
		dispatch_atomic_add2o(dfx, dfx_data,
							  1ull << DISPATCH_FUTEX_NWAITERS_SHIFT);
		int ret = _dispatch_futex_syscall(dfx, FUTEX_WAIT, 0, timeout);
		dispatch_atomic_sub2o(dfx, dfx_data,
							  1ull << DISPATCH_FUTEX_NWAITERS_SHIFT);
		switch (ret == -1 ? errno : 0) {
			case EWOULDBLOCK:
				if (!timeout) {
					break;
				} else if (!_dispatch_futex_trywait(dfx)) {
					return 0;
				} else {
					// Caller must recompute timeout.
					errno = EINTR;
					return -1;
				}
			case EINTR:
			case ETIMEDOUT:
				return -1;
			default:
				DISPATCH_SEMAPHORE_VERIFY_RET(ret);
				break;
		}
	}
	return 0;
}

static int
_dispatch_futex_syscall(
						dispatch_futex_t *dfx, int op, int val, const struct timespec *timeout)
{
#ifdef DISPATCH_LITTLE_ENDIAN
	int *addr = (int *)&dfx->dfx_data;
#elif DISPATCH_BIG_ENDIAN
	int *addr = (int *)&dfx->dfx_data + 1;
#endif
	return (int)syscall(SYS_futex, addr, op | FUTEX_PRIVATE_FLAG, val, timeout,
						NULL, 0);
}
#endif /* USE_FUTEX_SEM */


#endif /* semaphore_port_h */
