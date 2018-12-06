//
//  barrier_sync.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef barrier_sync_h
#define barrier_sync_h

#include <stdio.h>

/*!
 * @function dispatch_barrier_sync
 *
 * @abstract
 * Submits a barrier block for synchronous execution on a dispatch queue.
 *
 * @discussion
 * Submits a block to a dispatch queue like dispatch_sync(), but marks that
 * block as a barrier (relevant only on DISPATCH_QUEUE_CONCURRENT queues).
 *
 * See dispatch_sync() for details.
 *
 * @param queue
 * The target dispatch queue to which the block is submitted.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param block
 * The block to be invoked on the target dispatch queue.
 * The result of passing NULL in this parameter is undefined.
 */
#ifdef __BLOCKS__
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_4_3)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_barrier_sync(dispatch_queue_t queue, dispatch_block_t block);
#endif

/*!
 * @function dispatch_barrier_sync_f
 *
 * @abstract
 * Submits a barrier function for synchronous execution on a dispatch queue.
 *
 * @discussion
 * Submits a function to a dispatch queue like dispatch_sync_f(), but marks that
 * fuction as a barrier (relevant only on DISPATCH_QUEUE_CONCURRENT queues).
 *
 * See dispatch_sync_f() for details.
 *
 * @param queue
 * The target dispatch queue to which the function is submitted.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param context
 * The application-defined context parameter to pass to the function.
 *
 * @param work
 * The application-defined function to invoke on the target queue. The first
 * parameter passed to this function is the context provided to
 * dispatch_barrier_sync_f().
 * The result of passing NULL in this parameter is undefined.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_4_3)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NONNULL3 DISPATCH_NOTHROW
void
dispatch_barrier_sync_f(dispatch_queue_t queue,
						void *context,
						dispatch_function_t work);


#endif /* barrier_sync_h */
