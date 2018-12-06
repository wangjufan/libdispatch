//
//  source_kevent.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/4.
//

#ifndef source_kevent_h
#define source_kevent_h

#include <stdio.h>

//https://wiki.netbsd.org/tutorials/kqueue_tutorial/
//Kqueue provides a standard API for applications to
//register their interest in various events/conditions
//and have the notifications for these delivered in an efficient way.
//It was designed to be scalable, flexible, reliable and correct.

//pair
//A kevent is identified by an <ident, filter> pair. The ident might be a descriptor (file, socket, stream), a process ID or a signal number, depending on what we want to monitor. The filter identifies the kernel filter used to process the respective event. There are some pre-defined system filters, such as EVFILT_READ or EVFILT_WRITE, that are triggered when data exists for read or write operation is possible respectively.
//
//If for instance we want to be notified when there's data available for reading in a socket, we have to specify a kevent in the form <sckfd, EVFILT_READ>, where sckfd is the file descriptor associated with the socket. If we would like to monitor the activity of a process, we would need a <pid, EVFILT_PROC> tuple. Keep in mind there can be only one kevent with the same <ident, filter> in our kqueue.


//struct kevent {
//	uintptr_t	ident;		/* identifier for this event */
//	int16_t		filter;		/* filter for event */
//	uint16_t	flags;		/* general flags */
//	uint32_t	fflags;		/* filter-specific flags */
//	intptr_t	data;		/* filter-specific data */
//	void		*udata;		/* opaque user data identifier */
//};
//struct kevent64_s {
//	uint64_t	ident;		/* identifier for this event */
//	int16_t		filter;		/* filter for event */
//	uint16_t	flags;		/* general flags */
//	uint32_t	fflags;		/* filter-specific flags */
//	int64_t		data;		/* filter-specific data */
//	uint64_t	udata;		/* opaque user data identifier */
//	uint64_t	ext[2];		/* filter-specific extensions */
//};


struct dispatch_kevent_s {
	TAILQ_ENTRY(dispatch_kevent_s) dk_list;
	TAILQ_HEAD(, dispatch_source_refs_s) dk_sources;
	struct kevent dk_kevent;
};
//#define	TAILQ_HEAD(name, type)						\
//struct name {								\
//struct type *tqh_first;	/* first element */			\
//struct type **tqh_last;	/* addr of last next element */		\
//TRACEBUF							\
//}
typedef struct dispatch_kevent_s *dispatch_kevent_t;


static void _dispatch_source_merge_kevent(dispatch_source_t ds,
										  const struct kevent *ke);

static void _dispatch_kevent_register(dispatch_source_t ds);
static void _dispatch_kevent_unregister(dispatch_source_t ds);

static bool _dispatch_kevent_resume(dispatch_kevent_t dk,
									uint32_t new_flags,
									uint32_t del_flags);

#endif /* source_kevent_h */
