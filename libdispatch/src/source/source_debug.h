//
//  source_debug.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#ifndef source_debug_h
#define source_debug_h

#include <stdio.h>

#if DISPATCH_DEBUG
static void _dispatch_kevent_debugger(void *context);
#endif

#endif /* source_debug_h */
