//
//  source_debug.c
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#include "source_debug.h"


#pragma mark -
#pragma mark dispatch_source_debug

DISPATCH_NOINLINE
static const char *
_evfiltstr(short filt)
{
	switch (filt) {
#define _evfilt2(f) case (f): return #f
			_evfilt2(EVFILT_READ);
			_evfilt2(EVFILT_WRITE);
			_evfilt2(EVFILT_AIO);
			_evfilt2(EVFILT_VNODE);
			_evfilt2(EVFILT_PROC);
			_evfilt2(EVFILT_SIGNAL);
			_evfilt2(EVFILT_TIMER);
#ifdef EVFILT_VM
			_evfilt2(EVFILT_VM);
#endif
#if HAVE_MACH
			_evfilt2(EVFILT_MACHPORT);
#endif
			_evfilt2(EVFILT_FS);
			_evfilt2(EVFILT_USER);
			
			_evfilt2(DISPATCH_EVFILT_TIMER);
			_evfilt2(DISPATCH_EVFILT_CUSTOM_ADD);
			_evfilt2(DISPATCH_EVFILT_CUSTOM_OR);
		default:
			return "EVFILT_missing";
	}
}

static size_t
_dispatch_source_debug_attr(dispatch_source_t ds, char* buf, size_t bufsiz)
{
	dispatch_queue_t target = ds->do_targetq;
	return snprintf(buf, bufsiz, "target = %s[%p], pending_data = 0x%lx, "
					"pending_data_mask = 0x%lx, ",
					target ? target->dq_label : "", target,
					ds->ds_pending_data, ds->ds_pending_data_mask);
}

static size_t
_dispatch_timer_debug_attr(dispatch_source_t ds, char* buf, size_t bufsiz)
{
	dispatch_source_refs_t dr = ds->ds_refs;
	return snprintf(buf, bufsiz, "timer = { target = 0x%"PRIu64", "
					"last_fire = 0x%"PRIu64", interval = 0x%"PRIu64", flags = 0x%"PRIu64" }, ",
					ds_timer(dr).target, ds_timer(dr).last_fire, ds_timer(dr).interval,
					ds_timer(dr).flags);
}

size_t
_dispatch_source_debug(dispatch_source_t ds, char* buf, size_t bufsiz)
{
	size_t offset = 0;
	offset += snprintf(&buf[offset], bufsiz - offset, "%s[%p] = { ",
					   dx_kind(ds), ds);
	offset += _dispatch_object_debug_attr(ds, &buf[offset], bufsiz - offset);
	offset += _dispatch_source_debug_attr(ds, &buf[offset], bufsiz - offset);
	if (ds->ds_is_timer) {
		offset += _dispatch_timer_debug_attr(ds, &buf[offset], bufsiz - offset);
	}
	offset += snprintf(&buf[offset], bufsiz - offset, "filter = %s }",
					   ds->ds_dkev ? _evfiltstr(ds->ds_dkev->dk_kevent.filter) : "????");
	return offset;
}

#if DISPATCH_DEBUG
void
dispatch_debug_kevents(struct kevent* kev, size_t count, const char* str)
{
	size_t i;
	for (i = 0; i < count; ++i) {
		_dispatch_log("kevent[%lu] = { ident = %p, filter = %s, flags = 0x%x, "
					  "fflags = 0x%x, data = %p, udata = %p }: %s",
					  i, (void*)kev[i].ident, _evfiltstr(kev[i].filter), kev[i].flags,
					  kev[i].fflags, (void*)kev[i].data, (void*)kev[i].udata, str);
	}
}

static void
_dispatch_kevent_debugger2(void *context)
{
	struct sockaddr sa;
	socklen_t sa_len = sizeof(sa);
	int c, fd = (int)(long)context;
	unsigned int i;
	dispatch_kevent_t dk;
	dispatch_source_t ds;
	dispatch_source_refs_t dr;
	FILE *debug_stream;
	
	c = accept(fd, &sa, &sa_len);
	if (c == -1) {
		if (errno != EAGAIN) {
			(void)dispatch_assume_zero(errno);
		}
		return;
	}
#if 0
	int r = fcntl(c, F_SETFL, 0); // disable non-blocking IO
	if (r == -1) {
		(void)dispatch_assume_zero(errno);
	}
#endif
	debug_stream = fdopen(c, "a");
	if (!dispatch_assume(debug_stream)) {
		close(c);
		return;
	}
	
	fprintf(debug_stream, "HTTP/1.0 200 OK\r\n");
	fprintf(debug_stream, "Content-type: text/html\r\n");
	fprintf(debug_stream, "Pragma: nocache\r\n");
	fprintf(debug_stream, "\r\n");
	fprintf(debug_stream, "<html>\n");
	fprintf(debug_stream, "<head><title>PID %u</title></head>\n", getpid());
	fprintf(debug_stream, "<body>\n<ul>\n");
	
	//fprintf(debug_stream, "<tr><td>DK</td><td>DK</td><td>DK</td><td>DK</td>"
	//		"<td>DK</td><td>DK</td><td>DK</td></tr>\n");
	
	for (i = 0; i < DSL_HASH_SIZE; i++) {
		if (TAILQ_EMPTY(&_dispatch_sources[i])) {
			continue;
		}
		TAILQ_FOREACH(dk, &_dispatch_sources[i], dk_list) {
			fprintf(debug_stream, "\t<br><li>DK %p ident %lu filter %s flags "
					"0x%hx fflags 0x%x data 0x%lx udata %p\n",
					dk, (unsigned long)dk->dk_kevent.ident,
					_evfiltstr(dk->dk_kevent.filter), dk->dk_kevent.flags,
					dk->dk_kevent.fflags, (unsigned long)dk->dk_kevent.data,
					dk->dk_kevent.udata);
			fprintf(debug_stream, "\t\t<ul>\n");
			TAILQ_FOREACH(dr, &dk->dk_sources, dr_list) {
				ds = _dispatch_source_from_refs(dr);
				fprintf(debug_stream, "\t\t\t<li>DS %p refcnt 0x%x suspend "
						"0x%x data 0x%lx mask 0x%lx flags 0x%x</li>\n",
						ds, ds->do_ref_cnt + 1, ds->do_suspend_cnt,
						ds->ds_pending_data, ds->ds_pending_data_mask,
						ds->ds_atomic_flags);
				if (ds->do_suspend_cnt == DISPATCH_OBJECT_SUSPEND_LOCK) {
					dispatch_queue_t dq = ds->do_targetq;
					fprintf(debug_stream, "\t\t<br>DQ: %p refcnt 0x%x suspend "
							"0x%x label: %s\n", dq, dq->do_ref_cnt + 1,
							dq->do_suspend_cnt, dq->dq_label);
				}
			}
			fprintf(debug_stream, "\t\t</ul>\n");
			fprintf(debug_stream, "\t</li>\n");
		}
	}
	fprintf(debug_stream, "</ul>\n</body>\n</html>\n");
	fflush(debug_stream);
	fclose(debug_stream);
}

static void
_dispatch_kevent_debugger2_cancel(void *context)
{
	int ret, fd = (int)(long)context;
	
	ret = close(fd);
	if (ret != -1) {
		(void)dispatch_assume_zero(errno);
	}
}

static void
_dispatch_kevent_debugger(void *context DISPATCH_UNUSED)
{
	union {
		struct sockaddr_in sa_in;
		struct sockaddr sa;
	} sa_u = {
		.sa_in = {
			.sin_family = AF_INET,
			.sin_addr = { htonl(INADDR_LOOPBACK), },
		},
	};
	dispatch_source_t ds;
	const char *valstr;
	int val, r, fd, sock_opt = 1;
	socklen_t slen = sizeof(sa_u);
	
	if (issetugid()) {
		return;
	}
	valstr = getenv("LIBDISPATCH_DEBUGGER");
	if (!valstr) {
		return;
	}
	val = atoi(valstr);
	if (val == 2) {
		sa_u.sa_in.sin_addr.s_addr = 0;
	}
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		(void)dispatch_assume_zero(errno);
		return;
	}
	r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&sock_opt,
				   (socklen_t) sizeof sock_opt);
	if (r == -1) {
		(void)dispatch_assume_zero(errno);
		goto out_bad;
	}
#if 0
	r = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (r == -1) {
		(void)dispatch_assume_zero(errno);
		goto out_bad;
	}
#endif
	r = bind(fd, &sa_u.sa, sizeof(sa_u));
	if (r == -1) {
		(void)dispatch_assume_zero(errno);
		goto out_bad;
	}
	r = listen(fd, SOMAXCONN);
	if (r == -1) {
		(void)dispatch_assume_zero(errno);
		goto out_bad;
	}
	r = getsockname(fd, &sa_u.sa, &slen);
	if (r == -1) {
		(void)dispatch_assume_zero(errno);
		goto out_bad;
	}
	
	ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ,
								fd, 0,
								&_dispatch_mgr_q);
	if (dispatch_assume(ds)) {
		_dispatch_log("LIBDISPATCH: debug port: %hu",
					  (in_port_t)ntohs(sa_u.sa_in.sin_port));
		
		/* ownership of fd transfers to ds */
		dispatch_set_context(ds, (void *)(long)fd);
		dispatch_source_set_event_handler_f(ds, _dispatch_kevent_debugger2);
		dispatch_source_set_cancel_handler_f(ds,
											 _dispatch_kevent_debugger2_cancel);
		dispatch_resume(ds);
		
		return;
	}
out_bad:
	close(fd);
}

#if HAVE_MACH

#ifndef MACH_PORT_TYPE_SPREQUEST
#define MACH_PORT_TYPE_SPREQUEST 0x40000000
#endif

void
dispatch_debug_machport(mach_port_t name, const char* str)
{
	mach_port_type_t type;
	mach_msg_bits_t ns = 0, nr = 0, nso = 0, nd = 0;
	unsigned int dnreqs = 0, dnrsiz;
	kern_return_t kr = mach_port_type(mach_task_self(), name, &type);
	if (kr) {
		_dispatch_log("machport[0x%08x] = { error(0x%x) \"%s\" }: %s", name,
					  kr, mach_error_string(kr), str);
		return;
	}
	if (type & MACH_PORT_TYPE_SEND) {
		(void)dispatch_assume_zero(mach_port_get_refs(mach_task_self(), name,
													  MACH_PORT_RIGHT_SEND, &ns));
	}
	if (type & MACH_PORT_TYPE_SEND_ONCE) {
		(void)dispatch_assume_zero(mach_port_get_refs(mach_task_self(), name,
													  MACH_PORT_RIGHT_SEND_ONCE, &nso));
	}
	if (type & MACH_PORT_TYPE_DEAD_NAME) {
		(void)dispatch_assume_zero(mach_port_get_refs(mach_task_self(), name,
													  MACH_PORT_RIGHT_DEAD_NAME, &nd));
	}
	if (type & (MACH_PORT_TYPE_RECEIVE|MACH_PORT_TYPE_SEND|
				MACH_PORT_TYPE_SEND_ONCE)) {
		(void)dispatch_assume_zero(mach_port_dnrequest_info(mach_task_self(),
															name, &dnrsiz, &dnreqs));
	}
	if (type & MACH_PORT_TYPE_RECEIVE) {
		mach_port_status_t status = { .mps_pset = 0, };
		mach_msg_type_number_t cnt = MACH_PORT_RECEIVE_STATUS_COUNT;
		(void)dispatch_assume_zero(mach_port_get_refs(mach_task_self(), name,
													  MACH_PORT_RIGHT_RECEIVE, &nr));
		(void)dispatch_assume_zero(mach_port_get_attributes(mach_task_self(),
															name, MACH_PORT_RECEIVE_STATUS, (void*)&status, &cnt));
		_dispatch_log("machport[0x%08x] = { R(%03u) S(%03u) SO(%03u) D(%03u) "
					  "dnreqs(%03u) spreq(%s) nsreq(%s) pdreq(%s) srights(%s) "
					  "sorights(%03u) qlim(%03u) msgcount(%03u) mkscount(%03u) "
					  "seqno(%03u) }: %s", name, nr, ns, nso, nd, dnreqs,
					  type & MACH_PORT_TYPE_SPREQUEST ? "Y":"N",
					  status.mps_nsrequest ? "Y":"N", status.mps_pdrequest ? "Y":"N",
					  status.mps_srights ? "Y":"N", status.mps_sorights,
					  status.mps_qlimit, status.mps_msgcount, status.mps_mscount,
					  status.mps_seqno, str);
	} else if (type & (MACH_PORT_TYPE_SEND|MACH_PORT_TYPE_SEND_ONCE|
					   MACH_PORT_TYPE_DEAD_NAME)) {
		_dispatch_log("machport[0x%08x] = { R(%03u) S(%03u) SO(%03u) D(%03u) "
					  "dnreqs(%03u) spreq(%s) }: %s", name, nr, ns, nso, nd, dnreqs,
					  type & MACH_PORT_TYPE_SPREQUEST ? "Y":"N", str);
	} else {
		_dispatch_log("machport[0x%08x] = { type(0x%08x) }: %s", name, type,
					  str);
	}
}

#endif // HAVE_MACH

#endif // DISPATCH_DEBUG
