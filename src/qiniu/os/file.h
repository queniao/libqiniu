#ifndef __QN_OS_FILE_H__
#define __QN_OS_FILE_H__

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of file info ----

#if defined(_MSC_VER)

#else

#include <sys/types.h>

typedef off_t qn_fsize;

#endif

// ---- Declaration of file

struct _QN_FILE;
typedef struct _QN_FILE * qn_file_ptr;

typedef struct _QN_FL_OPEN_EXTRA
{
} qn_fl_open_extra, *qn_fl_open_extra_ptr;

QN_API extern qn_file_ptr qn_fl_open(const char * restrict fname, qn_fl_open_extra_ptr restrict extra);
QN_API extern qn_file_ptr qn_fl_duplicate(qn_file_ptr restrict fl);
QN_API extern void qn_fl_close(qn_file_ptr restrict fl);

QN_API extern int qn_fl_read(qn_file_ptr restrict fl, char * restrict buf, int buf_size);
QN_API extern qn_bool qn_fl_seek(qn_file_ptr restrict fl, qn_fsize offset);
QN_API extern qn_bool qn_fl_advance(qn_file_ptr restrict fl, int delta);

QN_API extern qn_size qn_fl_reader_callback(void * restrict user_data, char * restrict buf, qn_size size);

// ----

struct _QN_FL_INFO;
typedef struct _QN_FL_INFO * qn_fl_info_ptr;

QN_API extern qn_fl_info_ptr qn_fl_info_stat(const char * restrict fname);
QN_API extern void qn_fl_info_destroy(qn_fl_info_ptr restrict fi);

QN_API extern qn_fsize qn_fl_info_fsize(qn_fl_info_ptr restrict fi);
QN_API extern qn_string qn_fl_info_fname(qn_fl_info_ptr restrict fi);

// ----

struct _QN_FL_SECTION;
typedef struct _QN_FL_SECTION * qn_fl_section_ptr;

QN_API extern qn_fl_section_ptr qn_fl_sec_create(qn_file_ptr restrict fl);
QN_API extern void qn_fl_sec_destroy(qn_fl_section_ptr restrict fs);
QN_API extern qn_bool qn_fl_sec_reset(qn_fl_section_ptr restrict fs, qn_fsize offset, qn_fsize max_size);

QN_API extern qn_bool qn_fl_sec_read(qn_fl_section_ptr restrict fs, char * restrict buf, qn_size * restrict buf_size);

QN_API extern qn_size qn_fl_sec_reader_callback(void * restrict user_data, char * restrict buf, qn_size size);

#ifdef __cplusplus
}
#endif

#endif // __QN_OS_FILE_H__
