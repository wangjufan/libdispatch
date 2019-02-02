//
//  source_timer.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#ifndef source_timer_h
#define source_timer_h

#include <stdio.h>

#define	TAILQ_INSERT_TAIL(head, elm, field) do {			\
	QMD_TAILQ_CHECK_TAIL(head, field);				\
	TAILQ_NEXT((elm), field) = NULL;				\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &TAILQ_NEXT((elm), field);			\
	QMD_TRACE_HEAD(head);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

struct dispatch_timer_source_s {
	uint64_t target;
	uint64_t last_fire;
	uint64_t interval;
	uint64_t leeway;
	uint64_t flags; // dispatch_timer_flags_t
	unsigned long missed;
};


struct dispatch_timer_source_refs_s {
	struct dispatch_source_refs_s _ds_refs;
	struct dispatch_timer_source_s _ds_timer;
};

static inline void _dispatch_source_timer_init(void);
static void _dispatch_timer_list_update(dispatch_source_t ds);
static inline unsigned long _dispatch_source_timer_data(
														dispatch_source_refs_t dr, unsigned long prev);

#endif /* source_timer_h */
