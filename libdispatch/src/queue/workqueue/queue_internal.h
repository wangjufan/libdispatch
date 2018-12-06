
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
void _dispatch_queue_push_list_slow(dispatch_queue_t dq,
		struct dispatch_object_s *obj, unsigned int n);
void _dispatch_queue_push_slow(dispatch_queue_t dq,
		struct dispatch_object_s *obj);
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




#if !__OBJC2__

DISPATCH_ALWAYS_INLINE
static inline bool
_dispatch_queue_push_list2(dispatch_queue_t dq,
						   struct dispatch_object_s *head,
		                   struct dispatch_object_s *tail)
{
	struct dispatch_object_s *prev;
	tail->do_next = NULL;
	dispatch_atomic_store_barrier();
	prev = dispatch_atomic_xchg2o(dq, dq_items_tail, tail);
	if (fastpath(prev)) {
		// if we crash here with a value less than 0x1000, then we are at a
		// known bug in client code for example, see _dispatch_queue_dispose
		// or _dispatch_atfork_child
		prev->do_next = head;
	}
	return prev;
}

DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_queue_push_list(dispatch_queue_t dq,
						  dispatch_object_t _head,
						  dispatch_object_t _tail, unsigned int n)
{
	struct dispatch_object_s *head = _head._do, *tail = _tail._do;
	if (!fastpath(_dispatch_queue_push_list2(dq, head, tail))) {
		_dispatch_queue_push_list_slow(dq, head, n);
	}
}

DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_queue_push(dispatch_queue_t dq, dispatch_object_t _tail)
{
	struct dispatch_object_s *tail = _tail._do;
	if (!fastpath(_dispatch_queue_push_list2(dq, tail, tail))) {
		_dispatch_queue_push_slow(dq, tail);
	}
}


#endif // !__OBJC2__

#endif

// desinged method
DISPATCH_NOINLINE
static void
_dispatch_queue_push_list_slow2(dispatch_queue_t dq,
								struct dispatch_object_s *obj)
{
	// The queue must be retained before dq_items_head is written in order
	// to ensure that the reference is still valid when _dispatch_wakeup is
	// called. Otherwise, if preempted between the assignment to
	// dq_items_head and _dispatch_wakeup, the blocks submitted to the
	// queue may release the last reference to the queue when invoked by
	// _dispatch_queue_drain. <rdar://problem/6932776>
	_dispatch_retain(dq);
	dq->dq_items_head = obj;
	_dispatch_wakeup(dq);
	_dispatch_release(dq);
}

DISPATCH_NOINLINE
void
_dispatch_queue_push_list_slow(dispatch_queue_t dq,
							   struct dispatch_object_s *obj, unsigned int n)
{
	if (dx_type(dq) == DISPATCH_QUEUE_GLOBAL_TYPE) {
		dq->dq_items_head = obj;
		return _dispatch_queue_wakeup_global2(dq, n);
	}
	_dispatch_queue_push_list_slow2(dq, obj);
}
DISPATCH_NOINLINE
void
_dispatch_queue_push_slow(dispatch_queue_t dq,
						  struct dispatch_object_s *obj)
{
	if (dx_type(dq) == DISPATCH_QUEUE_GLOBAL_TYPE) {
		dq->dq_items_head = obj;
		return _dispatch_queue_wakeup_global(dq);
	}
	_dispatch_queue_push_list_slow2(dq, obj);
}

