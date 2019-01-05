
#ifndef __DISPATCH_QUEUE_INTERNAL__
#define __DISPATCH_QUEUE_INTERNAL__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include <dispatch/base.h> // for HeaderDoc
#endif

struct dispatch_apply_s {
	size_t da_index;
	size_t da_iterations;
	void (*da_func)(void *, size_t);
	void *da_ctxt;
	_dispatch_thread_semaphore_t da_sema;
	dispatch_queue_t da_queue;
	size_t da_done;
	uint32_t da_thr_cnt;
};

typedef struct dispatch_apply_s *dispatch_apply_t;

DISPATCH_CLASS_DECL(queue_attr);
struct dispatch_queue_attr_s {
	DISPATCH_STRUCT_HEADER(queue_attr);
};

#define DISPATCH_QUEUE_MIN_LABEL_SIZE 64

#ifdef __LP64__
#define DISPATCH_QUEUE_CACHELINE_PAD (4*sizeof(void*))
#else
#define DISPATCH_QUEUE_CACHELINE_PAD (2*sizeof(void*))
#endif

#define DISPATCH_QUEUE_HEADER \
	uint32_t volatile dq_running; \
	uint32_t dq_width; \
	struct dispatch_object_s *volatile dq_items_tail; \
	struct dispatch_object_s *volatile dq_items_head; \
	unsigned long dq_serialnum; \
	dispatch_queue_t dq_specific_q;

DISPATCH_CLASS_DECL(queue);
struct dispatch_queue_s {
	DISPATCH_STRUCT_HEADER(queue);
	DISPATCH_QUEUE_HEADER;
	char dq_label[DISPATCH_QUEUE_MIN_LABEL_SIZE]; // must be last
	char _dq_pad[DISPATCH_QUEUE_CACHELINE_PAD]; // for static queues only
};

DISPATCH_INTERNAL_SUBCLASS_DECL(queue_root, queue);
DISPATCH_INTERNAL_SUBCLASS_DECL(queue_mgr, queue);

DISPATCH_DECL_INTERNAL_SUBCLASS(dispatch_queue_specific_queue, dispatch_queue);
DISPATCH_CLASS_DECL(queue_specific_queue);


extern struct dispatch_queue_s _dispatch_mgr_q;

void _dispatch_queue_dispose(dispatch_queue_t dq);
void _dispatch_queue_invoke(dispatch_queue_t dq);


dispatch_queue_t _dispatch_wakeup(dispatch_object_t dou);
void _dispatch_queue_specific_queue_dispose(dispatch_queue_specific_queue_t
		dqsq);
bool _dispatch_queue_probe_root(dispatch_queue_t dq);
bool _dispatch_mgr_wakeup(dispatch_queue_t dq);
DISPATCH_NORETURN
dispatch_queue_t _dispatch_mgr_thread(dispatch_queue_t dq);


#if DISPATCH_DEBUG
void dispatch_debug_queue(dispatch_queue_t dq, const char* str);
#else
static inline void dispatch_debug_queue(dispatch_queue_t dq DISPATCH_UNUSED,
		const char* str DISPATCH_UNUSED) {}
#endif

size_t dispatch_queue_debug(dispatch_queue_t dq, char* buf, size_t bufsiz);
size_t _dispatch_queue_debug_attr(dispatch_queue_t dq, char* buf,
		size_t bufsiz);

#define DISPATCH_QUEUE_PRIORITY_COUNT 4
#define DISPATCH_ROOT_QUEUE_COUNT (DISPATCH_QUEUE_PRIORITY_COUNT * 2)

// overcommit priority index values need bit 1 set
enum {
	DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY = 0,
	DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY,
	DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY,
	DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY,
	DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY,
	DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY,
	DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY,
	DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY,
};

extern unsigned long _dispatch_queue_serial_numbers;
extern struct dispatch_queue_s _dispatch_root_queues[];



