#include <stdio.h>
#include <time.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/base/io.h"
#include "qiniu/base/errors.h"
#include "qiniu/reader.h"
#include "qiniu/reader_filter.h"
#include "qiniu/storage.h"

typedef struct _FSIZE_CHECKER {
    size_t max_fsize;
    size_t offset;
} fsize_checker, *fsize_checker_ptr;

static fsize_checker fc;

static ssize_t fsize_checker_post_callback(void * user_data, char ** restrict buf, size_t * restrict buf_size)
{
    fsize_checker_ptr fc = (fsize_checker_ptr) user_data;
    if ((fc->offset += *buf_size) > fc->max_fsize) return QN_IO_RDR_READING_ABORTED;
    return *buf_size;
}

int main(int argc, char * argv[])
{
    const char * bucket;
    const char * key;
    const char * fname;
    qn_mac_ptr mac;
    qn_string uptoken;
    qn_string put_ret_str;
    qn_json_object_ptr put_policy;
    qn_json_object_ptr put_ret;
    qn_storage_ptr stor;
    qn_file_ptr fl;
    qn_reader_ptr ctrl_rdr = NULL;
    qn_flt_etag_ptr etag;
    qn_stor_put_extra_ptr pe;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 6) {
        printf("Demo qputfa - Put single file in one HTTP session and abort if it is larger than the given size.\n");
        printf("Usage: qputfa <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME> [SIZE_LIMIT]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    put_policy = qn_stor_pp_create(bucket, key, time(NULL) + 3600);
    if (! put_policy) {
        qn_mac_destroy(mac);
        printf("Cannot create a put policy due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    uptoken = qn_stor_pp_to_uptoken(put_policy, mac);
    qn_json_destroy_object(put_policy);
    qn_mac_destroy(mac);
    if (! uptoken) {
        printf("Cannot make an uptoken due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    pe = qn_stor_pe_create();
    if (! pe) {
        qn_str_destroy(uptoken);
        printf("Cannot create a put extra due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    qn_stor_pe_set_final_key(pe, key);

    if (argc == 7) {
        fc.max_fsize = atoi(argv[6]);
        fc.offset = 0;

        fl = qn_fl_open(fname, NULL);
        if (!fl) {
            qn_str_destroy(uptoken);
            printf("Cannot open the '%s' file due to application error `%s`.\n", fname, qn_err_get_message());
            return 1;
        } // if

        ctrl_rdr = qn_rdr_create(qn_fl_to_io_reader(fl), 2);
        if (!ctrl_rdr) {
            qn_fl_close(fl);
            qn_str_destroy(uptoken);
            printf("Cannot create a controllabl reader due to application error `%s`.\n", qn_err_get_message());
            return 1;
        } // if
        if (!qn_rdr_add_post_filter(ctrl_rdr, &fc, fsize_checker_post_callback)) {
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
            qn_str_destroy(uptoken);
            printf("Cannot add the fsize checker due to application error `%s`.\n", qn_err_get_message());
            return 1;
        } // if

        etag = qn_flt_etag_create();
        if (!etag) {
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
            qn_str_destroy(uptoken);
            printf("Cannot create a new etag calculator due to application error `%s`.\n", qn_err_get_message());
            return 1;
        } // if

        if (!qn_rdr_add_post_filter(ctrl_rdr, etag, qn_flt_etag_callback)) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
            qn_str_destroy(uptoken);
            printf("Cannot add the etag filter due to application error `%s`.\n", qn_err_get_message());
            return 1;
        } // if

        qn_stor_pe_set_source_reader(pe, qn_rdr_to_io_reader(ctrl_rdr), 0, qn_false);
    } // if

    stor = qn_stor_create();
    if (! stor) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_stor_pe_destroy(pe);
        qn_str_destroy(uptoken);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    put_ret = qn_stor_put_file(stor, uptoken, fname, pe);
    qn_stor_pe_destroy(pe);
    qn_str_destroy(uptoken);
    if (! put_ret) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_stor_destroy(stor);

        if (qn_err_stor_is_putting_aborted_by_filter_post_callback()) {
            printf("The '%s' file's size is larger than the given size limit %lu.\n", fname, fc.max_fsize);
        } else {
            printf("Cannot put the file `%s` to `%s:%s` due to application error `%s`.\n", fname, bucket, key, qn_err_get_message());
        } // if
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
    qn_http_hdr_itr_destroy(hdr_itr);

    put_ret_str = qn_json_object_to_string(put_ret);
    qn_stor_destroy(stor);
    if (! put_ret_str) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", put_ret_str);
    qn_str_destroy(put_ret_str);

    if (ctrl_rdr) {
        put_ret_str = qn_flt_etag_result(etag);
        printf("The etag of the given local file is `%s`.\n", put_ret_str);
        qn_str_destroy(put_ret_str);

        qn_flt_etag_destroy(etag);
        qn_rdr_destroy(ctrl_rdr);
        qn_fl_close(fl);
    } // if

    return 0;
}
