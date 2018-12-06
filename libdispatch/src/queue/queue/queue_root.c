#include "queue_root.h"

//void func(dispatch_queue_t queue, dispatch_block_t block)
//{
//	if (dispatch_get_current_queue() == queue) {
//		block();
//	}else{
//		dispatch_sync(queue, block);
//	}
//}
//- (void)deadLockFunc
//{
//	dispatch_queue_t queueA = dispatch_queue_create("com.yiyaaixuexi.queueA", NULL);
//	dispatch_queue_t queueB = dispatch_queue_create("com.yiyaaixuexi.queueB", NULL);
//	dispatch_sync(queueA, ^{
//		dispatch_sync(queueB, ^{
//			dispatch_block_t block = ^{
//				//do something
//			};
//			func(queueA, block);
//		});
//	});
//}
//(lldb) po queue
//<OS_dispatch_queue_serial: com.yiyaaixuexi.queueA[0x282ebbd00] = { xref = 4, ref = 1, sref = 1, target = com.apple.root.default-qos.overcommit[0x10527cf00], width = 0x1, state = 0x0060002000000300, draining on 0x303, in-barrier}>
//
//(lldb) po dispatch_get_current_queue()
//<OS_dispatch_queue_serial: com.yiyaaixuexi.queueB[0x282ebb300] = { xref = 2, ref = 1, sref = 1, target = com.apple.root.default-qos.overcommit[0x10527cf00], width = 0x1, state = 0x0060002000000300, draining on 0x303, in-barrier}>

//Recommended for debugging and logging purposes only:
//The code must not make any assumptions about the queue returned,
//unless it is one of the global queues or a queue the code has itself created.

//The code must not assume that synchronous execution onto a queue is safe from deadlock
//if that queue is not the one returned by  dispatch_get_current_queue().
//
//该方法早在iOS 6.0就已被废弃，仅推荐用于调试和日志记录，不能依赖函数返回值进行逻辑判断。具体为什么被废弃，文档并没有详细说明，我们可以从GCD源码找到些线索，代码在libdispatch源码可以下载。
dispatch_queue_t
dispatch_get_current_queue(void)
{
	return _dispatch_queue_get_current() ?: _dispatch_get_root_queue(0, true);
}
DISPATCH_ALWAYS_INLINE
static inline dispatch_queue_t
_dispatch_queue_get_current(void)
{
	return (dispatch_queue_t)_dispatch_thread_getspecific(dispatch_queue_key);
}


dispatch_queue_t
dispatch_get_global_queue(long priority, unsigned long flags)
{
	if (flags & ~DISPATCH_QUEUE_OVERCOMMIT) {
		return NULL;
	}
	return _dispatch_get_root_queue(priority,
								  flags & DISPATCH_QUEUE_OVERCOMMIT);
}

DISPATCH_ALWAYS_INLINE DISPATCH_CONST
static inline dispatch_queue_t
_dispatch_get_root_queue(long priority, bool overcommit)
{
	if (overcommit)
		switch (priority) {
				
		case DISPATCH_QUEUE_PRIORITY_BACKGROUND:
#if !DISPATCH_NO_BG_PRIORITY
			return &_dispatch_root_queues[
					DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_OVERCOMMIT_PRIORITY];
#endif
		case DISPATCH_QUEUE_PRIORITY_LOW:
			return &_dispatch_root_queues[
										  DISPATCH_ROOT_QUEUE_IDX_LOW_OVERCOMMIT_PRIORITY];
		case DISPATCH_QUEUE_PRIORITY_DEFAULT:
			return &_dispatch_root_queues[
										  DISPATCH_ROOT_QUEUE_IDX_DEFAULT_OVERCOMMIT_PRIORITY];
		case DISPATCH_QUEUE_PRIORITY_HIGH:
			return &_dispatch_root_queues[
										  DISPATCH_ROOT_QUEUE_IDX_HIGH_OVERCOMMIT_PRIORITY];
	}
	
	switch (priority) {
		case DISPATCH_QUEUE_PRIORITY_BACKGROUND:
#if !DISPATCH_NO_BG_PRIORITY
			return &_dispatch_root_queues[
								DISPATCH_ROOT_QUEUE_IDX_BACKGROUND_PRIORITY];
#endif
		case DISPATCH_QUEUE_PRIORITY_LOW:
			return &_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_LOW_PRIORITY];
		case DISPATCH_QUEUE_PRIORITY_DEFAULT:
			return &_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_DEFAULT_PRIORITY];
		case DISPATCH_QUEUE_PRIORITY_HIGH:
			return &_dispatch_root_queues[DISPATCH_ROOT_QUEUE_IDX_HIGH_PRIORITY];
		default:
			return NULL;
	}
}


