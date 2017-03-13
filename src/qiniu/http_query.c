#include "qiniu/base/errors.h"
#include "qiniu/base/string.h"
#include "qiniu/base/ds/etable.h"
#include "qiniu/http_query.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of HTTP query ----

typedef struct _QN_HTTP_QUERY
{
    qn_etable_ptr etbl;
} qn_http_query;

QN_SDK qn_http_query_ptr qn_http_qry_create(void)
{
    qn_http_query_ptr new_qry = calloc(1, sizeof(qn_http_query));
    if (!new_qry) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_qry->etbl = qn_etbl_create("=");
    if (!new_qry->etbl) {
        free(new_qry);
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_qry;
}

QN_SDK void qn_http_qry_destroy(qn_http_query_ptr restrict qry)
{
    if (qry) {
        qn_etbl_destroy(qry->etbl);
        free(qry);
    } // if
}

QN_SDK void qn_http_qry_reset(qn_http_query_ptr restrict qry)
{
    qn_etbl_reset(qry->etbl); 
}

QN_SDK int qn_http_qry_count(qn_http_query_ptr restrict qry)
{
    return qn_etbl_count(qry->etbl);
}

QN_SDK qn_bool qn_http_qry_set_raw(qn_http_query_ptr restrict qry, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size)
{
    qn_bool ret;
    qn_string encoded_val = qn_cs_percent_encode(val, val_size);
    ret = qn_etbl_set_raw(qry->etbl, key, key_size, qn_str_cstr(encoded_val), qn_str_size(encoded_val));
    qn_str_destroy(encoded_val);
    return ret;
}

QN_SDK qn_bool qn_http_qry_set_integer(qn_http_query_ptr restrict qry, const char * restrict key, int value)
{
    return qn_etbl_set_integer(qry->etbl, key, value);
}

QN_SDK qn_string qn_http_qry_to_string(qn_http_query_ptr restrict qry)
{
    return qn_str_join_list("&", qn_etbl_entries(qry->etbl), qn_etbl_count(qry->etbl));
}

#ifdef __cplusplus
}
#endif

