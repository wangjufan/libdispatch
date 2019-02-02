//
//  queue_debug.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "queue_debug.h"

#pragma mark -
#pragma mark dispatch_queue_debug

size_t
_dispatch_queue_debug_attr(dispatch_queue_t dq, char* buf, size_t bufsiz)
{
	dispatch_queue_t target = dq->do_targetq;
	return snprintf(buf, bufsiz, "target = %s[%p], width = 0x%x, "
					"running = 0x%x, barrier = %d ", target ? target->dq_label : "",
					target, dq->dq_width / 2, dq->dq_running / 2, dq->dq_running & 1);
}

size_t
dispatch_queue_debug(dispatch_queue_t dq, char* buf, size_t bufsiz)
{
	size_t offset = 0;
	offset += snprintf(&buf[offset], bufsiz - offset, "%s[%p] = { ",
					   dq->dq_label, dq);
	offset += _dispatch_object_debug_attr(dq, &buf[offset], bufsiz - offset);
	offset += _dispatch_queue_debug_attr(dq, &buf[offset], bufsiz - offset);
	offset += snprintf(&buf[offset], bufsiz - offset, "}");
	return offset;
}

#if DISPATCH_DEBUG
void
dispatch_debug_queue(dispatch_queue_t dq, const char* str) {
	if (fastpath(dq)) {
		dispatch_debug(dq, "%s", str);
	} else {
		_dispatch_log("queue[NULL]: %s", str);
	}
}
#endif

#if DISPATCH_PERF_MON
static OSSpinLock _dispatch_stats_lock;
static size_t _dispatch_bad_ratio;
static struct {
	uint64_t time_total;
	uint64_t count_total;
	uint64_t thread_total;
} _dispatch_stats[65]; // ffs*/fls*() returns zero when no bits are set

static void
_dispatch_queue_merge_stats(uint64_t start)
{
	uint64_t avg, delta = _dispatch_absolute_time() - start;
	unsigned long count, bucket;
	
	count = (size_t)_dispatch_thread_getspecific(dispatch_bcounter_key);
	_dispatch_thread_setspecific(dispatch_bcounter_key, NULL);
	
	if (count) {
		avg = delta / count;
		bucket = flsll(avg);
	} else {
		bucket = 0;
	}
	
	// 64-bit counters on 32-bit require a lock or a queue
	OSSpinLockLock(&_dispatch_stats_lock);
	
	_dispatch_stats[bucket].time_total += delta;
	_dispatch_stats[bucket].count_total += count;
	_dispatch_stats[bucket].thread_total++;
	
	OSSpinLockUnlock(&_dispatch_stats_lock);
}
#endif
