#ifndef __QN_HTTP_HEADER_H__
#define __QN_HTTP_HEADER_H__

#include "qiniu/base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of HTTP header ----

struct _QN_HTTP_HEADER;
typedef struct _QN_HTTP_HEADER * qn_http_header_ptr;

extern qn_http_header_ptr qn_http_hdr_create(void);
extern void qn_http_hdr_destroy(qn_http_header_ptr hdr);
extern void qn_http_hdr_reset(qn_http_header_ptr hdr);

extern qn_size qn_http_hdr_size(qn_http_header_ptr hdr);

extern qn_string qn_http_hdr_get_entry_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size);

static inline qn_string qn_http_hdr_get_entry(qn_http_header_ptr hdr, const qn_string key)
{
    return qn_http_hdr_get_entry_raw(hdr, qn_str_cstr(key), qn_str_size(key));
}

extern qn_bool qn_http_hdr_get_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size, const char ** val, qn_size * val_size);

static inline qn_bool qn_http_hdr_get(qn_http_header_ptr hdr, const qn_string key, const char ** val, qn_size * val_size)
{
    return qn_http_hdr_get_raw(hdr, qn_str_cstr(key), qn_str_size(key), val, val_size);
}

extern qn_bool qn_http_hdr_set_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size, const char * val, qn_size val_size);

static inline qn_bool qn_http_hdr_set(qn_http_header_ptr hdr, const qn_string key, const qn_string val)
{
    return qn_http_hdr_set_raw(hdr, qn_str_cstr(key), qn_str_size(key), qn_str_cstr(val), qn_str_size(val));
}

extern void qn_http_hdr_unset_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size);

static inline void qn_http_hdr_unset(qn_http_header_ptr hdr, const qn_string key)
{
    return qn_http_hdr_unset_raw(hdr, qn_str_cstr(key), qn_str_size(key));
}

// ---- Declaration of HTTP header iterator ----

struct _QN_HTTP_HDR_ITERATOR;
typedef struct _QN_HTTP_HDR_ITERATOR * qn_http_hdr_iterator_ptr;

extern qn_http_hdr_iterator_ptr qn_http_hdr_itr_create(qn_http_header_ptr hdr);
extern void qn_http_hdr_itr_destroy(qn_http_hdr_iterator_ptr itr);
extern void qn_http_hdr_itr_rewind(qn_http_hdr_iterator_ptr itr);
extern qn_bool qn_http_hdr_itr_next_pair_raw(qn_http_hdr_iterator_ptr itr, const char ** key, qn_size * key_size, const char ** val, qn_size * val_size);
extern const qn_string qn_http_hdr_itr_next_entry(qn_http_hdr_iterator_ptr itr);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_HEADER_H__

