//
//  crash.h
//  libdispatch
//
//  Created by 王举范 on 2019/1/28.
//

#ifndef crash_h
#define crash_h


关联分析 与 因果分析

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


Fatal Exception: NSInvalidArgumentException
*** +[NSString stringWithUTF8String:]: NULL cString
Fatal Exception: NSInvalidArgumentException
0  CoreFoundation                 0x19e357ea0 __exceptionPreprocess
1  libobjc.A.dylib                0x19d529a40 objc_exception_throw
2  CoreFoundation                 0x19e25e674 -[NSCache init]
3  Foundation                     0x19ecd14f4 +[NSString stringWithUTF8String:]
4  TTSSpeechBundle                0x1184f2950 (Missing)
5  libsystem_pthread.dylib        0x19df732ac _pthread_body
6  libsystem_pthread.dylib        0x19df7320c _pthread_start
7  libsystem_pthread.dylib        0x19df76cf4 thread_start


#0
Crashed: WebThread
EXC_BAD_ACCESS KERN_INVALID_ADDRESS 0x0000000000000001
background
0  libGPUSupportMercury.dylib     0x19e31af28 gpus_ReturnNotPermittedKillClient
1  AGXGLDriver                    0x1a2af58d0 (Missing)
2  libGPUSupportMercury.dylib     0x19e31bf04 gpusSubmitDataBuffers
3  AGXGLDriver                    0x1a2af6e18 (Missing)
4  AGXGLDriver                    0x1a2af0388 (Missing)
5  GLEngine                       0x1a3ccaa44 glDrawElements_ACC_ES2Exec
6  WebCore                        0x18af8da80 WebCore::GraphicsContext3D::drawElements(unsigned int, int, unsigned int, long) + 64
7  WebCore                        0x18b3f53b4 WebCore::WebGLRenderingContextBase::drawElements(unsigned int, int, unsigned int, long long) + 320
8  WebCore                        0x18a4882f4 WebCore::jsWebGLRenderingContextPrototypeFunctionDrawElements(JSC::ExecState*) + 316
9  JavaScriptCore                 0x18874c804 llint_entry + 32692
10 JavaScriptCore                 0x18874bd24 llint_entry + 29908
11 JavaScriptCore                 0x18874bd24 llint_entry + 29908
12 JavaScriptCore                 0x18874bd24 llint_entry + 29908


#0
Crashed: com.apple.network.connections
SIGABRT ABORT 0x00000001a4731104
Crashed: com.apple.network.connections
0  libsystem_kernel.dylib         0x1a4731104 __pthread_kill + 8
1  libsystem_pthread.dylib        0x1a47ac0e0 pthread_kill$VARIANT$mp + 380
2  libsystem_c.dylib              0x1a4688d78 abort + 140
3  libsystem_malloc.dylib         0x1a4785768 _malloc_put + 570
4  libsystem_malloc.dylib         0x1a4785924 malloc_report + 64
5  libsystem_malloc.dylib         0x1a47782d0 free + 376
6  libdispatch.dylib              0x1a45d4484 _dispatch_client_callout + 16
7  libdispatch.dylib              0x1a457db9c _dispatch_workloop_invoke$VARIANT$mp + 2312
8  libdispatch.dylib              0x1a4584f00 _dispatch_workloop_worker_thread + 600
9  libsystem_pthread.dylib        0x1a47b60f0 _pthread_wqthread + 312
10 libsystem_pthread.dylib        0x1a47b8d00 start_wqthread + 4



#0
Crashed: com.apple.main-thread
EXC_BAD_ACCESS KERN_INVALID_ADDRESS 0x0000000000000000
Crashed: com.apple.main-thread
0  libobjc.A.dylib                0x181f88bb4 lookUpImpOrForward + 80
1  libobjc.A.dylib                0x181f93258 _objc_msgSend_uncached + 56
2  libsystem_blocks.dylib         0x182426a28 _Block_release + 144
3  libdispatch.dylib              0x1823d11c0 _dispatch_client_callout + 16
4  libdispatch.dylib              0x1823d5d6c _dispatch_main_queue_callback_4CF + 1000
5  CoreFoundation                 0x1834f5f2c __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__ + 12
6  CoreFoundation                 0x1834f3b18 __CFRunLoopRun + 1660
7  CoreFoundation                 0x183422048 CFRunLoopRunSpecific + 444
8  GraphicsServices               0x184ea5198 GSEventRunModal + 180
9  UIKit                          0x1893fc628 -[UIApplication _run] + 684
10 UIKit                          0x1893f7360 UIApplicationMain + 208
11 Trans                          0x1007a11a4 main (main.m:20)
12 (Missing)                      0x1824045b8 (Missing)

background
#0
Crashed: WebThread
EXC_BAD_ACCESS KERN_INVALID_ADDRESS 0x0000000000000001
Crashed: WebThread
0  libGPUSupportMercury.dylib     0x1e6a7ef28 gpus_ReturnNotPermittedKillClient
1  AGXGLDriver                    0x1eb2648e0 (Missing)
2  libGPUSupportMercury.dylib     0x1e6a7ff04 gpusSubmitDataBuffers
3  AGXGLDriver                    0x1eb265e28 (Missing)
4  WebCore                        0x1d36a78c8 WebCore::GraphicsContext3D::reshape(int, int) + 580
5  WebCore                        0x1d3b0876c WebCore::WebGLRenderingContextBase::initializeNewContext() + 936
6  WebCore                        0x1d3b08050 WebCore::WebGLRenderingContextBase::WebGLRenderingContextBase(WebCore::CanvasBase&, WTF::Ref<WebCore::GraphicsContext3D, WTF::DumbPtrTraits<WebCore::GraphicsContext3D> >&&, WebCore::GraphicsContext3DAttributes) + 528
7  WebCore                        0x1d3afe6f8 WebCore::WebGLRenderingContext::create(WebCore::CanvasBase&, WTF::Ref<WebCore::GraphicsContext3D, WTF::DumbPtrTraits<WebCore::GraphicsContext3D> >&&, WebCore::GraphicsContext3DAttributes) + 84
8  WebCore                        0x1d3b074ac WebCore::WebGLRenderingContextBase::create(WebCore::CanvasBase&, WebCore::GraphicsContext3DAttributes&, WTF::String const&) + 1436
9  WebCore                        0x1d32390fc WebCore::HTMLCanvasElement::getContext(JSC::ExecState&, WTF::String const&, WTF::Vector<JSC::Strong<JSC::Unknown>, 0ul, WTF::CrashOnOverflow, 16ul>&&) + 960
10 WebCore                        0x1d27eef34 WebCore::jsHTMLCanvasElementPrototypeFunctionGetContext(JSC::ExecState*) + 416
11 JavaScriptCore                 0x1d0e68814 llint_entry + 32692
12 JavaScriptCore                 0x1d0e67d9c llint_entry + 30012
13 JavaScriptCore                 0x1d0e67d9c llint_entry + 30012
14 JavaScriptCore                 0x1d0e67d9c llint_entry + 30012
15 JavaScriptCore                 0x1d0e67d9c llint_entry + 30012
16 JavaScriptCore                 0x1d0e67d9c llint_entry + 30012
17 JavaScriptCore                 0x1d0e67d9c llint_entry + 30012
18 JavaScriptCore                 0x1d0e60664 vmEntryToJavaScript + 308
19 JavaScriptCore                 0x1d14ce434 JSC::Interpreter::executeProgram(JSC::SourceCode const&, JSC::ExecState*, JSC::JSObject*) + 9620
20 JavaScriptCore                 0x1d16a9f54 JSC::evaluate(JSC::ExecState*, JSC::SourceCode const&, JSC::JSValue, WTF::NakedPtr<JSC::Exception>&) + 320
21 WebCore                        0x1d2e8ad94 WebCore::ScriptController::evaluateInWorld(WebCore::ScriptSourceCode const&, WebCore::DOMWrapperWorld&, WebCore::ExceptionDetails*) + 328
22 WebCore                        0x1d311ac3c WebCore::ScriptElement::executeClassicScript(WebCore::ScriptSourceCode const&) + 624
23 WebCore                        0x1d30e4b20 WebCore::LoadableClassicScript::execute(WebCore::ScriptElement&) + 152
24 WebCore                        0x1d311aec4 WebCore::ScriptElement::executeScriptAndDispatchEvent(WebCore::LoadableScript&) + 216
25 WebCore                        0x1d3120b84 WebCore::ScriptRunner::timerFired() + 516
26 WebCore                        0x1d35ac02c WebCore::ThreadTimers::sharedTimerFiredInternal() + 352
27 WebCore                        0x1d35f0a54 WebCore::timerFired(__CFRunLoopTimer*, void*) + 28
28 CoreFoundation                 0x1c9aa9828 __CFRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__ + 28
29 CoreFoundation                 0x1c9aa9558 __CFRunLoopDoTimer + 864
30 CoreFoundation                 0x1c9aa8d8c __CFRunLoopDoTimers + 248
31 CoreFoundation                 0x1c9aa3c68 __CFRunLoopRun + 1880
32 CoreFoundation                 0x1c9aa31f0 CFRunLoopRunSpecific + 436
33 WebCore                        0x1d2912eec RunWebThread(void*) + 592
34 libsystem_pthread.dylib        0x1c973425c _pthread_body + 128
35 libsystem_pthread.dylib        0x1c97341bc _pthread_start + 48
36 libsystem_pthread.dylib        0x1c9737cf4 thread_start + 4

#0
Crashed: com.apple.avfoundation.videodataoutput.bufferqueue
EXC_BREAKPOINT 0x000000019bdf2bb0
CVPixelBuffer::finalize()
Raw Text




#endif /* crash_h */
