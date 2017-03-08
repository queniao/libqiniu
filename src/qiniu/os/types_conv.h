#ifndef __QN_OS_TYPES_CONV_H__
#define __QN_OS_TYPES_CONV_H__

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(QN_CFG_LARGE_FILE_SUPPORT)

#define qn_type_fsize_to_string qn_type_long_long_to_string
#define qn_type_string_to_fsize qn_type_string_to_long_long
#define qn_type_foffset_to_string qn_type_long_long_to_string
#define qn_type_string_to_foffset qn_type_string_to_long_long

#else

#define qn_type_fsize_to_string qn_type_long_to_string
#define qn_type_string_to_fsize qn_type_string_to_long
#define qn_type_foffset_to_string qn_type_long_to_string
#define qn_type_string_to_foffset qn_type_string_to_long

#endif

#include "qiniu/base/string.h"

QN_SDK extern qn_string qn_type_long_long_to_string(long long val);
QN_SDK extern qn_bool qn_type_string_to_long_long(const char * restrict str, size_t str_len, long long * restrict val);
QN_SDK extern qn_string qn_type_long_to_string(long val);
QN_SDK extern qn_bool qn_type_string_to_long(const char * restrict str, size_t str_len, long * restrict val);

#ifdef __cplusplus
}
#endif

#endif // __QN_OS_TYPES_CONV_H__

