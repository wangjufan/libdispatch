
#ifndef continuation_push_h
#define continuation_push_h

void _dispatch_queue_push_list_slow(dispatch_queue_t dq,
							   struct dispatch_object_s *obj, unsigned int n);
void _dispatch_queue_push_slow(dispatch_queue_t dq,
							   struct dispatch_object_s *obj);

///////////////////////////////////

#if !__OBJC2__

DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_queue_push_list(dispatch_queue_t dq,  //used by _dispatch_apply_f2 only
						  dispatch_object_t _head,
						  dispatch_object_t _tail, unsigned int n)
{
	struct dispatch_object_s  *head = _head._do,
							*tail = _tail._do;
	if (!fastpath(_dispatch_queue_push_list2(dq, head, tail))) {
		_dispatch_queue_push_list_slow(dq, head, n);
	}
}

DISPATCH_ALWAYS_INLINE  //used much ****
static inline void
_dispatch_queue_push(dispatch_queue_t dq, dispatch_object_t _tail)
{
	struct dispatch_object_s *tail = _tail._do;
	if (!fastpath(_dispatch_queue_push_list2(dq, tail, tail))) {
		_dispatch_queue_push_slow(dq, tail);
	}
}

#endif // !__OBJC2__

DISPATCH_ALWAYS_INLINE
static inline bool
_dispatch_queue_push_list2(dispatch_queue_t dq,  //return the head
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

#endif

/////////////////////////////////////////
/////////////////////////////////////////

DISPATCH_NOINLINE
void _dispatch_queue_push_list_slow(dispatch_queue_t dq,
							   struct dispatch_object_s *obj, unsigned int n)
{
	if (dx_type(dq) == DISPATCH_QUEUE_GLOBAL_TYPE) {
		dq->dq_items_head = obj;
		return _dispatch_queue_wakeup_global2(dq, n);
	}
	_dispatch_queue_push_list_slow2(dq, obj);
}
DISPATCH_NOINLINE
void _dispatch_queue_push_slow(dispatch_queue_t dq,
						  struct dispatch_object_s *obj)
{
	if (dx_type(dq) == DISPATCH_QUEUE_GLOBAL_TYPE) {
		dq->dq_items_head = obj;
		return _dispatch_queue_wakeup_global(dq);//real wake up
	}
	_dispatch_queue_push_list_slow2(dq, obj);
}

// desinged method
DISPATCH_NOINLINE
static void _dispatch_queue_push_list_slow2(dispatch_queue_t dq,
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

#endif /* continuation_push_h */
