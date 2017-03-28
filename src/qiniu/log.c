#include <errno.h>

#include "qiniu/base/errors.h"
#include "qiniu/log.h"

#ifdef __cplusplus
extern "C"
{
#endif

static qn_log_level qn_log_threshold = QN_LOG_INFO;
static char qn_log_buf[1024 * 8];
static qn_io_writer_itf qn_log_writer = NULL;
static const char * qn_log_level_tags[] = {
    "[TRACE]",
    "[DEBUG]",
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[FATAL]"
};

QN_SDK qn_log_level qn_log_get_level(void)
{
    return qn_log_threshold;
}

QN_SDK void qn_log_set_level(qn_log_level lvl)
{
    qn_log_threshold = lvl;
}

QN_SDK void qn_log_set_writer(qn_io_writer_itf restrict wrt)
{
    qn_log_writer = wrt;
}

QN_SDK void qn_log_output_va(qn_log_level lvl, const char * restrict file, int line, const char * restrict fmt, va_list ap)
{
    qn_ssize ret = 0;
    qn_size size = 0;
    const char * fname = NULL;

    if (lvl < qn_log_threshold) return;
    if (! qn_log_writer) return;

    fname = posix_strrchr(file, '/'); // TODO: Be compatible to Windows pathes.
    if (! fname) fname = file;

    ret = qn_cs_snprintf(qn_log_buf, sizeof(qn_log_buf), "%s %s:%d", (&qn_log_level_tags[QN_LOG_DEBUG] + lvl), fname, line);
    if (ret <= 0) {
        qn_err_3rdp_set_glibc_error_occurred(errno);
        return;
    } // if
    size += ret;

    ret = qn_cs_vsnprintf(qn_log_buf + size, sizeof(qn_log_buf) - size, fmt, ap);
    if (ret <= 0) {
        qn_err_3rdp_set_glibc_error_occurred(errno);
        return;
    } // if
    size += ret;

    if (sizeof(qn_log_buf) - size < 1) {
        qn_err_set_out_of_buffer();
        return;
    } // if
    qn_log_buf[size++] = '\n'; // TODO: Be compatible to Windows line break.

    qn_io_wrt_write(qn_log_writer, qn_log_buf, size);
}

#ifdef __cplusplus
}
#endif

