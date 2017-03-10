#ifndef __QN_OS_FILE_H__
#define __QN_OS_FILE_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"
#include "qiniu/base/io.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of file

struct _QN_FILE;
typedef struct _QN_FILE * qn_file_ptr;

struct _QN_FL_SECTION;
typedef struct _QN_FL_SECTION * qn_fl_section_ptr;

typedef struct _QN_FL_OPEN_EXTRA
{
} qn_fl_open_extra, *qn_fl_open_extra_ptr;

QN_SDK extern qn_file_ptr qn_fl_open(const char * restrict fname, qn_fl_open_extra_ptr restrict extra);
QN_SDK extern void qn_fl_close(qn_file_ptr restrict fl);

QN_SDK extern qn_file_ptr qn_fl_duplicate(qn_file_ptr restrict fl);
QN_SDK extern qn_fl_section_ptr qn_fl_section(qn_file_ptr restrict fl, qn_foffset offset, size_t sec_size);

QN_SDK extern qn_io_reader_itf qn_fl_to_io_reader(qn_file_ptr restrict fl);

QN_SDK extern qn_string qn_fl_fname(qn_file_ptr restrict fl);
QN_SDK extern qn_fsize qn_fl_fsize(qn_file_ptr restrict fl);

QN_SDK extern ssize_t qn_fl_peek(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size);
QN_SDK extern ssize_t qn_fl_read(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size);
QN_SDK extern qn_bool qn_fl_seek(qn_file_ptr restrict fl, qn_foffset offset);
QN_SDK extern qn_bool qn_fl_advance(qn_file_ptr restrict fl, size_t delta);

QN_SDK extern size_t qn_fl_reader_callback(void * restrict user_data, char * restrict buf, size_t buf_size);

// ---- Declaration of file info ----

struct _QN_FL_INFO;
typedef struct _QN_FL_INFO * qn_fl_info_ptr;

QN_SDK extern qn_fl_info_ptr qn_fl_info_stat(const char * restrict fname);
QN_SDK extern void qn_fl_info_destroy(qn_fl_info_ptr restrict fi);

QN_SDK extern qn_fsize qn_fl_info_fsize(qn_fl_info_ptr restrict fi);
QN_SDK extern qn_string qn_fl_info_fname(qn_fl_info_ptr restrict fi);

// ---- Declaration of file section ----

QN_SDK extern qn_fl_section_ptr qn_fl_sec_create(qn_file_ptr restrict fl, qn_foffset offset, size_t sec_size);
QN_SDK extern void qn_fl_sec_destroy(qn_fl_section_ptr restrict fs);
QN_SDK extern qn_bool qn_fl_sec_reset(qn_fl_section_ptr restrict fs);

QN_SDK extern qn_fl_section_ptr qn_fl_sec_duplicate(qn_fl_section_ptr restrict fs);
QN_SDK extern qn_fl_section_ptr qn_fl_sec_section(qn_fl_section_ptr restrict fs, qn_foffset offset, size_t sec_size);

QN_SDK extern qn_io_reader_itf qn_fl_sec_to_io_reader(qn_fl_section_ptr restrict fs);

QN_SDK extern ssize_t qn_fl_sec_peek(qn_fl_section_ptr restrict fs, char * restrict buf, size_t buf_size);
QN_SDK extern ssize_t qn_fl_sec_read(qn_fl_section_ptr restrict fs, char * restrict buf, size_t buf_size);
QN_SDK extern qn_bool qn_fl_sec_seek(qn_fl_section_ptr restrict fs, qn_foffset offset);
QN_SDK extern qn_bool qn_fl_sec_advance(qn_fl_section_ptr restrict fs, size_t delta);

QN_SDK extern size_t qn_fl_sec_reader_callback(void * restrict user_data, char * restrict buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_OS_FILE_H__
