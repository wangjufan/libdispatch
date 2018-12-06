//
//  worker_thread.h
//  libdispatch
//
//  Created by 王举范 on 2018/12/3.
//

#ifndef worker_thread_h
#define worker_thread_h

#include <stdio.h>


//static void called inner
//_dispatch_worker_thread4(dispatch_queue_t dq);

#if DISPATCH_USE_LEGACY_WORKQUEUE_FALLBACK
static void _dispatch_worker_thread3(void *context);
#endif

#if HAVE_PTHREAD_WORKQUEUE_SETDISPATCH_NP
static void _dispatch_worker_thread2(int priority, int options, void *context);
#endif

#if DISPATCH_USE_PTHREAD_POOL
static void *_dispatch_worker_thread(void *context);
static int _dispatch_pthread_sigmask(int how, sigset_t *set, sigset_t *oset);
#endif

#endif /* worker_thread_h */
