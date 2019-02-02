
#ifndef __DISPATCH_SEMAPHORE__
#define __DISPATCH_SEMAPHORE__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include <dispatch/base.h> // for HeaderDoc
#endif

/*!
 * @typedef dispatch_semaphore_t
 *
 * @abstract
 * A counting semaphore.
 */
DISPATCH_DECL(dispatch_semaphore);

__BEGIN_DECLS

/*!
 * @function dispatch_semaphore_create
 *
 * @abstract
 * Creates new counting semaphore with an initial value.
 *
 * @discussion
 * Passing zero for the value is useful for when two threads need to reconcile
 * the completion of a particular event.
 
 * Passing a value greather than zero is useful
 * for managing a finite pool of resources, where the pool size is equal to the value.
 *
 * @param value
 * The starting value for the semaphore. Passing a value less than zero will
 * cause NULL to be returned.
 *
 * @result
 * The newly created semaphore, or NULL on failure.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_EXPORT DISPATCH_MALLOC DISPATCH_RETURNS_RETAINED DISPATCH_WARN_RESULT
DISPATCH_NOTHROW
dispatch_semaphore_t
dispatch_semaphore_create(long value);

///////////////////////////////////////////////////
void _dispatch_semaphore_dispose(dispatch_object_t dou);
size_t _dispatch_semaphore_debug(dispatch_object_t dou, char *buf,
								 size_t bufsiz);

__END_DECLS

#endif /* __DISPATCH_SEMAPHORE__ */
