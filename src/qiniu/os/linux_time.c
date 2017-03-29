#include <time.h>
#include <errno.h>

#include "qiniu/os/time.h"
#include "qiniu/base/errors.h"

QN_SDK qn_time qn_tm_time(void)
{
    return time(NULL);
}

QN_SDK qn_string qn_tm_to_string(qn_time tm)
{
    qn_string ret = NULL;
    if (sizeof(qn_time) == 4) {
        ret = qn_cs_sprintf("%u", tm);
    } else {
        ret = qn_cs_sprintf("%llu", tm);
    } // if
    return ret;
}

QN_SDK qn_ssize qn_tm_format_timestamp(qn_time tm, char * restrict buf, qn_size buf_size)
{
    struct tm brk_tm;

    if (buf == NULL && buf_size == 0) return 20;

    if (! localtime_r(&tm, &brk_tm)) {
        qn_err_3rdp_set_glibc_error_occurred(errno);
        return -1;
    } // if

    return qn_cs_snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d", brk_tm.tm_year + 1900, brk_tm.tm_mon + 1,brk_tm.tm_mday, brk_tm.tm_hour, brk_tm.tm_min, brk_tm.tm_sec);
}
