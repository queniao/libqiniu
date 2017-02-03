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
} qn_file_info_st;

QN_API void qn_fl_info_destroy(qn_fl_info_ptr restrict fi)
{
    if (fi) {
        qn_str_destroy(fi->fname);
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

static qn_fl_info_ptr qn_fl_info_duplicate(qn_fl_info_ptr restrict fi)
{
    qn_fl_info_ptr new_fi = calloc(1, sizeof(qn_file_info_st));
    if (! new_fi) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_fi->fname = qn_cs_duplicate(fi->fname);
    if (! new_fi->fname) {
        free(new_fi);
        return NULL;
    } // if

    new_fi->fsize = fi->fsize;
    return new_fi;
}

#if defined(_MSC_VER)

#else

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// ---- Definition of file depends on operating system ----

struct _QN_FILE
{
    qn_io_reader_ptr rdr_vtbl;
    qn_fl_info_ptr fi;
    int fd;
} qn_file_st;

static inline qn_file_ptr qn_fl_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_file_ptr)( ( (char *) itf ) - (char *)( &((qn_file_ptr)0)->rdr_vtbl ) );
}

static void qn_fl_close_fn(qn_io_reader_itf restrict itf)
{
    qn_fl_close(qn_fl_from_io_reader(itf));
}

static ssize_t qn_fl_peek_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_peek(qn_fl_from_io_reader(itf), buf, buf_size);
}

static ssize_t qn_fl_read_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_read(qn_fl_from_io_reader(itf), buf, buf_size);
}

static qn_bool qn_fl_seek_fn(qn_io_reader_itf restrict itf, qn_fsize offset)
{
    return qn_fl_seek(qn_fl_from_io_reader(itf), offset);
}

static qn_bool qn_fl_advance_fn(qn_io_reader_itf restrict itf, size_t delta)
{
    return qn_fl_advance(qn_fl_from_io_reader(itf), delta);
}

static qn_string qn_fl_name_fn(qn_io_reader_itf restrict itf)
{
    return qn_fl_fname(qn_fl_from_io_reader(itf));
}

static qn_fsize qn_fl_size_fn(qn_io_reader_itf restrict itf)
{
    return qn_fl_fsize(qn_fl_from_io_reader(itf));
}

static qn_io_reader_itf qn_fl_duplicate_fn(qn_io_reader_itf restrict itf)
{
    qn_file_ptr new_file = qn_fl_duplicate(qn_fl_from_io_reader(itf));
    if (!new_file) return NULL;
    return qn_fl_to_io_reader(new_file);
}

static qn_io_reader_itf qn_fl_section_fn(qn_io_reader_itf restrict itf, qn_fsize offset, size_t sec_size)
{
    qn_fl_section_ptr new_file = qn_fl_section(qn_fl_from_io_reader(itf), offset, sec_size);
    if (!new_file) return NULL;
    return qn_fl_sec_to_io_reader(new_file);
}

static qn_io_reader_st qn_fl_rdr_vtable = {
    &qn_fl_close_fn,
    &qn_fl_peek_fn,
    &qn_fl_read_fn,
    &qn_fl_seek_fn,
    &qn_fl_advance_fn,
    &qn_fl_duplicate_fn,
    &qn_fl_section_fn,
    &qn_fl_name_fn,
    &qn_fl_size_fn
};

QN_API qn_file_ptr qn_fl_open(const char * restrict fname, qn_fl_open_extra_ptr restrict extra)
{
    qn_file_ptr new_file = calloc(1, sizeof(qn_file_st));
    if (!new_file) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_file->fd = open(fname, 0);
    if (new_file->fd < 0) {
        free(new_file);
        qn_err_fl_set_opening_file_failed();
        return NULL;
    } // if

    new_file->fi = qn_fl_info_stat(fname);
    if (! new_file->fi) {
        free(new_file);
        return NULL;
    } // if

    new_file->rdr_vtbl = &qn_fl_rdr_vtable;
    return new_file;
}

QN_API qn_file_ptr qn_fl_duplicate(qn_file_ptr restrict fl)
{
    qn_file_ptr new_file = calloc(1, sizeof(qn_file_st));
    if (!new_file) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    
    new_file->fd = dup(fl->fd);
    if (new_file->fd < 0) {
        free(new_file);
        qn_err_fl_set_duplicating_file_failed();
        return NULL;
    } // if

    new_file->fi = qn_fl_info_duplicate(fl->fi);
    if (! new_file->fi) {
        free(new_file);
        return NULL;
    } // if

    new_file->rdr_vtbl = &qn_fl_rdr_vtable;
    return new_file;
}

QN_API void qn_fl_close(qn_file_ptr restrict fl)
{
    if (fl) {
        qn_fl_info_destroy(fl->fi);
        close(fl->fd);
        free(fl);
    } // if
}

QN_API qn_io_reader_itf qn_fl_to_io_reader(qn_file_ptr restrict fl)
{
    return &fl->rdr_vtbl;
}

QN_API qn_string qn_fl_fname(qn_file_ptr restrict fl)
{
    return fl->fi->fname;
}

QN_API qn_fsize qn_fl_fsize(qn_file_ptr restrict fl)
{
    return fl->fi->fsize;
}

QN_API ssize_t qn_fl_peek(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size)
{
    ssize_t ret = qn_fl_read(fl, buf, buf_size);
    if (ret >= 0 && !qn_fl_advance(fl, -ret)) return QN_IO_RDR_READING_FAILED;
    return ret;
}

QN_API ssize_t qn_fl_read(qn_file_ptr restrict fl, char * restrict buf, size_t buf_size)
{
    ssize_t ret = read(fl->fd, buf, buf_size);
    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if
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

QN_API qn_bool qn_fl_advance(qn_file_ptr restrict fl, size_t delta)
{
    if (lseek(fl->fd, delta, SEEK_CUR) < 0) {
        qn_err_fl_set_seeking_file_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_API qn_fl_section_ptr qn_fl_section(qn_file_ptr restrict fl, qn_fsize offset, size_t sec_size)
{
    qn_fl_section_ptr new_sec = qn_fl_sec_create(fl, offset, sec_size);
    if (!new_sec) return NULL;
    return new_sec;
}

QN_API size_t qn_fl_reader_callback(void * restrict user_data, char * restrict buf, size_t buf_size)
{
    qn_file_ptr fl = (qn_file_ptr) user_data;
    return qn_fl_read(fl, buf, buf_size);
}

// ---- Definition of file info depends on operating system ----

QN_API qn_fl_info_ptr qn_fl_info_stat(const char * restrict fname)
{
    struct stat st;

    qn_fl_info_ptr new_fi = calloc(1, sizeof(qn_file_info_st));
    if (!new_fi) {
        qn_err_set_out_of_memory();
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

#if !defined(QN_OS_FILE_SHARED_FOR_SECTION) && (defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500))
#define QN_OS_FILE_SHARED_FOR_SECTION
#endif

typedef struct _QN_FL_SECTION
{
    qn_io_reader_ptr rdr_vtbl;
    qn_file_ptr file;
    qn_fsize offset;
    size_t sec_size;
    size_t rem_size;
} qn_fl_section_st;

static inline qn_fl_section_ptr qn_fl_sec_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_fl_section_ptr)( ( (char *) itf ) - (char *)( &((qn_fl_section_ptr)0)->rdr_vtbl ) );
}

static void qn_fl_sec_close_fn(qn_io_reader_itf restrict itf)
{
    qn_fl_sec_destroy(qn_fl_sec_from_io_reader(itf));
}

static ssize_t qn_fl_sec_peek_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_sec_peek(qn_fl_sec_from_io_reader(itf), buf, buf_size);
}

static ssize_t qn_fl_sec_read_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_fl_sec_read(qn_fl_sec_from_io_reader(itf), buf, buf_size);
}

static qn_bool qn_fl_sec_seek_fn(qn_io_reader_itf restrict itf, qn_fsize offset)
{
    return qn_fl_sec_seek(qn_fl_sec_from_io_reader(itf), offset);
}

static qn_bool qn_fl_sec_advance_fn(qn_io_reader_itf restrict itf, size_t delta)
{
    return qn_fl_sec_advance(qn_fl_sec_from_io_reader(itf), delta);
}

static qn_io_reader_itf qn_fl_sec_duplicate_fn(qn_io_reader_itf restrict itf)
{
    qn_fl_section_ptr new_section = qn_fl_sec_duplicate(qn_fl_sec_from_io_reader(itf));
    if (!new_section) return NULL;
    return qn_fl_sec_to_io_reader(new_section);
}

static qn_io_reader_itf qn_fl_sec_section_fn(qn_io_reader_itf restrict itf, qn_fsize offset, size_t sec_size)
{
    qn_fl_section_ptr new_section = qn_fl_sec_section(qn_fl_sec_from_io_reader(itf), offset, sec_size);
    if (!new_section) return NULL;
    return qn_fl_sec_to_io_reader(new_section);
}

static qn_io_reader_st qn_fl_sec_rdr_vtable = {
    &qn_fl_sec_close_fn,
    &qn_fl_sec_peek_fn,
    &qn_fl_sec_read_fn,
    &qn_fl_sec_seek_fn,
    &qn_fl_sec_advance_fn,
    &qn_fl_sec_duplicate_fn,
    &qn_fl_sec_section_fn
};

QN_API qn_fl_section_ptr qn_fl_sec_create(qn_file_ptr restrict fl, qn_fsize offset, size_t sec_size)
{
    qn_fl_section_ptr new_section = calloc(1, sizeof(qn_fl_section_st));
    if (!new_section) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
    new_section->file = qn_fl_duplicate(fl);
#else
    new_section->file = fl;
#endif

    new_section->offset = offset;
    new_section->sec_size = sec_size;
    new_section->rem_size = sec_size;

    // TODO: Check if the offset is larger than the file's size.

    if (! qn_fl_sec_seek(new_section, offset)) {
        qn_fl_sec_destroy(new_section);
        return NULL;
    } // if

    new_section->rdr_vtbl = &qn_fl_sec_rdr_vtable;
    return new_section;
}

QN_API qn_fl_section_ptr qn_fl_sec_duplicate(qn_fl_section_ptr restrict fs)
{
    return qn_fl_sec_create(fs->file, fs->offset, fs->sec_size);
}

QN_API qn_fl_section_ptr qn_fl_sec_section(qn_fl_section_ptr restrict fs, qn_fsize offset, size_t sec_size)
{
    if (offset < fs->offset || offset + sec_size > fs->offset + fs->sec_size) return NULL;
    qn_fl_section_ptr new_section = qn_fl_sec_create(fs->file, offset, sec_size);
    if (!new_section) return NULL;
    return new_section;
}

QN_API void qn_fl_sec_destroy(qn_fl_section_ptr restrict fs)
{
    if (fs) {
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
        qn_fl_close(fs->file);
#endif
        free(fs);
    } // if
}

QN_API qn_bool qn_fl_sec_reset(qn_fl_section_ptr restrict fs)
{
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
    if (! qn_fl_seek(fs->file, fs->offset)) return qn_false;
#endif

    fs->rem_size = fs->sec_size;
    return qn_true;
}

QN_API qn_io_reader_itf qn_fl_sec_to_io_reader(qn_fl_section_ptr restrict fs)
{
    return &fs->rdr_vtbl;
}

QN_API ssize_t qn_fl_sec_peek(qn_fl_section_ptr restrict fs, char * restrict buf, size_t buf_size)
{
    ssize_t ret;
    size_t read_size;

    if (fs->rem_size == 0) return QN_IO_RDR_EOF;

    read_size = (buf_size < fs->rem_size) ? buf_size : fs->rem_size;

#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
    ret = read(fs->file->fd, buf, read_size); 
    if (ret > 0 && !qn_fl_sec_advance(fs, -read_size)) return QN_IO_RDR_READING_FAILED;
#else
    ret = pread(fs->file->fd, buf, read_size, fs->offset + (fs->sec_size - fs->rem_size));
#endif

    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if

    fs->rem_size -= ret;
    return ret;
}

QN_API ssize_t qn_fl_sec_read(qn_fl_section_ptr restrict fs, char * restrict buf, size_t buf_size)
{
    ssize_t ret;
    size_t read_size;

    if (fs->rem_size == 0) return QN_IO_RDR_EOF;

    read_size = (buf_size < fs->rem_size) ? buf_size : fs->rem_size;

#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
    ret = read(fs->file->fd, buf, read_size); 
#else
    ret = pread(fs->file->fd, buf, read_size, fs->offset + (fs->sec_size - fs->rem_size));
#endif

    if (ret < 0) {
        qn_err_fl_set_reading_file_failed();
        return QN_IO_RDR_READING_FAILED;
    } // if

    fs->rem_size -= ret;
    return ret;
}

QN_API qn_bool qn_fl_sec_seek(qn_fl_section_ptr restrict fs, qn_fsize offset)
{
    if (offset < fs->offset) {
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
        if (! qn_fl_seek(fs->file, fs->offset)) return qn_false;
#endif
        fs->rem_size = fs->sec_size;
    } else if (offset > fs->offset + fs->sec_size) {
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
        if (! qn_fl_seek(fs->file, fs->offset + fs->sec_size)) return qn_false;
#endif
        fs->rem_size = 0;
    } else {
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
        if (! qn_fl_seek(fs->file, offset)) return qn_false;
#endif
        fs->rem_size = fs->offset + fs->sec_size - offset;
    } // if
    return qn_true;
}

QN_API qn_bool qn_fl_sec_advance(qn_fl_section_ptr restrict fs, size_t delta)
{
    if (delta <= fs->rem_size) {
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
        if (! qn_fl_advance(fs->file, delta)) return qn_false;
#endif
        fs->rem_size -= delta;
    } else {
#if !defined(QN_OS_FILE_SHARED_FOR_SECTION)
        if (! qn_fl_advance(fs->file, fs->rem_size)) return qn_false;
#endif
        fs->rem_size = 0;
    } // if
    return qn_true;
}

QN_API size_t qn_fl_sec_reader_callback(void * restrict user_data, char * restrict buf, size_t buf_size)
{
    size_t ret;
    qn_fl_section_ptr fs = (qn_fl_section_ptr) user_data;
    if ((ret = qn_fl_sec_read(fs, buf, buf_size)) <= 0) return 0;
    return ret;
}

#endif

#ifdef __cplusplus
}
#endif
