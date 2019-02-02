//
//  source_timer.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#include "source_timer.h"


#pragma mark -
#pragma mark dispatch_timer

DISPATCH_CACHELINE_ALIGN
static struct dispatch_kevent_s _dispatch_kevent_timer[] = {
	
	[DISPATCH_TIMER_INDEX_WALL] = {
		.dk_kevent = {
			.ident = DISPATCH_TIMER_INDEX_WALL,
			.filter = DISPATCH_EVFILT_TIMER,
			.udata = &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_WALL],
		},
		.dk_sources = TAILQ_HEAD_INITIALIZER(
			_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_WALL].dk_sources),
	},
	[DISPATCH_TIMER_INDEX_MACH] = {
		.dk_kevent = {
			.ident = DISPATCH_TIMER_INDEX_MACH,
			.filter = DISPATCH_EVFILT_TIMER,
			.udata = &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_MACH],
		},
		.dk_sources = TAILQ_HEAD_INITIALIZER(
			_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_MACH].dk_sources),
	},
	[DISPATCH_TIMER_INDEX_DISARM] = {
		.dk_kevent = {
			.ident = DISPATCH_TIMER_INDEX_DISARM,
			.filter = DISPATCH_EVFILT_TIMER,
			.udata = &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_DISARM],
		},
		.dk_sources = TAILQ_HEAD_INITIALIZER(
			_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_DISARM].dk_sources),
	},
};
// Don't count disarmed timer list
#define DISPATCH_TIMER_COUNT ((sizeof(_dispatch_kevent_timer) \
/ sizeof(_dispatch_kevent_timer[0])) - 1)

static inline void
_dispatch_source_timer_init(void)
{
	TAILQ_INSERT_TAIL(&_dispatch_sources[DSL_HASH(DISPATCH_TIMER_INDEX_WALL)],
					  &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_WALL],
					  dk_list);
	
	TAILQ_INSERT_TAIL(&_dispatch_sources[DSL_HASH(DISPATCH_TIMER_INDEX_MACH)],
					  &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_MACH],
					  dk_list);
	
	TAILQ_INSERT_TAIL(&_dispatch_sources[DSL_HASH(DISPATCH_TIMER_INDEX_DISARM)],
					  &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_DISARM],
					  dk_list);
}

DISPATCH_ALWAYS_INLINE
static inline unsigned int
_dispatch_source_timer_idx(dispatch_source_refs_t dr)
{
	return ds_timer(dr).flags & DISPATCH_TIMER_WALL_CLOCK ?
	DISPATCH_TIMER_INDEX_WALL : DISPATCH_TIMER_INDEX_MACH;
}

DISPATCH_ALWAYS_INLINE
static inline uint64_t
_dispatch_source_timer_now2(unsigned int timer)
{
	switch (timer) {
		case DISPATCH_TIMER_INDEX_MACH:
			return _dispatch_absolute_time();
		case DISPATCH_TIMER_INDEX_WALL:
			return _dispatch_get_nanoseconds();
		default:
			DISPATCH_CRASH("Invalid timer");
	}
}

DISPATCH_ALWAYS_INLINE
static inline uint64_t
_dispatch_source_timer_now(dispatch_source_refs_t dr)
{
	return _dispatch_source_timer_now2(_dispatch_source_timer_idx(dr));
}

// Updates the ordered list of timers based on next fire date for changes to ds.
// Should only be called from the context of _dispatch_mgr_q.
static void
_dispatch_timer_list_update(dispatch_source_t ds)
{
	dispatch_source_refs_t dr = ds->ds_refs, dri = NULL;
	
	dispatch_assert(_dispatch_queue_get_current() == &_dispatch_mgr_q);
	
	// do not reschedule timers unregistered with _dispatch_kevent_unregister()
	if (!ds->ds_dkev) {
		return;
	}
	
	// Ensure the source is on the global kevent lists before it is removed and
	// readded below.
	_dispatch_kevent_register(ds);
	
	TAILQ_REMOVE(&ds->ds_dkev->dk_sources, dr, dr_list);
	
	// Move timers that are disabled, suspended or have missed intervals to the
	// disarmed list, rearm after resume resp. source invoke will reenable them
	if (!ds_timer(dr).target || DISPATCH_OBJECT_SUSPENDED(ds) ||
		ds->ds_pending_data) {
		(void)dispatch_atomic_and2o(ds, ds_atomic_flags, ~DSF_ARMED);
		ds->ds_dkev = &_dispatch_kevent_timer[DISPATCH_TIMER_INDEX_DISARM];
		TAILQ_INSERT_TAIL(&ds->ds_dkev->dk_sources, (dispatch_source_refs_t)dr,
						  dr_list);
		return;
	}
	
	// change the list if the clock type has changed
	ds->ds_dkev = &_dispatch_kevent_timer[_dispatch_source_timer_idx(dr)];
	
	TAILQ_FOREACH(dri, &ds->ds_dkev->dk_sources, dr_list) {
		if (ds_timer(dri).target == 0 ||
			ds_timer(dr).target < ds_timer(dri).target) {
			break;
		}
	}
	
	if (dri) {
		TAILQ_INSERT_BEFORE(dri, dr, dr_list);
	} else {
		TAILQ_INSERT_TAIL(&ds->ds_dkev->dk_sources, dr, dr_list);
	}
}

static inline void
_dispatch_run_timers2(unsigned int timer)
{
	dispatch_source_refs_t dr;
	dispatch_source_t ds;
	uint64_t now, missed;
	
	now = _dispatch_source_timer_now2(timer);
	while ((dr = TAILQ_FIRST(&_dispatch_kevent_timer[timer].dk_sources))) {
		ds = _dispatch_source_from_refs(dr);
		// We may find timers on the wrong list due to a pending update from
		// dispatch_source_set_timer. Force an update of the list in that case.
		if (timer != ds->ds_ident_hack) {
			_dispatch_timer_list_update(ds);
			continue;
		}
		if (!ds_timer(dr).target) {
			// no configured timers on the list
			break;
		}
		if (ds_timer(dr).target > now) {
			// Done running timers for now.
			break;
		}
		// Remove timers that are suspended or have missed intervals from the
		// list, rearm after resume resp. source invoke will reenable them
		if (DISPATCH_OBJECT_SUSPENDED(ds) || ds->ds_pending_data) {
			_dispatch_timer_list_update(ds);
			continue;
		}
		// Calculate number of missed intervals.
		missed = (now - ds_timer(dr).target) / ds_timer(dr).interval;
		if (++missed > INT_MAX) {
			missed = INT_MAX;
		}
		ds_timer(dr).target += missed * ds_timer(dr).interval;
		_dispatch_timer_list_update(ds);
		ds_timer(dr).last_fire = now;
		(void)dispatch_atomic_add2o(ds, ds_pending_data, (int)missed);
		_dispatch_wakeup(ds);
	}
}

void
_dispatch_run_timers(void)
{
	dispatch_once_f(&__dispatch_kevent_init_pred,
					NULL, _dispatch_kevent_init);
	
	unsigned int i;
	for (i = 0; i < DISPATCH_TIMER_COUNT; i++) {
		if (!TAILQ_EMPTY(&_dispatch_kevent_timer[i].dk_sources)) {
			_dispatch_run_timers2(i);
		}
	}
}

static inline unsigned long
_dispatch_source_timer_data(dispatch_source_refs_t dr, unsigned long prev)
{
	// calculate the number of intervals since last fire
	unsigned long data, missed;
	uint64_t now = _dispatch_source_timer_now(dr);
	missed = (unsigned long)((now - ds_timer(dr).last_fire) /
							 ds_timer(dr).interval);
	// correct for missed intervals already delivered last time
	data = prev - ds_timer(dr).missed + missed;
	ds_timer(dr).missed = missed;
	return data;
}

// approx 1 year (60s * 60m * 24h * 365d)
#define FOREVER_NSEC 31536000000000000ull

struct timespec *
_dispatch_get_next_timer_fire(struct timespec *howsoon)
{
	// <rdar://problem/6459649>
	// kevent(2) does not allow large timeouts, so we use a long timeout
	// instead (approximately 1 year).
	dispatch_source_refs_t dr = NULL;
	unsigned int timer;
	uint64_t now, delta_tmp, delta = UINT64_MAX;
	
	for (timer = 0; timer < DISPATCH_TIMER_COUNT; timer++) {
		// Timers are kept in order, first one will fire next
		dr = TAILQ_FIRST(&_dispatch_kevent_timer[timer].dk_sources);
		if (!dr || !ds_timer(dr).target) {
			// Empty list or disabled timer
			continue;
		}
		now = _dispatch_source_timer_now(dr);
		if (ds_timer(dr).target <= now) {
			howsoon->tv_sec = 0;
			howsoon->tv_nsec = 0;
			return howsoon;
		}
		// the subtraction cannot go negative because the previous "if"
		// verified that the target is greater than now.
		delta_tmp = ds_timer(dr).target - now;
		if (!(ds_timer(dr).flags & DISPATCH_TIMER_WALL_CLOCK)) {
			delta_tmp = _dispatch_time_mach2nano(delta_tmp);
		}
		if (delta_tmp < delta) {
			delta = delta_tmp;
		}
	}
	if (slowpath(delta > FOREVER_NSEC)) {
		return NULL;
	} else {
		howsoon->tv_sec = (time_t)(delta / NSEC_PER_SEC);
		howsoon->tv_nsec = (long)(delta % NSEC_PER_SEC);
	}
	return howsoon;
}

struct dispatch_set_timer_params {
	dispatch_source_t ds;
	uintptr_t ident;
	struct dispatch_timer_source_s values;
};

static void
_dispatch_source_set_timer3(void *context)
{
	// Called on the _dispatch_mgr_q
	struct dispatch_set_timer_params *params = context;
	dispatch_source_t ds = params->ds;
	ds->ds_ident_hack = params->ident;
	ds_timer(ds->ds_refs) = params->values;
	// Clear any pending data that might have accumulated on
	// older timer params <rdar://problem/8574886>
	ds->ds_pending_data = 0;
	_dispatch_timer_list_update(ds);
	dispatch_resume(ds);
	dispatch_release(ds);
	free(params);
}

static void
_dispatch_source_set_timer2(void *context)
{
	// Called on the source queue
	struct dispatch_set_timer_params *params = context;
	dispatch_suspend(params->ds);
	dispatch_barrier_async_f(&_dispatch_mgr_q, params,
							 _dispatch_source_set_timer3);
}

void
dispatch_source_set_timer(dispatch_source_t ds,
						  dispatch_time_t start,
						  uint64_t interval,
						  uint64_t leeway)
{
	if (slowpath(!ds->ds_is_timer)) {
		DISPATCH_CLIENT_CRASH("Attempt to set timer on a non-timer source");
	}
	
	struct dispatch_set_timer_params *params;
	
	// we use zero internally to mean disabled
	if (interval == 0) {
		interval = 1;
	} else if ((int64_t)interval < 0) {
		// 6866347 - make sure nanoseconds won't overflow
		interval = INT64_MAX;
	}
	if ((int64_t)leeway < 0) {
		leeway = INT64_MAX;
	}
	
	if (start == DISPATCH_TIME_NOW) {
		start = _dispatch_absolute_time();
	} else if (start == DISPATCH_TIME_FOREVER) {
		start = INT64_MAX;
	}
	
	while (!(params = calloc(1ul, sizeof(struct dispatch_set_timer_params)))) {
		sleep(1);
	}
	
	params->ds = ds;
	params->values.flags = ds_timer(ds->ds_refs).flags;
	
	if ((int64_t)start < 0) {
		// wall clock
		params->ident = DISPATCH_TIMER_INDEX_WALL;
		params->values.target = -((int64_t)start);
		params->values.interval = interval;
		params->values.leeway = leeway;
		params->values.flags |= DISPATCH_TIMER_WALL_CLOCK;
	} else {
		// absolute clock
		params->ident = DISPATCH_TIMER_INDEX_MACH;
		params->values.target = start;
		params->values.interval = _dispatch_time_nano2mach(interval);
		
		// rdar://problem/7287561 interval must be at least one in
		// in order to avoid later division by zero when calculating
		// the missed interval count. (NOTE: the wall clock's
		// interval is already "fixed" to be 1 or more)
		if (params->values.interval < 1) {
			params->values.interval = 1;
		}
		
		params->values.leeway = _dispatch_time_nano2mach(leeway);
		params->values.flags &= ~DISPATCH_TIMER_WALL_CLOCK;
	}
	// Suspend the source so that it doesn't fire with pending changes
	// The use of suspend/resume requires the external retain/release
	dispatch_retain(ds);
	dispatch_barrier_async_f((dispatch_queue_t)ds, params,
							 _dispatch_source_set_timer2);
}
