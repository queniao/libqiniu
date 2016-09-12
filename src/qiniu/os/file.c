#include "qiniu/base/errors.h"
#include "qiniu/os/file.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of file info ----

typedef struct _QN_FL_INFO
{
    qn_string fname;
    qn_fsize fsize;
} qn_file_info;

QN_API void qn_fl_info_destroy(qn_fl_info_ptr restrict fi)
{
    if (fi) {
        free(fi);
    } // fi
}

QN_API qn_fsize qn_fl_info_fsize(qn_fl_info_ptr restrict fi)
{
    return fi->fsize;
}

QN_API qn_string qn_fl_info_fname(qn_fl_info_ptr restrict fi)
{
    return fi->fname;
}

#if defined(_MSC_VER)

#else

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// ---- Definition of file depends on operating system ----

struct _QN_FILE
{
    int fd;
} qn_file;

QN_API qn_file_ptr qn_fl_open(const char * restrict fname, qn_fl_open_extra_ptr restrict extra)
{
    qn_file_ptr new_file = calloc(1, sizeof(qn_file));
    if (!new_file) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_file->fd = open(fname, 0);
    if (new_file->fd < 0) {
        qn_err_fl_set_opening_file_failed();
        free(new_file);
        return NULL;
    } // if
    return new_file;
}

QN_API qn_file_ptr qn_fl_duplicate(qn_file_ptr restrict fl)
{
    qn_file_ptr new_file = calloc(1, sizeof(qn_file));
    if (!new_file) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if
    
    new_file->fd = dup(fl->fd);
    if (new_file->fd < 0) {
        qn_err_fl_set_duplicating_file_failed();
        free(new_file);
        return NULL;
    } // if
    return new_file;
}

QN_API void qn_fl_close(qn_file_ptr restrict fl)
{
    if (fl) {
        close(fl->fd);
        free(fl);
    } // if
}

QN_API int qn_fl_read(qn_file_ptr restrict fl, char * restrict buf, int buf_size)
{
    int ret;
    ret = read(fl->fd, buf, buf_size);
    if (ret < 0) qn_err_fl_set_reading_file_failed();
    return ret;
}

QN_API qn_bool qn_fl_seek(qn_file_ptr restrict fl, qn_fsize offset)
{
    if (lseek(fl->fd, offset, SEEK_SET) < 0) {
        qn_err_fl_set_seeking_file_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_API qn_bool qn_fl_advance(qn_file_ptr restrict fl, int delta)
{
    if (lseek(fl->fd, delta, SEEK_CUR) < 0) {
        qn_err_fl_set_seeking_file_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_API qn_size qn_fl_reader_callback(void * restrict user_data, char * restrict buf, qn_size size)
{
    qn_size buf_size = size;
    qn_file_ptr fl = (qn_file_ptr) user_data;
    return qn_fl_read(fl, buf, buf_size);
}

// ---- Definition of file info depends on operating system ----

QN_API qn_fl_info_ptr qn_fl_info_stat(const char * restrict fname)
{
    struct stat st;

    qn_fl_info_ptr new_fi = calloc(1, sizeof(qn_file_info));
    if (!new_fi) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    if (stat(fname, &st) < 0) {
        qn_fl_info_destroy(new_fi);
        qn_err_fl_info_set_stating_file_info_failed();
        return NULL;
    } // if

    new_fi->fname = qn_cs_duplicate(fname);
    if (!new_fi->fname) {
        qn_fl_info_destroy(new_fi);
        return NULL;
    } // if
    new_fi->fsize = st.st_size;

    return new_fi;
}

// ---- Definition of file section depends on operating system ----

typedef struct _QN_FL_SECTION
{
    qn_file_ptr file;

    qn_fsize offset;
    qn_fsize max_size;
    qn_fsize rem_size;
} qn_fl_section;

QN_API qn_fl_section_ptr qn_fl_sec_create(qn_file_ptr restrict fl)
{
    qn_fl_section_ptr new_section = calloc(1, sizeof(qn_fl_section));
    if (!new_section) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500)
    new_section->file = fl;
#else
    new_section->file = qn_fl_duplicate(fl);
#endif

    if (!qn_fl_sec_reset(new_section, 0, 0)) {
        free(new_section);
        return NULL;
    } // if
    return new_section;
}

QN_API void qn_fl_sec_destroy(qn_fl_section_ptr restrict fs)
{
    if (fs) {
#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500)
#else
        qn_fl_close(fs->file);
#endif
        free(fs);
    } // if
}

QN_API qn_bool qn_fl_sec_reset(qn_fl_section_ptr restrict fs, qn_fsize offset, qn_fsize max_size)
{
    fs->offset = offset;
    fs->max_size = max_size;
    fs->rem_size = max_size;

#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500)
#else
    if (!qn_fl_seek(fs->file, fs->offset)) {
        return qn_false;
    } // if
#endif

    return qn_true;
}

QN_API qn_bool qn_fl_sec_read(qn_fl_section_ptr restrict fs, char * restrict buf, qn_size * restrict buf_size)
{
    ssize_t ret;
    size_t read_size;

    if (fs->rem_size == 0) {
        *buf_size = 0;
        return qn_true;
    } // if

    read_size = (*buf_size < fs->rem_size) ? *buf_size : fs->rem_size;

#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500)
    ret = pread(fs->file->fd, buf, read_size, fs->offset + (fs->max_size - fs->rem_size));
#else
    ret = read(fs->file->fd, buf, read_size); 
#endif

    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return qn_false;
    } // if
    fs->rem_size -= ret;
    *buf_size = ret;
    return qn_true;
}

QN_API qn_size qn_fl_sec_reader_callback(void * restrict user_data, char * restrict buf, qn_size size)
{
    qn_size buf_size = size;
    qn_fl_section_ptr fs = (qn_fl_section_ptr) user_data;
    if (!qn_fl_sec_read(fs, buf, &buf_size)) {
        return 0;
    } // if
    return buf_size;
}

#endif

#ifdef __cplusplus
}
#endif
