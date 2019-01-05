//
//  queue_drain.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef queue_drain_h
#define queue_drain_h

#include <stdio.h>


#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
static void _dispatch_main_queue_drain(void);
#endif

#endif /* queue_drain_h */
