//
//  queue_create.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#include "queue_create.h"

#pragma mark -
#pragma mark dispatch_queue_t

// Note to later developers: ensure that any initialization changes are
// made for statically allocated queues (i.e. _dispatch_main_q).
static inline void
_dispatch_queue_init(dispatch_queue_t dq)
{
	dq->do_next = (struct dispatch_queue_s *)DISPATCH_OBJECT_LISTLESS;
	// Default target queue is overcommit!
	dq->do_targetq = _dispatch_get_root_queue(0, true);
	dq->dq_running = 0;
	dq->dq_width = 1;
	dq->dq_serialnum = dispatch_atomic_inc(&_dispatch_queue_serial_numbers) - 1;
}

// skip zero
// 1 - main_q
// 2 - mgr_q
// 3 - _unused_
// 4,5,6,7,8,9,10,11 - global queues
// we use 'xadd' on Intel, so the initial value == next assigned
unsigned long _dispatch_queue_serial_numbers = 12;

dispatch_queue_t
dispatch_queue_create(const char *label, dispatch_queue_attr_t attr)
{
	dispatch_queue_t dq;
	size_t label_len;
	
	if (!label) {
		label = "";
	}
	
	label_len = strlen(label);
	if (label_len < (DISPATCH_QUEUE_MIN_LABEL_SIZE - 1)) {
		label_len = (DISPATCH_QUEUE_MIN_LABEL_SIZE - 1);
	}
	
	// XXX switch to malloc()
	dq = _dispatch_alloc(DISPATCH_VTABLE(queue),
						 sizeof(struct dispatch_queue_s) - DISPATCH_QUEUE_MIN_LABEL_SIZE -
						 DISPATCH_QUEUE_CACHELINE_PAD + label_len + 1);
	
	_dispatch_queue_init(dq);
	strcpy(dq->dq_label, label);
	
	if (fastpath(!attr)) {
		return dq;
	}
	if (fastpath(attr == DISPATCH_QUEUE_CONCURRENT)) {
		dq->dq_width = UINT32_MAX;
		dq->do_targetq = _dispatch_get_root_queue(0, false);
	} else {
		dispatch_debug_assert(!attr, "Invalid attribute");
	}
	return dq;
}

// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
void
_dispatch_queue_dispose(dispatch_queue_t dq)
{
	if (slowpath(dq == _dispatch_queue_get_current())) {
		DISPATCH_CRASH("Release of a queue by itself");
	}
	if (slowpath(dq->dq_items_tail)) {
		DISPATCH_CRASH("Release of a queue while items are enqueued");
	}
	
	// trash the tail queue so that use after free will crash
	dq->dq_items_tail = (void *)0x200;
	
	dispatch_queue_t dqsq = dispatch_atomic_xchg2o(dq, dq_specific_q,
												   (void *)0x200);
	if (dqsq) {
		_dispatch_release(dqsq);
	}
}

const char *
dispatch_queue_get_label(dispatch_queue_t dq)
{
	return dq->dq_label;
}

static void
_dispatch_queue_set_width2(void *ctxt)
{
	int w = (int)(intptr_t)ctxt; // intentional truncation
	uint32_t tmp;
	dispatch_queue_t dq = _dispatch_queue_get_current();
	
	if (w == 1 || w == 0) {
		dq->dq_width = 1;
		return;
	}
	if (w > 0) {
		tmp = w;
	} else switch (w) {
		case DISPATCH_QUEUE_WIDTH_MAX_PHYSICAL_CPUS:
			tmp = _dispatch_hw_config.cc_max_physical;
			break;
		case DISPATCH_QUEUE_WIDTH_ACTIVE_CPUS:
			tmp = _dispatch_hw_config.cc_max_active;
			break;
		default:
			// fall through
		case DISPATCH_QUEUE_WIDTH_MAX_LOGICAL_CPUS:
			tmp = _dispatch_hw_config.cc_max_logical;
			break;
	}
	// multiply by two since the running count is inc/dec by two
	// (the low bit == barrier)
	dq->dq_width = tmp * 2;
}

void
dispatch_queue_set_width(dispatch_queue_t dq, long width)
{
	if (slowpath(dq->do_ref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT)) {
		return;
	}
	dispatch_barrier_async_f(dq, (void*)(intptr_t)width,
							 _dispatch_queue_set_width2);
}

// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
static void
_dispatch_set_target_queue2(void *ctxt)
{
	dispatch_queue_t prev_dq, dq = _dispatch_queue_get_current();
	
	prev_dq = dq->do_targetq;
	dq->do_targetq = ctxt;
	_dispatch_release(prev_dq);
}

void
dispatch_set_target_queue(dispatch_object_t dou, dispatch_queue_t dq)
{
	dispatch_queue_t prev_dq;
	unsigned long type;
	
	if (slowpath(dou._do->do_xref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT)) {
		return;
	}
	type = dx_type(dou._do) & _DISPATCH_META_TYPE_MASK;
	if (slowpath(!dq)) {
		bool is_concurrent_q = (type == _DISPATCH_QUEUE_TYPE &&
								slowpath(dou._dq->dq_width > 1));
		dq = _dispatch_get_root_queue(0, !is_concurrent_q);
	}
	// TODO: put into the vtable
	switch(type) {
		case _DISPATCH_QUEUE_TYPE:
		case _DISPATCH_SOURCE_TYPE:
			_dispatch_retain(dq);
			return dispatch_barrier_async_f(dou._dq, dq,
											_dispatch_set_target_queue2);
#if WITH_DISPATCH_IO
		case _DISPATCH_IO_TYPE:
			return _dispatch_io_set_target_queue(dou._dchannel, dq);
#endif
		default:
			_dispatch_retain(dq);
			dispatch_atomic_store_barrier();
			prev_dq = dispatch_atomic_xchg2o(dou._do, do_targetq, dq);
			if (prev_dq) _dispatch_release(prev_dq);
			return;
	}
}

void
dispatch_set_current_target_queue(dispatch_queue_t dq)
{
	dispatch_queue_t queue = _dispatch_queue_get_current();
	
	if (slowpath(!queue)) {
		DISPATCH_CLIENT_CRASH("SPI not called from a queue");
	}
	if (slowpath(queue->do_xref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT)) {
		DISPATCH_CLIENT_CRASH("SPI not supported on this queue");
	}
	if (slowpath(queue->dq_width != 1)) {
		DISPATCH_CLIENT_CRASH("SPI not called from a serial queue");
	}
	if (slowpath(!dq)) {
		dq = _dispatch_get_root_queue(0, true);
	}
	_dispatch_retain(dq);
	_dispatch_set_target_queue2(dq);
}
