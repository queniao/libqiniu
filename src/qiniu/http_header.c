#include <strings.h>

#include "qiniu/base/ds/etable.h"
#include "qiniu/base/errors.h"
#include "qiniu/http_header.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of HTTP header ----

typedef struct _QN_HTTP_HEADER
{
    qn_etable_ptr etbl;
} qn_http_header;

QN_SDK qn_http_header_ptr qn_http_hdr_create(void)
{
    qn_http_header_ptr new_hdr = calloc(1, sizeof(qn_http_header));
    if (!new_hdr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_hdr->etbl = qn_etbl_create(": ");
    if (!new_hdr->etbl) {
        free(new_hdr);
        return NULL;
    } // if
    return new_hdr;
}

QN_SDK void qn_http_hdr_destroy(qn_http_header_ptr restrict hdr)
{
    if (hdr) {
        qn_etbl_reset(hdr->etbl);
        qn_etbl_destroy(hdr->etbl);
        free(hdr);
    } // if
}

QN_SDK void qn_http_hdr_reset(qn_http_header_ptr restrict hdr)
{
    qn_etbl_reset(hdr->etbl);
}

QN_SDK int qn_http_hdr_count(qn_http_header_ptr restrict hdr)
{
    return qn_etbl_count(hdr->etbl);
}

QN_SDK qn_string qn_http_hdr_get_entry(qn_http_header_ptr restrict hdr, const char * restrict key)
{
    return qn_etbl_get_entry(hdr->etbl, key);
}

QN_SDK const char * qn_http_hdr_get_value(qn_http_header_ptr restrict hdr, const char * restrict key)
{
    return qn_etbl_get_value(hdr->etbl, key);
}

QN_SDK qn_bool qn_http_hdr_set_raw(qn_http_header_ptr restrict hdr, const char * restrict key, size_t key_size, const char * restrict val, size_t val_size)
{
    return qn_etbl_set_raw(hdr->etbl, key, key_size, val, val_size);
}

QN_SDK qn_bool qn_http_hdr_set_integer(qn_http_header_ptr restrict hdr, const char * restrict key, int val)
{
    return qn_etbl_set_integer(hdr->etbl, key, val);
}

QN_SDK void qn_http_hdr_unset(qn_http_header_ptr restrict hdr, const char * restrict key)
{
    qn_etbl_unset(hdr->etbl, key);
}

// ---- Definition of HTTP header iterator ----

QN_SDK qn_http_hdr_iterator_ptr qn_http_hdr_itr_create(qn_http_header_ptr restrict hdr)
{
    return qn_etbl_itr_create(hdr->etbl);
}

QN_SDK void qn_http_hdr_itr_destroy(qn_http_hdr_iterator_ptr restrict itr)
{
    qn_etbl_itr_destroy(itr);
}

QN_SDK void qn_http_hdr_itr_rewind(qn_http_hdr_iterator_ptr restrict itr)
{
    qn_etbl_itr_rewind(itr);
}

QN_SDK const qn_string qn_http_hdr_itr_next_entry(qn_http_hdr_iterator_ptr restrict itr)
{
    return qn_etbl_itr_next_entry(itr);
}

QN_SDK qn_bool qn_http_hdr_itr_next_pair_raw(qn_http_hdr_iterator_ptr restrict itr, const char ** restrict key, size_t * restrict key_size, const char ** restrict val, size_t * restrict val_size)
{
    return qn_etbl_itr_next_pair_raw(itr, key, key_size, val, val_size);
}

#ifdef __cplusplus
}
#endif

