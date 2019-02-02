//
//  invoke.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef invoke_h
#define invoke_h

#include <stdio.h>

struct dispatch_function_recurse_s {
	dispatch_queue_t dfr_dq;
	void* dfr_ctxt;
	dispatch_function_t dfr_func;
};

#endif /* invoke_h */
