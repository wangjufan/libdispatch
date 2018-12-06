//
//  source_mach.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#ifndef source_mach_h
#define source_mach_h

#include <stdio.h>

#if HAVE_MACH
static kern_return_t _dispatch_kevent_machport_resume(dispatch_kevent_t dk,
													  uint32_t new_flags, uint32_t del_flags);
static void _dispatch_drain_mach_messages(struct kevent *ke);
#endif



__BEGIN_DECLS
#if TARGET_OS_MAC
/*!
 * @typedef dispatch_mig_callback_t
 *
 * @abstract
 * The signature of a function that handles Mach message delivery and response.
 */
typedef boolean_t (*dispatch_mig_callback_t)(mach_msg_header_t *message,
											 mach_msg_header_t *reply);
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
mach_msg_return_t
dispatch_mig_server(dispatch_source_t ds, size_t maxmsgsz,
					dispatch_mig_callback_t callback);
#endif
__END_DECLS


#endif /* source_mach_h */
