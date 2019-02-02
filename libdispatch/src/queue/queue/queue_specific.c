
#include "queue_specific.h"

#pragma mark -
#pragma mark dispatch_queue_specific

struct dispatch_queue_specific_queue_s {
	DISPATCH_STRUCT_HEADER(queue_specific_queue);
	DISPATCH_QUEUE_HEADER;
	union {
		char _dqsq_pad[DISPATCH_QUEUE_MIN_LABEL_SIZE];
		struct {
			char dq_label[16];
			TAILQ_HEAD(dispatch_queue_specific_head_s,
					   dispatch_queue_specific_s) dqsq_contexts;
		};
	};
};
struct dispatch_queue_specific_s {
	const void *dqs_key;
	void *dqs_ctxt;
	dispatch_function_t dqs_destructor;
	TAILQ_ENTRY(dispatch_queue_specific_s) dqs_list;
};
DISPATCH_DECL(dispatch_queue_specific);


void
_dispatch_queue_specific_queue_dispose(dispatch_queue_specific_queue_t dqsq)
{
	dispatch_queue_specific_t dqs, tmp;
	
	TAILQ_FOREACH_SAFE(dqs, &dqsq->dqsq_contexts, dqs_list, tmp) {
		if (dqs->dqs_destructor) {
			dispatch_async_f(_dispatch_get_root_queue(
									DISPATCH_QUEUE_PRIORITY_DEFAULT, false),
							 dqs->dqs_ctxt,
							 dqs->dqs_destructor);
		}
		free(dqs);
	}
	_dispatch_queue_dispose((dispatch_queue_t)dqsq);
}

static void
_dispatch_queue_init_specific(dispatch_queue_t dq)
{
	dispatch_queue_specific_queue_t dqsq;
	
	dqsq = _dispatch_alloc(DISPATCH_VTABLE(queue_specific_queue),
						   sizeof(struct dispatch_queue_specific_queue_s));
	_dispatch_queue_init((dispatch_queue_t)dqsq);
	dqsq->do_xref_cnt = -1;
	dqsq->do_targetq = _dispatch_get_root_queue(DISPATCH_QUEUE_PRIORITY_HIGH,
												true);
	dqsq->dq_width = UINT32_MAX;
	strlcpy(dqsq->dq_label, "queue-specific", sizeof(dqsq->dq_label));
	TAILQ_INIT(&dqsq->dqsq_contexts);
	dispatch_atomic_store_barrier();
	if (slowpath(!dispatch_atomic_cmpxchg2o(dq, dq_specific_q, NULL,
											(dispatch_queue_t)dqsq))) {
		_dispatch_release((dispatch_queue_t)dqsq);
	}
}

static void
_dispatch_queue_set_specific(void *ctxt)
{
	dispatch_queue_specific_t dqs, dqsn = ctxt;
	dispatch_queue_specific_queue_t dqsq =
	(dispatch_queue_specific_queue_t)_dispatch_queue_get_current();
	
	TAILQ_FOREACH(dqs, &dqsq->dqsq_contexts, dqs_list) {
		if (dqs->dqs_key == dqsn->dqs_key) {
			// Destroy previous context for existing key
			if (dqs->dqs_destructor) {
				dispatch_async_f(_dispatch_get_root_queue(
														  DISPATCH_QUEUE_PRIORITY_DEFAULT, false), dqs->dqs_ctxt,
								 dqs->dqs_destructor);
			}
			if (dqsn->dqs_ctxt) {
				// Copy new context for existing key
				dqs->dqs_ctxt = dqsn->dqs_ctxt;
				dqs->dqs_destructor = dqsn->dqs_destructor;
			} else {
				// Remove context storage for existing key
				TAILQ_REMOVE(&dqsq->dqsq_contexts, dqs, dqs_list);
				free(dqs);
			}
			return free(dqsn);
		}
	}
	// Insert context storage for new key
	TAILQ_INSERT_TAIL(&dqsq->dqsq_contexts, dqsn, dqs_list);
}

DISPATCH_NOINLINE
void
dispatch_queue_set_specific(dispatch_queue_t dq, const void *key,
							void *ctxt, dispatch_function_t destructor)
{
	if (slowpath(!key)) {
		return;
	}
	dispatch_queue_specific_t dqs;
	
	dqs = calloc(1, sizeof(struct dispatch_queue_specific_s));
	dqs->dqs_key = key;
	dqs->dqs_ctxt = ctxt;
	dqs->dqs_destructor = destructor;
	if (slowpath(!dq->dq_specific_q)) {
		_dispatch_queue_init_specific(dq);
	}
	dispatch_barrier_async_f(dq->dq_specific_q, dqs,
							 _dispatch_queue_set_specific);
}

static void
_dispatch_queue_get_specific(void *ctxt)
{
	void **ctxtp = ctxt;
	void *key = *ctxtp;
	dispatch_queue_specific_queue_t dqsq =
	(dispatch_queue_specific_queue_t)_dispatch_queue_get_current();
	dispatch_queue_specific_t dqs;
	
	TAILQ_FOREACH(dqs, &dqsq->dqsq_contexts, dqs_list) {
		if (dqs->dqs_key == key) {
			*ctxtp = dqs->dqs_ctxt;
			return;
		}
	}
	*ctxtp = NULL;
}

DISPATCH_NOINLINE
void *
dispatch_queue_get_specific(dispatch_queue_t dq, const void *key)
{
	if (slowpath(!key)) {
		return NULL;
	}
	void *ctxt = NULL;
	
	if (fastpath(dq->dq_specific_q)) {
		ctxt = (void *)key;
		dispatch_sync_f(dq->dq_specific_q, &ctxt, _dispatch_queue_get_specific);
	}
	return ctxt;
}

DISPATCH_NOINLINE
void *
dispatch_get_specific(const void *key)
{
	if (slowpath(!key)) {
		return NULL;
	}
	void *ctxt = NULL;
	dispatch_queue_t dq = _dispatch_queue_get_current();
	
	while (slowpath(dq)) {
		if (slowpath(dq->dq_specific_q)) {
			ctxt = (void *)key;
			dispatch_sync_f(dq->dq_specific_q, &ctxt,
							_dispatch_queue_get_specific);
			if (ctxt) break;
		}
		dq = dq->do_targetq;
	}
	return ctxt;
}
