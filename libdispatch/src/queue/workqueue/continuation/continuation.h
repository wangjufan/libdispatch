
#ifndef continuation_h
#define continuation_h

#include <stdio.h>

// If dc_vtable is less than 127, then the object is a continuation.
// Otherwise, the object has a private layout and memory management rules. The
// layout until after 'do_next' must align with normal objects.
#define DISPATCH_CONTINUATION_HEADER(x) \
	_OS_OBJECT_HEADER( \
	const void *do_vtable, \
	do_ref_cnt, \
	do_xref_cnt); \
	struct dispatch_##x##_s *volatile do_next; \
	dispatch_function_t dc_func; \
	void *dc_ctxt; \
	void *dc_data; \
	void *dc_other;

#define DISPATCH_OBJ_ASYNC_BIT		0x1
#define DISPATCH_OBJ_BARRIER_BIT	0x2
#define DISPATCH_OBJ_GROUP_BIT		0x4
#define DISPATCH_OBJ_SYNC_SLOW_BIT	0x8
//they are continuation bits wjf
// vtables are pointers far away from the low page in memory
#define DISPATCH_OBJ_IS_VTABLE(x) ((unsigned long)(x)->do_vtable > 127ul)

struct dispatch_continuation_s {
	DISPATCH_CONTINUATION_HEADER(continuation);
};

					   
							   
DISPATCH_DECL(dispatch_continuation);

//dispatch_continuation_t next = _dispatch_continuation_alloc();
//next->do_vtable = (void *)DISPATCH_OBJ_ASYNC_BIT;


#endif /* continuation_h */
