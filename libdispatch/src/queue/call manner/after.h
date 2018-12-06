//
//  after.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef after_h
#define after_h

#include <stdio.h>

struct _dispatch_after_time_s {
	void *datc_ctxt;
	void (*datc_func)(void *);
	dispatch_source_t ds;
};

#endif /* after_h */
