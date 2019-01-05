
#ifndef transform_h
#define transform_h

typedef dispatch_data_t (*dispatch_transform_t)(dispatch_data_t data);

struct dispatch_data_format_type_s {
	uint64_t type;
	uint64_t input_mask;
	uint64_t output_mask;
	dispatch_transform_t decode;
	dispatch_transform_t encode;
};


/*!
 * @typedef dispatch_data_format_type_t
 *
 * @abstract
 * Data formats are used to specify the input and output types of data supplied
 * to dispatch_data_create_transform.
 */
typedef const struct dispatch_data_format_type_s *dispatch_data_format_type_t;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_NONE
 * @discussion A data format denoting that the given input or output format is,
 * or should be, comprised of raw data bytes with no given encoding.
 */
#define DISPATCH_DATA_FORMAT_TYPE_NONE (&_dispatch_data_format_type_none)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_none;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_BASE32
 * @discussion A data format denoting that the given input or output format is,
 * or should be, encoded in Base32 (RFC 4648) format. On input, this format will
 * skip whitespace characters. Cannot be used in conjunction with UTF format
 * types.
 */
#define DISPATCH_DATA_FORMAT_TYPE_BASE32 (&_dispatch_data_format_type_base32)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_base32;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_BASE64
 * @discussion A data format denoting that the given input or output format is,
 * or should be, encoded in Base64 (RFC 4648) format. On input, this format will
 * skip whitespace characters. Cannot be used in conjunction with UTF format
 * types.
 */
#define DISPATCH_DATA_FORMAT_TYPE_BASE64 (&_dispatch_data_format_type_base64)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_base64;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_UTF8
 * @discussion A data format denoting that the given input or output format is,
 * or should be, encoded in UTF-8 format. Is only valid when used in conjunction
 * with other UTF format types.
 */
#define DISPATCH_DATA_FORMAT_TYPE_UTF8 (&_dispatch_data_format_type_utf8)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_utf8;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_UTF16LE
 * @discussion A data format denoting that the given input or output format is,
 * or should be, encoded in UTF-16LE format. Is only valid when used in
 * conjunction with other UTF format types.
 */
#define DISPATCH_DATA_FORMAT_TYPE_UTF16LE (&_dispatch_data_format_type_utf16le)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_utf16le;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_UTF16BE
 * @discussion A data format denoting that the given input or output format is,
 * or should be, encoded in UTF-16BE format. Is only valid when used in
 * conjunction with other UTF format types.
 */
#define DISPATCH_DATA_FORMAT_TYPE_UTF16BE (&_dispatch_data_format_type_utf16be)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_utf16be;

/*!
 * @const DISPATCH_DATA_FORMAT_TYPE_UTFANY
 * @discussion A data format denoting that dispatch_data_create_transform should
 * attempt to automatically detect the input type based on the presence of a
 * byte order mark character at the beginning of the data. In the absence of a
 * BOM, the data will be assumed to be in UTF-8 format. Only valid as an input
 * format.
 */
#define DISPATCH_DATA_FORMAT_TYPE_UTF_ANY (&_dispatch_data_format_type_utf_any)
__OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_6_0)
DISPATCH_EXPORT
const struct dispatch_data_format_type_s _dispatch_data_format_type_utf_any;




#endif /* transform_h */
