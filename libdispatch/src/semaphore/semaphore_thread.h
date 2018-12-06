//
//  semaphore_thread.h
//  libdispatch
//
//  Created by 王举范 on 2018/11/27.
//

#ifndef semaphore_thread_h
#define semaphore_thread_h

#include <stdio.h>

/*
 *	struct waitq
 *
 *	This is the definition of the common event wait queue
 *	that the scheduler APIs understand.
 *	It is used internally by the gerneralized event waiting mechanism (assert_wait),
 *   and also for items that maintain their own wait queues (such as ports and semaphores).
 *
 *	It is not published to other kernel components.
 *
 *	NOTE:  Hardware locks are used to protect event wait
 *	queues since interrupt code is free to post events to
 *	them.
 */
struct waitq {
	uint32_t /* flags */
waitq_type:2,    /* only public field */
waitq_fifo:1,    /* fifo wakeup policy? */
waitq_prepost:1, /* waitq supports prepost? */
waitq_irq:1,     /* waitq requires interrupts disabled */
waitq_isvalid:1, /* waitq structure is valid */
waitq_eventmask:_EVENT_MASK_BITS;
	/* the wait queue set (set-of-sets) to which this queue belongs */
#if __arm64__
	hw_lock_bit_t	waitq_interlock;	/* interlock */
#else
	hw_lock_data_t	waitq_interlock;	/* interlock */
#endif /* __arm64__ */
	
	uint64_t waitq_set_id;
	uint64_t waitq_prepost_id;
	queue_head_t	waitq_queue;		/* queue of elements */
};


///////////////////////////////////////////////////
typedef uintptr_t _dispatch_thread_semaphore_t;

_dispatch_thread_semaphore_t _dispatch_get_thread_semaphore(void);
void _dispatch_put_thread_semaphore(_dispatch_thread_semaphore_t);
void _dispatch_thread_semaphore_wait(_dispatch_thread_semaphore_t);
void _dispatch_thread_semaphore_signal(_dispatch_thread_semaphore_t);
void _dispatch_thread_semaphore_dispose(_dispatch_thread_semaphore_t);

#endif /* semaphore_thread_h */
