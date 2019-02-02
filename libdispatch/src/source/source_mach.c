#include "source_mach.h"

#pragma mark -
#pragma mark dispatch_mach

#if HAVE_MACH

#if DISPATCH_DEBUG && DISPATCH_MACHPORT_DEBUG
#define _dispatch_debug_machport(name) \
dispatch_debug_machport((name), __func__)
#else
#define _dispatch_debug_machport(name)
#endif

// Flags for all notifications that are registered/unregistered when a
// send-possible notification is requested/delivered
#define _DISPATCH_MACH_SP_FLAGS (DISPATCH_MACH_SEND_POSSIBLE| \
DISPATCH_MACH_SEND_DEAD|DISPATCH_MACH_SEND_DELETED)

#define _DISPATCH_IS_POWER_OF_TWO(v) (!(v & (v - 1)) && v)
#define _DISPATCH_HASH(x, y) (_DISPATCH_IS_POWER_OF_TWO(y) ? \
(MACH_PORT_INDEX(x) & ((y) - 1)) : (MACH_PORT_INDEX(x) % (y)))

#define _DISPATCH_MACHPORT_HASH_SIZE 32
#define _DISPATCH_MACHPORT_HASH(x) \
_DISPATCH_HASH((x), _DISPATCH_MACHPORT_HASH_SIZE)

static dispatch_source_t _dispatch_mach_notify_source;
static mach_port_t _dispatch_port_set;//发送端口
static mach_port_t _dispatch_event_port;//接收端口

static kern_return_t _dispatch_mach_notify_u端口pdate(dispatch_kevent_t dk,
												  uint32_t new_flags, uint32_t del_flags, uint32_t mask,
												  mach_msg_id_t notify_msgid, mach_port_mscount_t notify_sync);

static void
_dispatch_port_set_init(void *context DISPATCH_UNUSED)
{
	struct kevent kev = {
		.filter = EVFILT_MACHPORT,
		.flags = EV_ADD,
	};
	kern_return_t kr;
	
	//send
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET,
							&_dispatch_port_set);
	DISPATCH_VERIFY_MIG(kr);
	if (kr) {
		_dispatch_bug_mach_client("_dispatch_port_set_init: mach_port_allocate() failed", kr);
		DISPATCH_CLIENT_CRASH("mach_port_allocate() failed: cannot create port set");
	}
	
	//receive
//	Function - Create caller-specified type of port right.
//	The mach_port_allocate function creates a new right in the specified task.
//	The new right's name is returned in name.
//
//	Ports that are allocated via this call do not support the full set of Mach port semantics;
//	in particular, the kernel will not provide no-more-senders notification service requests on such ports.
//	Any attempt to request no-more-senders notification service will generate an error.
//	Use the mach_port_allocate_full interface to allocate ports that support the full set of Mach port semantics.
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
							&_dispatch_event_port);
	DISPATCH_VERIFY_MIG(kr);
	if (kr) {
		_dispatch_bug_mach_client("_dispatch_port_set_init: mach_port_allocate() failed", kr);
		DISPATCH_CLIENT_CRASH("mach_port_allocate() failed: cannot create receive right");
	}
	
	
	kr = mach_port_move_member(mach_task_self(), _dispatch_event_port,
							   _dispatch_port_set);//读写合并
	DISPATCH_VERIFY_MIG(kr);
	if (kr) {
		_dispatch_bug_mach_client("_dispatch_port_set_init: mach_port_move_member() failed", kr);
		DISPATCH_CLIENT_CRASH("mach_port_move_member() failed");
	}
	
	kev.ident = _dispatch_port_set;
	_dispatch_update_kq(&kev);
}

static mach_port_t
_dispatch_get_port_set(void)
{
	static dispatch_once_t pred;
	dispatch_once_f(&pred, NULL, _dispatch_port_set_init);
	return _dispatch_port_set;
}


static kern_return_t
_dispatch_kevent_machport_enable(dispatch_kevent_t dk)
{
	mach_port_t mp = (mach_port_t)dk->dk_kevent.ident;
	kern_return_t kr;
	
	_dispatch_debug_machport(mp);
	kr = mach_port_move_member(mach_task_self(), mp, _dispatch_get_port_set());
	if (slowpath(kr)) {
		DISPATCH_VERIFY_MIG(kr);
		switch (kr) {
			case KERN_INVALID_NAME:
#if DISPATCH_DEBUG
				_dispatch_log("Corruption: Mach receive right 0x%x destroyed "
							  "prematurely", mp);
#endif
				break;
			case KERN_INVALID_RIGHT:
				_dispatch_bug_mach_client("_dispatch_kevent_machport_enable: "
										  "mach_port_move_member() failed ", kr);
				break;
			default:
				(void)dispatch_assume_zero(kr);
				break;
		}
	}
	return kr;
}
static void
_dispatch_kevent_machport_disable(dispatch_kevent_t dk)
{
	mach_port_t mp = (mach_port_t)dk->dk_kevent.ident;
	kern_return_t kr;
	
	_dispatch_debug_machport(mp);
	kr = mach_port_move_member(mach_task_self(), mp, 0);
	
//	kern_return_t   mach_port_move_member
//	(ipc_space_t                               task,
//	 mach_port_name_t                        member,//receive right.
//	 mach_port_name_t                         after);// port set
//	task
//	[in task send right] The task holding the port set and receive right.
//	member
//	[in scalar] The task's name for the receive right.
//	after
//	[in scalar] The task's name for the port set.
//	DESCRIPTION
//	The mach_port_move_member function moves a receive right into a port set. If the receive right is already a member of any other port sets, it is removed from those sets first. If the port set is MACH_PORT_NULL, then the receive right is not put into a port set, but removed from all its current port sets.
//	NOTES
//	This interface is machine word length specific because of the port name parameter.
	
	if (slowpath(kr)) {
		DISPATCH_VERIFY_MIG(kr);
		switch (kr) {
			case KERN_INVALID_RIGHT:
			case KERN_INVALID_NAME:
#if DISPATCH_DEBUG
				_dispatch_log("Corruption: Mach receive right 0x%x destroyed "
							  "prematurely", mp);
#endif
				break;
			default:
				(void)dispatch_assume_zero(kr);
				break;
		}
	}
}

kern_return_t
_dispatch_kevent_machport_resume(dispatch_kevent_t dk, uint32_t new_flags,
								 uint32_t del_flags)
{
	kern_return_t kr_recv = 0, kr_sp = 0;
	
	dispatch_assert_zero(new_flags & del_flags);
	if (new_flags & DISPATCH_MACH_RECV_MESSAGE) {
		kr_recv = _dispatch_kevent_machport_enable(dk);
	} else if (del_flags & DISPATCH_MACH_RECV_MESSAGE) {
		_dispatch_kevent_machport_disable(dk);
	}
	if ((new_flags & _DISPATCH_MACH_SP_FLAGS) ||
		(del_flags & _DISPATCH_MACH_SP_FLAGS)) {
		// Requesting a (delayed) non-sync send-possible notification
		// registers for both immediate dead-name notification and delayed-arm
		// send-possible notification for the port.
		// The send-possible notification is armed when a mach_msg() with the
		// the MACH_SEND_NOTIFY to the port times out.
		// If send-possible is unavailable, fall back to immediate dead-name
		// registration rdar://problem/2527840&9008724
		kr_sp = _dispatch_mach_notify_update(dk, new_flags, del_flags,
											 _DISPATCH_MACH_SP_FLAGS, MACH_NOTIFY_SEND_POSSIBLE,
											 MACH_NOTIFY_SEND_POSSIBLE == MACH_NOTIFY_DEAD_NAME ? 1 : 0);
	}
	return (kr_recv ? kr_recv : kr_sp);
}

void
_dispatch_drain_mach_messages(struct kevent *ke)
{
	mach_port_t name = (mach_port_name_t)ke->data;
	dispatch_source_refs_t dri;
	dispatch_kevent_t dk;
	struct kevent kev;
	
	if (!dispatch_assume(name)) {
		return;
	}
	_dispatch_debug_machport(name);
	dk = _dispatch_kevent_find(name, EVFILT_MACHPORT);
	if (!dispatch_assume(dk)) {
		return;
	}
	_dispatch_kevent_machport_disable(dk); // emulate EV_DISPATCH
	
	EV_SET(&kev, name,
		   EVFILT_MACHPORT,
		   EV_ADD|EV_ENABLE|EV_DISPATCH,
		   DISPATCH_MACH_RECV_MESSAGE,
		   0, dk);
	
	TAILQ_FOREACH(dri, &dk->dk_sources, dr_list) {
		_dispatch_source_merge_kevent(_dispatch_source_from_refs(dri), &kev);
	}
}

static inline void
_dispatch_mach_notify_merge(mach_port_t name, uint32_t flag, uint32_t unreg,
							bool final)
{
	dispatch_source_refs_t dri;
	dispatch_kevent_t dk;
	struct kevent kev;
	
	dk = _dispatch_kevent_find(name, EVFILT_MACHPORT);
	if (!dk) {
		return;
	}
	
	// Update notification registration state.
	dk->dk_kevent.data &= ~unreg;
	if (!final) {
		// Re-register for notification before delivery
		_dispatch_kevent_resume(dk, flag, 0);
	}
	
	EV_SET(&kev, name, EVFILT_MACHPORT, EV_ADD|EV_ENABLE, flag, 0, dk);
	
	TAILQ_FOREACH(dri, &dk->dk_sources, dr_list) {
		_dispatch_source_merge_kevent(_dispatch_source_from_refs(dri), &kev);
		if (final) {
			// this can never happen again
			// this must happen after the merge
			// this may be racy in the future, but we don't provide a 'setter'
			// API for the mask yet
			_dispatch_source_from_refs(dri)->ds_pending_data_mask &= ~unreg;
		}
	}
	
	if (final) {
		// no more sources have these flags
		dk->dk_kevent.fflags &= ~unreg;
	}
}

static kern_return_t
_dispatch_mach_notify_update(dispatch_kevent_t dk, uint32_t new_flags,
							 uint32_t del_flags, uint32_t mask, mach_msg_id_t notify_msgid,
							 mach_port_mscount_t notify_sync)
{
	mach_port_t previous, port = (mach_port_t)dk->dk_kevent.ident;
	typeof(dk->dk_kevent.data) prev = dk->dk_kevent.data;
	kern_return_t kr, krr = 0;
	
	// Update notification registration state.
	dk->dk_kevent.data |= (new_flags | dk->dk_kevent.fflags) & mask;
	dk->dk_kevent.data &= ~(del_flags & mask);
	
	_dispatch_debug_machport(port);
	if ((dk->dk_kevent.data & mask) && !(prev & mask)) {
		previous = MACH_PORT_NULL;
		krr = mach_port_request_notification(mach_task_self(), port,
											 notify_msgid, notify_sync, _dispatch_event_port,
											 MACH_MSG_TYPE_MAKE_SEND_ONCE, &previous);
		DISPATCH_VERIFY_MIG(krr);
		
		switch(krr) {
			case KERN_INVALID_NAME:
			case KERN_INVALID_RIGHT:
				// Supress errors & clear registration state
				dk->dk_kevent.data &= ~mask;
				break;
			default:
				// Else, we dont expect any errors from mach. Log any errors
				if (dispatch_assume_zero(krr)) {
					// log the error & clear registration state
					dk->dk_kevent.data &= ~mask;
				} else if (dispatch_assume_zero(previous)) {
					// Another subsystem has beat libdispatch to requesting the
					// specified Mach notification on this port. We should
					// technically cache the previous port and message it when the
					// kernel messages our port. Or we can just say screw those
					// subsystems and deallocate the previous port.
					// They should adopt libdispatch :-P
					kr = mach_port_deallocate(mach_task_self(), previous);
					DISPATCH_VERIFY_MIG(kr);
					(void)dispatch_assume_zero(kr);
					previous = MACH_PORT_NULL;
				}
		}
	} else if (!(dk->dk_kevent.data & mask) && (prev & mask)) {
		previous = MACH_PORT_NULL;
		kr = mach_port_request_notification(mach_task_self(), port,
											notify_msgid, notify_sync, MACH_PORT_NULL,
											MACH_MSG_TYPE_MOVE_SEND_ONCE, &previous);
		DISPATCH_VERIFY_MIG(kr);
		
		switch (kr) {
			case KERN_INVALID_NAME:
			case KERN_INVALID_RIGHT:
			case KERN_INVALID_ARGUMENT:
				break;
			default:
				if (dispatch_assume_zero(kr)) {
					// log the error
				}
		}
	} else {
		return 0;
	}
	if (slowpath(previous)) {
		// the kernel has not consumed the send-once right yet
		(void)dispatch_assume_zero(
								   _dispatch_send_consume_send_once_right(previous));
	}
	return krr;
}

static void
_dispatch_mach_notify_source2(void *context)
{
	dispatch_source_t ds = context;
	size_t maxsz = MAX(sizeof(union
							  __RequestUnion___dispatch_send_libdispatch_internal_protocol_subsystem),
					   sizeof(union
							  __ReplyUnion___dispatch_libdispatch_internal_protocol_subsystem));
	
	dispatch_mig_server(ds, maxsz, libdispatch_internal_protocol_server);
}

void
_dispatch_mach_notify_source_init(void *context DISPATCH_UNUSED)
{
	_dispatch_get_port_set();
	
	_dispatch_mach_notify_source = dispatch_source_create(
														  DISPATCH_SOURCE_TYPE_MACH_RECV, _dispatch_event_port, 0,
														  &_dispatch_mgr_q);
	dispatch_assert(_dispatch_mach_notify_source);
	dispatch_set_context(_dispatch_mach_notify_source,
						 _dispatch_mach_notify_source);
	dispatch_source_set_event_handler_f(_dispatch_mach_notify_source,
										_dispatch_mach_notify_source2);
	dispatch_resume(_dispatch_mach_notify_source);
}

kern_return_t
_dispatch_mach_notify_port_deleted(mach_port_t notify DISPATCH_UNUSED,
								   mach_port_name_t name)
{
#if DISPATCH_DEBUG
	_dispatch_log("Corruption: Mach send/send-once/dead-name right 0x%x "
				  "deleted prematurely", name);
#endif
	
	_dispatch_debug_machport(name);
	_dispatch_mach_notify_merge(name, DISPATCH_MACH_SEND_DELETED,
								_DISPATCH_MACH_SP_FLAGS, true);
	
	return KERN_SUCCESS;
}

kern_return_t
_dispatch_mach_notify_dead_name(mach_port_t notify DISPATCH_UNUSED,
								mach_port_name_t name)
{
	kern_return_t kr;
	
#if DISPATCH_DEBUG
	_dispatch_log("machport[0x%08x]: dead-name notification: %s",
				  name, __func__);
#endif
	_dispatch_debug_machport(name);
	_dispatch_mach_notify_merge(name, DISPATCH_MACH_SEND_DEAD,
								_DISPATCH_MACH_SP_FLAGS, true);
	
	// the act of receiving a dead name notification allocates a dead-name
	// right that must be deallocated
	kr = mach_port_deallocate(mach_task_self(), name);
	DISPATCH_VERIFY_MIG(kr);
	//(void)dispatch_assume_zero(kr);
	
	return KERN_SUCCESS;
}

kern_return_t
_dispatch_mach_notify_send_possible(mach_port_t notify DISPATCH_UNUSED,
									mach_port_name_t name)
{
#if DISPATCH_DEBUG
	_dispatch_log("machport[0x%08x]: send-possible notification: %s",
				  name, __func__);
#endif
	_dispatch_debug_machport(name);
	_dispatch_mach_notify_merge(name, DISPATCH_MACH_SEND_POSSIBLE,
								_DISPATCH_MACH_SP_FLAGS, false);
	
	return KERN_SUCCESS;
}


//mach interface generator
mach_msg_return_t
dispatch_mig_server(dispatch_source_t ds, size_t maxmsgsz,
					dispatch_mig_callback_t callback)
{
	mach_msg_options_t options = MACH_RCV_MSG | MACH_RCV_TIMEOUT
	| MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_CTX)
	| MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0);
	mach_msg_options_t tmp_options;
	mig_reply_error_t *bufTemp, *bufRequest, *bufReply;
	mach_msg_return_t kr = 0;
	unsigned int cnt = 1000; // do not stall out serial queues
	int demux_success;
	bool received = false;
	size_t rcv_size = maxmsgsz + MAX_TRAILER_SIZE;
	
	// XXX FIXME -- allocate these elsewhere
	bufRequest = alloca(rcv_size);
	bufReply = alloca(rcv_size);
	bufReply->Head.msgh_size = 0; // make CLANG happy
	bufRequest->RetCode = 0;
	
#if DISPATCH_DEBUG
	options |= MACH_RCV_LARGE; // rdar://problem/8422992
#endif
	tmp_options = options;
	// XXX FIXME -- change this to not starve out the target queue
	for (;;) {
		if (DISPATCH_OBJECT_SUSPENDED(ds) || (--cnt == 0)) {
			options &= ~MACH_RCV_MSG;
			tmp_options &= ~MACH_RCV_MSG;
			
			if (!(tmp_options & MACH_SEND_MSG)) {
				break;
			}
		}
		kr = mach_msg(&bufReply->Head, tmp_options, bufReply->Head.msgh_size,
					  (mach_msg_size_t)rcv_size, (mach_port_t)ds->ds_ident_hack, 0,0);
		
		tmp_options = options;
		
		if (slowpath(kr)) {
			switch (kr) {
				case MACH_SEND_INVALID_DEST:
				case MACH_SEND_TIMED_OUT:
					if (bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) {
						mach_msg_destroy(&bufReply->Head);
					}
					break;
				case MACH_RCV_TIMED_OUT:
					// Don't return an error if a message was sent this time or
					// a message was successfully received previously
					// rdar://problems/7363620&7791738
					if(bufReply->Head.msgh_remote_port || received) {
						kr = MACH_MSG_SUCCESS;
					}
					break;
				case MACH_RCV_INVALID_NAME:
					break;
#if DISPATCH_DEBUG
				case MACH_RCV_TOO_LARGE:
					// receive messages that are too large and log their id and size
					// rdar://problem/8422992
					tmp_options &= ~MACH_RCV_LARGE;
					size_t large_size = bufReply->Head.msgh_size + MAX_TRAILER_SIZE;
					void *large_buf = malloc(large_size);
					if (large_buf) {
						rcv_size = large_size;
						bufReply = large_buf;
					}
					if (!mach_msg(&bufReply->Head, tmp_options, 0,
								  (mach_msg_size_t)rcv_size,
								  (mach_port_t)ds->ds_ident_hack, 0, 0)) {
						_dispatch_log("BUG in libdispatch client: "
									  "dispatch_mig_server received message larger than "
									  "requested size %zd: id = 0x%x, size = %d",
									  maxmsgsz, bufReply->Head.msgh_id,
									  bufReply->Head.msgh_size);
					}
					if (large_buf) {
						free(large_buf);
					}
					// fall through
#endif
				default:
					_dispatch_bug_mach_client(
											  "dispatch_mig_server: mach_msg() failed", kr);
					break;
			}
			break;
		}
		
		if (!(tmp_options & MACH_RCV_MSG)) {
			break;
		}
		received = true;
		
		bufTemp = bufRequest;
		bufRequest = bufReply;
		bufReply = bufTemp;
		
		demux_success = callback(&bufRequest->Head, &bufReply->Head);
		
		if (!demux_success) {
			// destroy the request - but not the reply port
			bufRequest->Head.msgh_remote_port = 0;
			mach_msg_destroy(&bufRequest->Head);
		} else if (!(bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
			// if MACH_MSGH_BITS_COMPLEX is _not_ set, then bufReply->RetCode
			// is present
			if (slowpath(bufReply->RetCode)) {
				if (bufReply->RetCode == MIG_NO_REPLY) {
					continue;
				}
				
				// destroy the request - but not the reply port
				bufRequest->Head.msgh_remote_port = 0;
				mach_msg_destroy(&bufRequest->Head);
			}
		}
		
		if (bufReply->Head.msgh_remote_port) {
			tmp_options |= MACH_SEND_MSG;
			if (MACH_MSGH_BITS_REMOTE(bufReply->Head.msgh_bits) !=
				MACH_MSG_TYPE_MOVE_SEND_ONCE) {
				tmp_options |= MACH_SEND_TIMEOUT;
			}
		}
	}
	
	return kr;
}

#endif /* HAVE_MACH */
