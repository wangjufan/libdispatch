//
//  source_handler.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#ifndef source_handler_h
#define source_handler_h

#include <stdio.h>

/*!
 * @function dispatch_source_set_event_handler
 *
 * @abstract
 * Sets the event handler block for the given dispatch source.
 *
 * @param source
 * The dispatch source to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param handler
 * The event handler block to submit to the source's target queue.
 */
#ifdef __BLOCKS__
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_event_handler(dispatch_source_t source,
								  dispatch_block_t handler);
#endif /* __BLOCKS__ */

/*!
 * @function dispatch_source_set_event_handler_f
 *
 * @abstract
 * Sets the event handler function for the given dispatch source.
 *
 * @param source
 * The dispatch source to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param handler
 * The event handler function to submit to the source's target queue.
 * The context parameter passed to the event handler function is the current
 * context of the dispatch source at the time the handler call is made.
 * The result of passing NULL in this parameter is undefined.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_event_handler_f(dispatch_source_t source,
									dispatch_function_t handler);

/*!
 * @function dispatch_source_set_cancel_handler
 *
 * @abstract
 * Sets the cancellation handler block for the given dispatch source.
 *
 * @discussion
 * The cancellation handler (if specified) will be submitted to the source's
 * target queue in response to a call to dispatch_source_cancel() once the
 * system has released all references to the source's underlying handle and
 * the source's event handler block has returned.
 *
 * IMPORTANT:
 * A cancellation handler is required for file descriptor and mach port based
 * sources in order to safely close the descriptor or destroy the port. Closing
 * the descriptor or port before the cancellation handler may result in a race
 * condition. If a new descriptor is allocated with the same value as the
 * recently closed descriptor while the source's event handler is still running,
 * the event handler may read/write data to the wrong descriptor.
 *
 * @param source
 * The dispatch source to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param handler
 * The cancellation handler block to submit to the source's target queue.
 */
#ifdef __BLOCKS__
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_cancel_handler(dispatch_source_t source,
								   dispatch_block_t cancel_handler);
#endif /* __BLOCKS__ */

/*!
 * @function dispatch_source_set_cancel_handler_f
 *
 * @abstract
 * Sets the cancellation handler function for the given dispatch source.
 *
 * @discussion
 * See dispatch_source_set_cancel_handler() for more details.
 *
 * @param source
 * The dispatch source to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param handler
 * The cancellation handler function to submit to the source's target queue.
 * The context parameter passed to the event handler function is the current
 * context of the dispatch source at the time the handler call is made.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_cancel_handler_f(dispatch_source_t source,
									 dispatch_function_t cancel_handler);



/*!
 * @function dispatch_source_set_registration_handler
 *
 * @abstract
 * Sets the registration handler block for the given dispatch source.
 *
 * @discussion
 * The registration handler (if specified) will be submitted to the source's
 * target queue once the corresponding kevent() has been registered with the
 * system, following the initial dispatch_resume() of the source.
 *
 * If a source is already registered when the registration handler is set, the
 * registration handler will be invoked immediately.
 *
 * @param source
 * The dispatch source to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param handler
 * The registration handler block to submit to the source's target queue.
 */
#ifdef __BLOCKS__
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_4_3)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_registration_handler(dispatch_source_t source,
										 dispatch_block_t registration_handler);
#endif /* __BLOCKS__ */
/*!
 * @function dispatch_source_set_registration_handler_f
 *
 * @abstract
 * Sets the registration handler function for the given dispatch source.
 *
 * @discussion
 * See dispatch_source_set_registration_handler() for more details.
 *
 * @param source
 * The dispatch source to modify.
 * The result of passing NULL in this parameter is undefined.
 *
 * @param handler
 * The registration handler function to submit to the source's target queue.
 * The context parameter passed to the registration handler function is the
 * current context of the dispatch source at the time the handler call is made.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_7,__IPHONE_4_3)
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_registration_handler_f(dispatch_source_t source,
										   dispatch_function_t registration_handler);

__END_DECLS

#endif


#endif /* source_handler_h */
