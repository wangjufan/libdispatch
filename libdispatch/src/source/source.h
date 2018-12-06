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

#ifndef __DISPATCH_SOURCE__
#define __DISPATCH_SOURCE__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include <dispatch/base.h> // for HeaderDoc
#endif

#if TARGET_OS_MAC
#include <mach/port.h>
#include <mach/message.h>
#endif
#include <sys/signal.h>


__BEGIN_DECLS

/*!
 * @function dispatch_source_create
 *
 * @abstract
 * Creates a new dispatch source to monitor low-level system objects and auto-
 * matically submit a handler block to a dispatch queue in response to events.
 *
 * @discussion
 * Dispatch sources are not reentrant. Any events received while the dispatch
 * source is suspended or while the event handler block is currently executing
 * will be coalesced and delivered after the dispatch source is resumed or the
 * event handler block has returned.
 *
 * Dispatch sources are created in a suspended state. After creating the
 * source and setting any desired attributes (i.e. the handler, context, etc.),
 * a call must be made to dispatch_resume() in order to begin event delivery.
 *
 * @param type
 * Declares the type of the dispatch source. Must be one of the defined
 * dispatch_source_type_t constants.
 * @param handle
 * The underlying system handle to monitor. The interpretation of this argument
 * is determined by the constant provided in the type parameter.
 * @param mask
 * A mask of flags specifying which events are desired. The interpretation of
 * this argument is determined by the constant provided in the type parameter.
 * @param queue
 * The dispatch queue to which the event handler block will be submitted.
 * If queue is DISPATCH_TARGET_QUEUE_DEFAULT, the source will submit the event
 * handler block to the default priority global queue.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_MALLOC DISPATCH_RETURNS_RETAINED DISPATCH_WARN_RESULT
DISPATCH_NOTHROW
dispatch_source_t
dispatch_source_create(dispatch_source_type_t type,
	uintptr_t handle,
	unsigned long mask,
	dispatch_queue_t queue);



/*!
 * @function dispatch_source_cancel
 *
 * @abstract
 * Asynchronously cancel the dispatch source, preventing any further invocation
 * of its event handler block.
 *
 * @discussion
 * Cancellation prevents any further invocation of the event handler block for
 * the specified dispatch source, but does not interrupt an event handler
 * block that is already in progress.
 *
 * The cancellation handler is submitted to the source's target queue once the
 * the source's event handler has finished, indicating it is now safe to close
 * the source's handle (i.e. file descriptor or mach port).
 *
 * See dispatch_source_set_cancel_handler() for more information.
 *
 * @param source
 * The dispatch source to be canceled.
 * The result of passing NULL in this parameter is undefined.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_source_cancel(dispatch_source_t source);

/*!
 * @function dispatch_source_testcancel
 *
 * @abstract
 * Tests whether the given dispatch source has been canceled.
 *
 * @param source
 * The dispatch source to be tested.
 * The result of passing NULL in this parameter is undefined.
 *
 * @result
 * Non-zero if canceled and zero if not canceled.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
long
dispatch_source_testcancel(dispatch_source_t source);

/*!
 * @function dispatch_source_get_handle
 *
 * @abstract
 * Returns the underlying system handle associated with this dispatch source.
 *
 * @param source
 * The result of passing NULL in this parameter is undefined.
 *
 * @result
 * The return value should be interpreted according to the type of the dispatch
 * source, and may be one of the following handles:
 *
 *  DISPATCH_SOURCE_TYPE_DATA_ADD:        n/a
 *  DISPATCH_SOURCE_TYPE_DATA_OR:         n/a
 *  DISPATCH_SOURCE_TYPE_MACH_SEND:       mach port (mach_port_t)
 *  DISPATCH_SOURCE_TYPE_MACH_RECV:       mach port (mach_port_t)
 *  DISPATCH_SOURCE_TYPE_PROC:            process identifier (pid_t)
 *  DISPATCH_SOURCE_TYPE_READ:            file descriptor (int)
 *  DISPATCH_SOURCE_TYPE_SIGNAL:          signal number (int)
 *  DISPATCH_SOURCE_TYPE_TIMER:           n/a
 *  DISPATCH_SOURCE_TYPE_VNODE:           file descriptor (int)
 *  DISPATCH_SOURCE_TYPE_WRITE:           file descriptor (int)
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
uintptr_t
dispatch_source_get_handle(dispatch_source_t source);

/*!
 * @function dispatch_source_get_mask
 *
 * @abstract
 * Returns the mask of events monitored by the dispatch source.
 *
 * @param source
 * The result of passing NULL in this parameter is undefined.
 *
 * @result
 * The return value should be interpreted according to the type of the dispatch
 * source, and may be one of the following flag sets:
 *
 *  DISPATCH_SOURCE_TYPE_DATA_ADD:        n/a
 *  DISPATCH_SOURCE_TYPE_DATA_OR:         n/a
 *  DISPATCH_SOURCE_TYPE_MACH_SEND:       dispatch_source_mach_send_flags_t
 *  DISPATCH_SOURCE_TYPE_MACH_RECV:       n/a
 *  DISPATCH_SOURCE_TYPE_PROC:            dispatch_source_proc_flags_t
 *  DISPATCH_SOURCE_TYPE_READ:            n/a
 *  DISPATCH_SOURCE_TYPE_SIGNAL:          n/a
 *  DISPATCH_SOURCE_TYPE_TIMER:           n/a
 *  DISPATCH_SOURCE_TYPE_VNODE:           dispatch_source_vnode_flags_t
 *  DISPATCH_SOURCE_TYPE_WRITE:           n/a
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
unsigned long
dispatch_source_get_mask(dispatch_source_t source);

/*!
 * @function dispatch_source_get_data
 *
 * @abstract
 * Returns pending data for the dispatch source.
 *
 * @discussion
 * This function is intended to be called from within the event handler block.
 * The result of calling this function outside of the event handler callback is
 * undefined.
 *
 * @param source
 * The result of passing NULL in this parameter is undefined.
 *
 * @result
 * The return value should be interpreted according to the type of the dispatch
 * source, and may be one of the following:
 *
 *  DISPATCH_SOURCE_TYPE_DATA_ADD:        application defined data
 *  DISPATCH_SOURCE_TYPE_DATA_OR:         application defined data
 *  DISPATCH_SOURCE_TYPE_MACH_SEND:       dispatch_source_mach_send_flags_t
 *  DISPATCH_SOURCE_TYPE_MACH_RECV:       n/a
 *  DISPATCH_SOURCE_TYPE_PROC:            dispatch_source_proc_flags_t
 *  DISPATCH_SOURCE_TYPE_READ:            estimated bytes available to read
 *  DISPATCH_SOURCE_TYPE_SIGNAL:          number of signals delivered since
 *                                            the last handler invocation
 *  DISPATCH_SOURCE_TYPE_TIMER:           number of times the timer has fired
 *                                            since the last handler invocation
 *  DISPATCH_SOURCE_TYPE_VNODE:           dispatch_source_vnode_flags_t
 *  DISPATCH_SOURCE_TYPE_WRITE:           estimated buffer space available
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
unsigned long
dispatch_source_get_data(dispatch_source_t source);

/*!
 * @function dispatch_source_merge_data
 *
 * @abstract
 * Merges data into a dispatch source of type DISPATCH_SOURCE_TYPE_DATA_ADD or
 * DISPATCH_SOURCE_TYPE_DATA_OR and submits its event handler block to its
 * target queue.
 *
 * @param source
 * The result of passing NULL in this parameter is undefined.
 *
 * @param value
 * The value to coalesce with the pending data using a logical OR or an ADD
 * as specified by the dispatch source type. A value of zero has no effect
 * and will not result in the submission of the event handler block.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_source_merge_data(dispatch_source_t source, unsigned long value);

/*!
 * @function dispatch_source_set_timer
 *
 * @abstract
 * Sets a start time, interval, and leeway value for a timer source.
 *
 * @discussion
 * Calling this function has no effect if the timer source has already been
 * canceled. Once this function returns, any pending timer data accumulated
 * for the previous timer values has been cleared
 *
 * The start time argument also determines which clock will be used for the
 * timer. If the start time is DISPATCH_TIME_NOW or created with
 * dispatch_time() then the timer is based on mach_absolute_time(). Otherwise,
 * if the start time of the timer is created with dispatch_walltime() then the
 * timer is based on gettimeofday(3).
 *
 * @param start
 * The start time of the timer. See dispatch_time() and dispatch_walltime()
 * for more information.
 *
 * @param interval
 * The nanosecond interval for the timer.
 *
 * @param leeway
 * A hint given to the system by the application for the amount of leeway, in
 * nanoseconds, that the system may defer the timer in order to align with other
 * system activity for improved system performance or power consumption. (For
 * example, an application might perform a periodic task every 5 minutes, with
 * a leeway of up to 30 seconds.)  Note that some latency is to be expected for
 * all timers even when a leeway value of zero is specified.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_source_set_timer(dispatch_source_t source,
	dispatch_time_t start,
	uint64_t interval,
	uint64_t leeway);
