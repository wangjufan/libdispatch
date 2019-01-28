//
//  crash.h
//  libdispatch
//
//  Created by 王举范 on 2019/1/28.
//

#ifndef crash_h
#define crash_h

Crashed: com.apple.main-thread
0  libobjc.A.dylib                0x1df736b00 objc_object::release() + 16
1  libsystem_blocks.dylib         0x1dffe2a44 _Block_release + 152
2  libdispatch.dylib              0x1dff86484 _dispatch_client_callout + 16
3  libdispatch.dylib              0x1dff65b34 _dispatch_main_queue_callback_4CF$VARIANT$armv81 + 1012
4  CoreFoundation                 0x1e04dcdf4 __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__ + 12
5  CoreFoundation                 0x1e04d7cbc __CFRunLoopRun + 1964
6  CoreFoundation                 0x1e04d71f0 CFRunLoopRunSpecific + 436
7  GraphicsServices               0x1e2750584 GSEventRunModal + 100
8  UIKitCore                      0x20d862d40 UIApplicationMain + 212
9  Trans                          0x100b111a4 main (main.m:20)
10 libdyld.dylib                  0x1dff96bb4 start + 4

/////////////////////////////
Fatal Exception: NSGenericException
0  CoreFoundation                 0x22efb0ec4 __exceptionPreprocess
1  libobjc.A.dylib                0x22e181a50 objc_exception_throw
2  CoreFoundation                 0x22efb075c -[__NSSingleObjectEnumerator initWithObject:]
3  Foundation                     0x22f99059c -[NSArray(NSKeyValueCoding) valueForKey:]
4  Trans                          0x10407367c -[NEHTTPUploader requestWithModel:sessionID:]
5  Trans                          0x104073248 -[NEHTTPUploader dataRequestWithModel:cookie:sessionID:isLast:]
6  Trans                          0x104072c20 -[NEHTTPUploader uploadAudioData:isLast:cookie:session:completionHandler:]
7  Trans                          0x104074c34 -[NEUploadRequestManager sendAudioData:isLast:isRetring:]
8  Foundation                     0x22fa408bc __NSBLOCKOPERATION_IS_CALLING_OUT_TO_A_BLOCK__
9  Foundation                     0x22f948ab8 -[NSBlockOperation main]
10 Foundation                     0x22f947f8c -[__NSOperationInternal _start:]
11 Foundation                     0x22fa42790 __NSOQSchedule_f
12 libdispatch.dylib              0x22e9e96c8 _dispatch_call_block_and_release
13 libdispatch.dylib              0x22e9ea484 _dispatch_client_callout
14 libdispatch.dylib              0x22e98d82c _dispatch_continuation_pop$VARIANT$mp
15 libdispatch.dylib              0x22e98cef4 _dispatch_async_redirect_invoke
16 libdispatch.dylib              0x22e999a18 _dispatch_root_queue_drain
17 libdispatch.dylib              0x22e99a2c0 _dispatch_worker_thread2
18 libsystem_pthread.dylib        0x22ebcd17c _pthread_wqthread
19 libsystem_pthread.dylib        0x22ebcfcec start_wqthread

/////////////////////////////////////

Crashed: Thread
0  Trans                          0x1034c096c __kmp_join_barrier(int) + 4048536
1  Trans                          0x1034af8c8 __kmp_launch_thread + 3978740
2  Trans                          0x1034af8c8 __kmp_launch_thread + 3978740
3  Trans                          0x1034c88ac __kmp_launch_worker(void*) + 4081112
4  libsystem_pthread.dylib        0x22c4452fc _pthread_body + 128
5  libsystem_pthread.dylib        0x22c44525c _pthread_start + 48
6  libsystem_pthread.dylib        0x22c448d08 thread_start + 4

//////opencv
Crashed: com.twitter.crashlytics.ios.exception
0  Trans                          0x10156de08 CLSProcessRecordAllThreads + 336736
1  Trans                          0x10156e2c8 CLSProcessRecordAllThreads + 337952
2  Trans                          0x10155db18 CLSHandler + 270448
3  Trans                          0x10156c418 __CLSExceptionRecord_block_invoke + 330096
4  libdispatch.dylib              0x183372a14 _dispatch_client_callout + 16
5  libdispatch.dylib              0x18337b618 _dispatch_queue_barrier_sync_invoke_and_complete + 56
6  Trans                          0x10156be88 CLSExceptionRecord + 328672
7  Trans                          0x10156b974 CLSTerminateHandler() + 327372
8  libc++abi.dylib                0x182c2c54c std::__terminate(void (*)()) + 16
9  libc++abi.dylib                0x182c2c5b8 std::terminate() + 60
10 libobjc.A.dylib                0x182c3c76c _destroyAltHandlerList + 10
11 libdispatch.dylib              0x183372a28 _dispatch_client_callout + 36
12 libdispatch.dylib              0x18337a200 _dispatch_block_invoke_direct$VARIANT$mp + 288
13 libdispatch.dylib              0x183372a14 _dispatch_client_callout + 16
14 libdispatch.dylib              0x18337a200 _dispatch_block_invoke_direct$VARIANT$mp + 288
15 libdispatch.dylib              0x18337a0ac dispatch_block_perform$VARIANT$mp + 104
16 Foundation                     0x1843cb878 __NSOQSchedule_f + 376
17 libdispatch.dylib              0x183372a14 _dispatch_client_callout + 16
18 libdispatch.dylib              0x18337af08 _dispatch_continuation_pop$VARIANT$mp + 428
19 libdispatch.dylib              0x18337980c _dispatch_async_redirect_invoke$VARIANT$mp + 604
20 libdispatch.dylib              0x18337fcf4 _dispatch_root_queue_drain + 600
21 libdispatch.dylib              0x18337fa38 _dispatch_worker_thread3 + 120
22 libsystem_pthread.dylib        0x18361b06c _pthread_wqthread + 1268
23 libsystem_pthread.dylib        0x18361ab6c start_wqthread + 4



Crashed: com.apple.main-thread
0  libobjc.A.dylib                0x18141091c objc_msgSend + 28
1  libsystem_blocks.dylib         0x181bd4a5c _Block_release + 152
2  libdispatch.dylib              0x181b34ae4 _dispatch_client_callout + 16
3  libdispatch.dylib              0x181b75d60 _dispatch_main_queue_callback_4CF$VARIANT$armv81 + 964
4  CoreFoundation                 0x1821eb070 __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__ + 12
5  CoreFoundation                 0x1821e8bc8 __CFRunLoopRun + 2272
6  CoreFoundation                 0x182108da8 CFRunLoopRunSpecific + 552
7  GraphicsServices               0x1840eb020 GSEventRunModal + 100
8  UIKit                          0x18c0e978c UIApplicationMain + 236
9  Trans                          0x1049811a4 main (main.m:20)
10 libdyld.dylib                  0x181b99fc0 start + 4



#0
Crashed: com.apple.main-thread
EXC_BAD_ACCESS KERN_INVALID_ADDRESS 0x00000b0a09080700
Crashed: com.apple.main-thread
0  libobjc.A.dylib                0x183251990 lookUpImpOrForward + 92
1  libobjc.A.dylib                0x18325cc38 _objc_msgSend_uncached + 56
2  libobjc.A.dylib                0x183265698 objc_object::sidetable_release(bool) + 324
3  libsystem_blocks.dylib         0x183a20a5c _Block_release + 152
4  libdispatch.dylib              0x183980a60 _dispatch_client_callout + 16
5  libdispatch.dylib              0x18398d65c _dispatch_main_queue_callback_4CF$VARIANT$mp + 1012
6  CoreFoundation                 0x184037070 __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__ + 12
7  CoreFoundation                 0x184034bc8 __CFRunLoopRun + 2272
8  CoreFoundation                 0x183f54da8 CFRunLoopRunSpecific + 552
9  GraphicsServices               0x185f3a020 GSEventRunModal + 100
10 UIKit                          0x18df74758 UIApplicationMain + 236
11 Trans                          0x1017c3eac main (main.m:20)
12 libdyld.dylib                  0x1839e5fc0 start + 4

Crashed: StatStore
0  libsqlite3.dylib               0x1c15727fc sqlite3_get_table + 1772
1  libsqlite3.dylib               0x1c154fac4 sqlite3_log + 57420
2  libsqlite3.dylib               0x1c15460f0 sqlite3_log + 18040
3  libsqlite3.dylib               0x1c1525368 sqlite3_exec + 14640
4  libsqlite3.dylib               0x1c152359c sqlite3_exec + 7012
5  libsqlite3.dylib               0x1c1522744 sqlite3_exec + 3340
6  libsqlite3.dylib               0x1c1521b54 sqlite3_exec + 284
7  Trans                          0x103933a74 execute_query + 1800096
8  Trans                          0x1039353ec -[WXOMTAStore loadCachedEvents:] + 1806616
9  Trans                          0x103936308 __32-[WXOMTAStore sendCachedEvents:]_block_invoke + 1810484
10 libdispatch.dylib              0x1c0b556c8 _dispatch_call_block_and_release + 24
11 libdispatch.dylib              0x1c0b56484 _dispatch_client_callout + 16
12 libdispatch.dylib              0x1c0b30fa0 _dispatch_lane_serial_drain$VARIANT$armv81 + 548
13 libdispatch.dylib              0x1c0b31ae4 _dispatch_lane_invoke$VARIANT$armv81 + 412
14 libdispatch.dylib              0x1c0b39f04 _dispatch_workloop_worker_thread + 584
15 libsystem_pthread.dylib        0x1c0d390dc _pthread_wqthread + 312
16 libsystem_pthread.dylib        0x1c0d3bcec start_wqthread + 4

#endif /* crash_h */
