/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

/*
 * IMPORTANT: This header file describes INTERNAL interfaces to libdispatch
 * which are subject to change in future releases of Mac OS X. Any applications
 * relying on these interfaces WILL break.
 */

#ifndef __DISPATCH_DATA_PRIVATE__
#define __DISPATCH_DATA_PRIVATE__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include <dispatch/base.h> // for HeaderDoc
#endif

__BEGIN_DECLS

#ifdef __BLOCKS__

/*!
 * @const DISPATCH_DATA_DESTRUCTOR_NONE
 * @discussion The destructor for dispatch data objects that require no
 * management. This can be used to allow a data object to efficiently
 * encapsulate data that should not be copied or freed by the system.
 */
#define DISPATCH_DATA_DESTRUCTOR_NONE (_dispatch_data_destructor_none)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT const dispatch_block_t _dispatch_data_destructor_none;

#if HAVE_MACH
/*!
 * @const DISPATCH_DATA_DESTRUCTOR_VM_DEALLOCATE
 * @discussion The destructor for dispatch data objects that have been created
 * from buffers that require deallocation using vm_deallocate.
 */
#define DISPATCH_DATA_DESTRUCTOR_VM_DEALLOCATE \
		(_dispatch_data_destructor_vm_deallocate)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT const dispatch_block_t _dispatch_data_destructor_vm_deallocate;
#endif

/*!
 * @function dispatch_data_create_transform
 * Returns a new dispatch data object after transforming the given data object
 * from the supplied format, into the given output format.
 *
 * @param data
 * The data object representing the region(s) of memory to transform.
 * @param input_type
 * Flags specifying the input format of the source dispatch_data_t
 *
 * @param output_type
 * Flags specifying the expected output format of the resulting transfomation.
 *
 * @result
 * A newly created dispatch data object, dispatch_data_empty if no has been
 * produced, or NULL if an error occurred.
 */

__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_RETURNS_RETAINED
DISPATCH_WARN_RESULT DISPATCH_NOTHROW
dispatch_data_t
dispatch_data_create_with_transform(dispatch_data_t data,
	dispatch_data_format_type_t input_type,
	dispatch_data_format_type_t output_type);

#endif /* __BLOCKS__ */

__END_DECLS

#endif // __DISPATCH_DATA_PRIVATE__
