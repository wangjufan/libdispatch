
#include "internal.h"
#if HAVE_MACH
#include "protocol.h"
#endif

#if __linux__
#include <sys/eventfd.h>
#define DISPATCH_LINUX_COMPAT 1
#endif

#if (!HAVE_PTHREAD_WORKQUEUES || DISPATCH_DEBUG) && \
		!defined(DISPATCH_USE_PTHREAD_POOL)
#define DISPATCH_USE_PTHREAD_POOL 1
#endif
#if DISPATCH_USE_PTHREAD_POOL && !DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
#define pthread_workqueue_t void*
#endif

#if HAVE_PTHREAD_WORKQUEUES
#define DISPATCH_WORKQ_OPTION_OVERCOMMIT WORKQ_ADDTHREADS_OPTION_OVERCOMMIT
#define DISPATCH_WORKQ_BG_PRIOQUEUE WORKQ_BG_PRIOQUEUE
#define DISPATCH_WORKQ_LOW_PRIOQUEUE WORKQ_LOW_PRIOQUEUE
#define DISPATCH_WORKQ_DEFAULT_PRIOQUEUE WORKQ_DEFAULT_PRIOQUEUE
#define DISPATCH_WORKQ_HIGH_PRIOQUEUE WORKQ_HIGH_PRIOQUEUE
#endif

static void _dispatch_cache_cleanup(void *value);
static void _dispatch_queue_cleanup(void *ctxt);

static void _dispatch_async_f_redirect(dispatch_queue_t dq,
		dispatch_continuation_t dc);

static inline void _dispatch_queue_wakeup_global2(dispatch_queue_t dq,
		unsigned int n);
static inline void _dispatch_queue_wakeup_global(dispatch_queue_t dq);
static _dispatch_thread_semaphore_t _dispatch_queue_drain(dispatch_queue_t dq);
static inline _dispatch_thread_semaphore_t
		_dispatch_queue_drain_one_barrier_sync(dispatch_queue_t dq);


#if DISPATCH_COCOA_COMPAT || DISPATCH_LINUX_COMPAT
static dispatch_queue_t _dispatch_queue_wakeup_main(void);
static void _dispatch_main_queue_drain(void);
#endif

#if DISPATCH_COCOA_COMPAT
static unsigned int _dispatch_worker_threads;
static dispatch_once_t _dispatch_main_q_port_pred;
static mach_port_t main_q_port;  //主线程 接收port

static void _dispatch_main_q_port_init(void *ctxt);
#endif

#if DISPATCH_LINUX_COMPAT
static dispatch_once_t _dispatch_main_q_eventfd_pred;
static void _dispatch_main_q_eventfd_init(void *ctxt);
static void _dispatch_eventfd_write(int fd, uint64_t value);
static uint64_t _dispatch_eventfd_read(int fd);
static int main_q_eventfd = -1;

static void _dispatch_queue_cleanup_tsd(void);
#endif

