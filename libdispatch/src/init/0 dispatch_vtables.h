#ifndef dispatch_vtables_h
#define dispatch_vtables_h

#pragma mark -
#pragma mark dispatch_vtables

DISPATCH_VTABLE_INSTANCE(semaphore,
						 .do_type = DISPATCH_SEMAPHORE_TYPE,
						 .do_kind = "semaphore",
						 .do_dispose = _dispatch_semaphore_dispose,
						 .do_debug = _dispatch_semaphore_debug,
						 );

DISPATCH_VTABLE_INSTANCE(group,
						 .do_type = DISPATCH_GROUP_TYPE,
						 .do_kind = "group",
						 .do_dispose = _dispatch_semaphore_dispose,
						 .do_debug = _dispatch_semaphore_debug,
						 );

DISPATCH_VTABLE_INSTANCE(queue,
						 .do_type = DISPATCH_QUEUE_TYPE,
						 .do_kind = "queue",
						 .do_dispose = _dispatch_queue_dispose,
						 .do_invoke = NULL,
						 .do_probe = (void *)dummy_function_r0,
						 .do_debug = dispatch_queue_debug,
						 );

DISPATCH_VTABLE_SUBCLASS_INSTANCE(queue_root, queue,
								  .do_type = DISPATCH_QUEUE_GLOBAL_TYPE,
								  .do_kind = "global-queue",
								  .do_debug = dispatch_queue_debug,
								  .do_probe = _dispatch_queue_probe_root,
								  );

DISPATCH_VTABLE_SUBCLASS_INSTANCE(queue_mgr, queue,
								  .do_type = DISPATCH_QUEUE_MGR_TYPE,
								  .do_kind = "mgr-queue",
								  .do_invoke = _dispatch_mgr_thread,
								  .do_debug = dispatch_queue_debug,
								  .do_probe = _dispatch_mgr_wakeup,
								  );

DISPATCH_VTABLE_INSTANCE(queue_specific_queue,
						 .do_type = DISPATCH_QUEUE_SPECIFIC_TYPE,
						 .do_kind = "queue-context",
						 .do_dispose = _dispatch_queue_specific_queue_dispose,
						 .do_invoke = NULL,
						 .do_probe = (void *)dummy_function_r0,
						 .do_debug = (void *)dispatch_queue_debug,
						 );

DISPATCH_VTABLE_INSTANCE(queue_attr,
						 .do_type = DISPATCH_QUEUE_ATTR_TYPE,
						 .do_kind = "queue-attr",
						 );

DISPATCH_VTABLE_INSTANCE(source,
						 .do_type = DISPATCH_SOURCE_KEVENT_TYPE,
						 .do_kind = "kevent-source",
						 .do_invoke = _dispatch_source_invoke,
						 .do_dispose = _dispatch_source_dispose,
						 .do_probe = _dispatch_source_probe,
						 .do_debug = _dispatch_source_debug,
						 );

#if WITH_DISPATCH_IO
DISPATCH_VTABLE_INSTANCE(data,
						 .do_type = DISPATCH_DATA_TYPE,
						 .do_kind = "data",
						 .do_dispose = _dispatch_data_dispose,
						 .do_invoke = NULL,
						 .do_probe = (void *)dummy_function_r0,
						 .do_debug = _dispatch_data_debug,
						 );

DISPATCH_VTABLE_INSTANCE(io,
						 .do_type = DISPATCH_IO_TYPE,
						 .do_kind = "channel",
						 .do_dispose = _dispatch_io_dispose,
						 .do_invoke = NULL,
						 .do_probe = (void *)dummy_function_r0,
						 .do_debug = (void *)dummy_function_r0,
						 );

DISPATCH_VTABLE_INSTANCE(operation,
						 .do_type = DISPATCH_OPERATION_TYPE,
						 .do_kind = "operation",
						 .do_dispose = _dispatch_operation_dispose,
						 .do_invoke = NULL,
						 .do_probe = (void *)dummy_function_r0,
						 .do_debug = (void *)dummy_function_r0,
						 );

DISPATCH_VTABLE_INSTANCE(disk,
						 .do_type = DISPATCH_DISK_TYPE,
						 .do_kind = "disk",
						 .do_dispose = _dispatch_disk_dispose,
						 .do_invoke = NULL,
						 .do_probe = (void *)dummy_function_r0,
						 .do_debug = (void *)dummy_function_r0,
						 );
#endif  // WITH_DISPATCH_IO

void
_dispatch_vtable_init(void)
{
#if USE_OBJC
	// ObjC classes and dispatch vtables are co-located via linker order and
	// alias files, verify correct layout during initialization rdar://10640168
#define DISPATCH_OBJC_CLASS(name) \
DISPATCH_CONCAT(OBJC_CLASS_$_,DISPATCH_CLASS(name))
	extern void *DISPATCH_OBJC_CLASS(semaphore);
	dispatch_assert((char*)DISPATCH_VTABLE(semaphore) -
					(char*)&DISPATCH_OBJC_CLASS(semaphore) == 0);

    dispatch_assert((char*)&DISPATCH_CONCAT(_,
           DISPATCH_CLASS(semaphore_vtable))
					- (char*)&DISPATCH_OBJC_CLASS(semaphore) ==
					sizeof(_os_object_class_s));
#endif
}

#endif /* dispatch_vtables_h */
