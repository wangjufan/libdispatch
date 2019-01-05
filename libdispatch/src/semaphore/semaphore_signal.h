//
//  semaphore_signal.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/6.
//

#ifndef semaphore_signal_h
#define semaphore_signal_h

#include <stdio.h>

/*!
 * @function dispatch_semaphore_signal
 *
 * @abstract
 * Signal (increment) a semaphore.
 *
 * @discussion
 * Increment the counting semaphore. If the previous value was less than zero,
 * this function wakes a waiting thread before returning.
 *
 * @param dsema The counting semaphore.
 * The result of passing NULL in this parameter is undefined.
 *
 * @result
 * This function returns non-zero if a thread is woken. Otherwise, zero is
 * returned.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
long
dispatch_semaphore_signal(dispatch_semaphore_t dsema);

#endif /* semaphore_signal_h */
