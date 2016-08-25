#include "qiniu/base/errors.h"
#include "qiniu/os/file.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of file info ----

typedef struct _QN_FILE_INFO
{
    qn_string fname;
    qn_fsize fsize;
} qn_file_info;

qn_file_info_ptr qn_fi_create(void)
{
    qn_file_info_ptr new_fi = calloc(1, sizeof(qn_file_info));
    if (!new_fi) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if
    return new_fi;
}

void qn_fi_destroy(qn_file_info_ptr fi)
{
    if (fi) {
        free(fi);
    } // fi
}

qn_fsize qn_fi_file_size(qn_file_info_ptr fi)
{
    return fi->fsize;
}

qn_string qn_fi_file_name(qn_file_info_ptr fi)
{
    return fi->fname;
}

#if defined(_MSC_VER)

#else

// ---- Definition of file info depends on operating system ----

#include <sys/stat.h>

qn_file_info_ptr qn_fi_stat_raw(const char * fname)
{
    struct stat st;

    qn_file_info_ptr fi = qn_fi_create();
    if (!fi) return NULL;

    if (stat(fname, &st) < 0) {
        qn_fi_destroy(fi);
        qn_err_fi_set_stating_file_info_failed();
        return NULL;
    } // if

    fi->fname = qn_str_duplicate(fname);
    if (!fi->fname) {
        qn_fi_destroy(fi);
        return NULL;
    } // if

    fi->fsize = st.st_size;

    return fi;
}

#endif

#ifdef __cplusplus
}
#endif
