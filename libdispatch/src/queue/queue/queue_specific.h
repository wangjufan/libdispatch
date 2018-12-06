
#ifndef queue_specific_h
#define queue_specific_h

#include <stdio.h>

/*!
 * @functiongroup Dispatch queue-specific contexts
 * This API allows different subsystems to associate context to a shared queue
 * without risk of collision and to retrieve that context from blocks executing
 * on that queue or any of its child queues in the target queue hierarchy.
 */

/*!
 * @function dispatch_queue_set_specific
 *
 * @abstract
 * Associates a subsystem-specific context with a dispatch queue, for a key
 * unique to the subsystem.
 *
 * @discussion
 * The specified destructor will be invoked with the context on the default
 * priority global concurrent queue when a new context is set for the same key,
 * or after all references to the queue have been released.
 *
 * @param queue
 * The dispatch queue to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param key
 * The key to set the context for, typically a pointer to a static variable
 * specific to the subsystem. Keys are only compared as pointers and never
 * dereferenced. Passing a string constant directly is not recommended.
 * The NULL key is reserved and attemps to set a context for it are ignored.
 *
 * @param context
 * The new subsystem-specific context for the object. This may be NULL.
 *
 * @param destructor
 * The destructor function pointer. This may be NULL and is ignored if context
 * is NULL.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_5_0)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NONNULL2 DISPATCH_NOTHROW
void
dispatch_queue_set_specific(dispatch_queue_t queue, const void *key,
							void *context, dispatch_function_t destructor);

/*!
 * @function dispatch_queue_get_specific
 *
 * @abstract
 * Returns the subsystem-specific context associated with a dispatch queue, for
 * a key unique to the subsystem.
 *
 * @discussion
 * Returns the context for the specified key if it has been set on the specified
 * queue.
 *
 * @param queue
 * The dispatch queue to query.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param key
 * The key to get the context for, typically a pointer to a static variable
 * specific to the subsystem. Keys are only compared as pointers and never
 * dereferenced. Passing a string constant directly is not recommended.
 *
 * @result
 * The context for the specified key or NULL if no context was found.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_5_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_PURE DISPATCH_WARN_RESULT
DISPATCH_NOTHROW
void *
dispatch_queue_get_specific(dispatch_queue_t queue, const void *key);

/*!
 * @function dispatch_get_specific
 *
 * @abstract
 * Returns the current subsystem-specific context for a key unique to the
 * subsystem.
 *
 * @discussion
 * When called from a block executing on a queue, returns the context for the
 * specified key if it has been set on the queue, otherwise returns the result
 * of dispatch_get_specific() executed on the queue's target queue or NULL
 * if the current queue is a global concurrent queue.
 *
 * @param key
 * The key to get the context for, typically a pointer to a static variable
 * specific to the subsystem. Keys are only compared as pointers and never
 * dereferenced. Passing a string constant directly is not recommended.
 *
 * @result
 * The context for the specified key or NULL if no context was found.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_5_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_PURE DISPATCH_WARN_RESULT
DISPATCH_NOTHROW
void *
dispatch_get_specific(const void *key);

#endif /* queue_specific_h */
