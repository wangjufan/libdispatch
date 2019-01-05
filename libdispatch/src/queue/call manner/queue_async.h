//
//  queue_async.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef queue_async_h
#define queue_async_h

#include <stdio.h>

/*!
 * @function dispatch_async
 *
 * @abstract
 * Submits a block for asynchronous execution on a dispatch queue.
 *
 * @discussion
 * The dispatch_async() function is the fundamental mechanism for submitting
 * blocks to a dispatch queue.
 *
 * Calls to dispatch_async() always return immediately after the block has
 * been submitted, and never wait for the block to be invoked.
 *
 * The target queue determines whether the block will be invoked serially or
 * concurrently with respect to other blocks submitted to that same queue.
 * Serial queues are processed concurrently with respect to each other.
 *
 * @param queue
 * The target dispatch queue to which the block is submitted.
 * The system will hold a reference on the target queue until the block
 * has finished.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param block
 * The block to submit to the target dispatch queue. This function performs
 * Block_copy() and Block_release() on behalf of callers.
 * The result of passing NULL in this parameter is undefined.
 */
#ifdef __BLOCKS__
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_async(dispatch_queue_t queue, dispatch_block_t block);
#endif

/*!
 * @function dispatch_async_f
 *
 * @abstract
 * Submits a function for asynchronous execution on a dispatch queue.
 *
 * @discussion
 * See dispatch_async() for details.
 *
 * @param queue
 * The target dispatch queue to which the function is submitted.
 * The system will hold a reference on the target queue until the function
 * has returned.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param context
 * The application-defined context parameter to pass to the function.
 *
 * @param work
 * The application-defined function to invoke on the target queue. The first
 * parameter passed to this function is the context provided to
 * dispatch_async_f().
 * The result of passing NULL in this parameter is undefined.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NONNULL3 DISPATCH_NOTHROW
void
dispatch_async_f(dispatch_queue_t queue,
				 void *context,
				 dispatch_function_t work);

#endif /* queue_async_h */


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

DISPATCH_NOINLINE//内核通信 慢
static void
_dispatch_async_f2_slow(dispatch_queue_t dq, dispatch_continuation_t dc)
{
	_dispatch_wakeup(dq);
	_dispatch_queue_push(dq, dc);
}

DISPATCH_NOINLINE //创建继续 慢
static void
_dispatch_async_f_slow(dispatch_queue_t dq, void *ctxt,
					   dispatch_function_t func)
{
	dispatch_continuation_t dc = _dispatch_continuation_alloc_from_heap();
	
	dc->do_vtable = (void *)DISPATCH_OBJ_ASYNC_BIT;
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->do_targetq) {//b目标队列
		return _dispatch_async_f2(dq, dc);
	}
	
	_dispatch_queue_push(dq, dc);
}

DISPATCH_NOINLINE
void
dispatch_async_f(dispatch_queue_t dq,
				 void *ctxt,
				 dispatch_function_t func)
{
	dispatch_continuation_t dc;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->dq_width == 1) {//
		return dispatch_barrier_async_f(dq, ctxt, func);
	}
	
	dc = fastpath(_dispatch_continuation_alloc_cacheonly());
	if (!dc) {
		return _dispatch_async_f_slow(dq, ctxt, func);
	}
	
	dc->do_vtable = (void *)DISPATCH_OBJ_ASYNC_BIT;
	dc->dc_func = func;
	dc->dc_ctxt = ctxt;
	
	// No fastpath/slowpath hint because we simply don't know
	if (dq->do_targetq) {
		return _dispatch_async_f2(dq, dc);
	}
	
	_dispatch_queue_push(dq, dc);
}

#ifdef __BLOCKS__
void
dispatch_async(dispatch_queue_t dq, void (^work)(void))
{
	dispatch_async_f(dq,
					 _dispatch_Block_copy(work),
					 _dispatch_call_block_and_release);
}
#endif
 
