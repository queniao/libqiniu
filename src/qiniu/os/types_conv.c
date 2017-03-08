#include "qiniu/os/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_SDK qn_string qn_type_long_long_to_string(long long val)
{
    return qn_cs_sprintf("%Ld", val);
}

QN_SDK qn_bool qn_type_string_to_long_long(const char * restrict str, size_t str_len, long long * restrict val)
{
#if defined(__ISOC99_SOURCE || _BSD_SOURCE || _SVID_SOURCE)
    if (str_len == 0) {
        *val = atoll(str);
        return qn_true;
    } // if
#endif

    if (sscanf(str, "%Ld", val) == EOF) {
        // TODO: Set an appropriate error if the call fails.
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_string qn_type_long_to_string(long val)
{
    return qn_cs_sprintf("%ld", val);
}

QN_SDK qn_bool qn_type_string_to_long(const char * restrict str, size_t str_len, long * restrict val)
{
    if (str_len == 0) {
        *val = atol(str);
        return qn_true;
    } // if

    if (sscanf(str, "%ld", val) == EOF) {
        // TODO: Set an appropriate error if the call fails.
        return qn_false;
    } // if
    return val;
}

#ifdef __cplusplus
}
#endif

