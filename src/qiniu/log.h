#ifndef __QN_LOG_H__
#define __QN_LOG_H__

#include <stdarg.h>

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"
#include "qiniu/base/io.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _QN_LOG_LEVEL
{
    QN_LOG_TRACE = -1,
    QN_LOG_DEBUG = 0,
    QN_LOG_INFO = 1,
    QN_LOG_WARN = 2,
    QN_LOG_ERROR = 3,
    QN_LOG_FATAL = 4
} qn_log_level;

QN_SDK extern qn_log_level qn_log_get_level(void);
QN_SDK extern void qn_log_set_level(qn_log_level lvl);

QN_SDK extern void qn_log_set_writer(qn_io_writer_itf restrict wrt);

QN_SDK extern void qn_log_output_va(qn_log_level lvl, const char * restrict file, int line, const char * restrict fmt, va_list ap);

static inline void qn_log_output(qn_log_level lvl, const char * restrict file, int line, const char * restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(lvl, file, line, fmt, ap);
    va_end(ap);
}

static inline void qn_log_trace_cstr(const char * restrict file, int line, const char * restrict msg)
{
    qn_log_output(QN_LOG_TRACE, file, line, "%s", msg);
}

static inline void qn_log_trace(const char * restrict file, int line, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(QN_LOG_TRACE, file, line, fmt, ap);
    va_end(ap);
}

static inline void qn_log_debug_cstr(const char * restrict file, int line, const char * restrict msg)
{
    qn_log_output(QN_LOG_DEBUG, file, line, "%s", msg);
}

static inline void qn_log_debug(const char * restrict file, int line, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(QN_LOG_DEBUG, file, line, fmt, ap);
    va_end(ap);
}

static inline void qn_log_info_cstr(const char * restrict file, int line, const char * restrict msg)
{
    qn_log_output(QN_LOG_INFO, file, line, "%s", msg);
}

static inline void qn_log_info(const char * restrict file, int line, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(QN_LOG_INFO, file, line, fmt, ap);
    va_end(ap);
}

static inline void qn_log_warn_cstr(const char * restrict file, int line, const char * restrict msg)
{
    qn_log_output(QN_LOG_WARN, file, line, "%s", msg);
}

static inline void qn_log_warn(const char * restrict file, int line, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(QN_LOG_WARN, file, line, fmt, ap);
    va_end(ap);
}

static inline void qn_log_error_cstr(const char * restrict file, int line, const char * restrict msg)
{
    qn_log_output(QN_LOG_ERROR, file, line, "%s", msg);
}

static inline void qn_log_error(const char * restrict file, int line, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(QN_LOG_ERROR, file, line, fmt, ap);
    va_end(ap);
}

static inline void qn_log_fatal_cstr(const char * restrict file, int line, const char * restrict msg)
{
    qn_log_output(QN_LOG_FATAL, file, line, "%s", msg);
}

static inline void qn_log_fatal(const char * restrict file, int line, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    qn_log_output_va(QN_LOG_FATAL, file, line, fmt, ap);
    va_end(ap);
}

#ifdef __cplusplus
}
#endif

#endif // __QN_LOG_H__

