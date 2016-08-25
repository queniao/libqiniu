#ifndef __QN_OS_FILE_H__
#define __QN_OS_FILE_H__

#include "qiniu/base/string.h"

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

struct _QN_FILE_INFO;
typedef struct _QN_FILE_INFO * qn_file_info_ptr;

extern qn_file_info_ptr qn_fi_create(void);
extern void qn_fi_destroy(qn_file_info_ptr fi);

extern qn_file_info_ptr qn_fi_stat_raw(const char * fname);

static inline qn_file_info_ptr qn_fi_stat(qn_string fname)
{
    return qn_fi_stat_raw(qn_str_cstr(fname));
}

extern qn_fsize qn_fi_file_size(qn_file_info_ptr fi);
extern qn_string qn_fi_file_name(qn_file_info_ptr fi);

#ifdef __cplusplus
}
#endif

#endif // __QN_OS_FILE_H__
