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
    if ((fc->offset += *buf_size) > fc->max_fsize) return QN_RDR_READING_ABORTED;
    return QN_RDR_SUCCEED ;
}

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string scope = NULL;
    qn_string put_ret = NULL;
    const char * bucket = NULL;
    const char * key = NULL;
    const char * fname = NULL;
    qn_storage_ptr stor = NULL;
    qn_stor_auth auth;
    qn_file_ptr fl = NULL;
    qn_io_reader base_rdr;
    qn_reader_ptr ctrl_rdr = NULL;
    qn_flt_etag_ptr etag = NULL;
    qn_stor_put_extra ext;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 6) {
        printf("Usage: qput <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME> [SIZE_LIMIT]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    memset(&ext, 0, sizeof(ext));
    ext.final_key = key;

    if (argc == 7) {
        fc.max_fsize = atoi(argv[6]);
        fc.offset = 0;

        fl = qn_fl_open(fname, NULL);
        if (!fl) {
            printf("Cannot open the '%s' file.\n", fname);
            return 1;
        } // if

        memset(&base_rdr, 0, sizeof(base_rdr));
        base_rdr.user_data = fl;
        base_rdr.read = (qn_io_read) &qn_fl_read;
        base_rdr.advance = (qn_io_advance) &qn_fl_advance;

        ctrl_rdr = qn_rdr_create(&base_rdr, 2);
        if (!ctrl_rdr) {
            qn_fl_close(fl);
            printf("Cannot create a controllabl reader.\n");
            return 1;
        } // if
        if (!qn_rdr_add_post_filter(ctrl_rdr, &fc, fsize_checker_post_callback)) {
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
            printf("Cannot add the fsize checker.\n");
            return 1;
        } // if

        etag = qn_flt_etag_create();
        if (!etag) {
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
            printf("Cannot create a new etag calculator.\n");
            return 1;
        } // if

        if (!qn_rdr_add_post_filter(ctrl_rdr, etag, qn_flt_etag_callback)) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
            printf("Cannot add the etag filter.\n");
            return 1;
        } // if

        ext.put_ctrl.rdr = ctrl_rdr;
    } // if

    memset(&auth, 0, sizeof(auth));
    auth.server_end.mac = mac;
    auth.server_end.put_policy = qn_json_create_object();
    if (!auth.server_end.put_policy) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        printf("Cannot create a put policy.\n");
        return 1;
    } // if

    scope = qn_cs_sprintf("%s:%s", bucket, key);
    if (!scope) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_json_destroy_object(auth.server_end.put_policy);
        printf("Cannot format a valid scope for inserting the file.\n");
        return 1;
    } // if

    if (!qn_json_set_string(auth.server_end.put_policy, "scope", qn_str_cstr(scope))) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_str_destroy(scope);
        qn_json_destroy_object(auth.server_end.put_policy);
        printf("Cannot set the scope field.\n");
        return 1;
    } // if
    qn_str_destroy(scope);

    if (!qn_json_set_integer(auth.server_end.put_policy, "deadline", time(NULL) + 3600)) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_json_destroy_object(auth.server_end.put_policy);
        printf("Cannot set the deadline field.\n");
        return 1;
    } // if

    stor = qn_stor_create();
    if (!stor) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_json_destroy_object(auth.server_end.put_policy);
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    if (!qn_stor_put_file(stor, &auth, fname, &ext)) {
        if (qn_err_stor_is_putting_aborted_by_filter_post_callback()) {
            printf("The '%s' file's size is larger than the given size limit %lu.\n", fname, fc.max_fsize);
        } // if
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_stor_destroy(stor);
        qn_json_destroy_object(auth.server_end.put_policy);
        printf("Cannot put the file `%s` to `%s:%s`.\n", fname, bucket, key);
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while
    qn_http_hdr_itr_destroy(hdr_itr);

    put_ret = qn_json_object_to_string(qn_stor_get_object_body(stor));
    if (!put_ret) {
        if (ctrl_rdr) {
            qn_flt_etag_destroy(etag);
            qn_rdr_destroy(ctrl_rdr);
            qn_fl_close(fl);
        } // if
        qn_stor_destroy(stor);
        qn_json_destroy_object(auth.server_end.put_policy);
        printf("Cannot format the object body from upload interface.\n");
        return 3;
    } // if

    printf("%s\n", put_ret);
    qn_str_destroy(put_ret);

    if (ctrl_rdr) {
        put_ret = qn_flt_etag_result(etag);
        printf("The etag of the given local file is `%s`.\n", put_ret);
        qn_str_destroy(put_ret);

        qn_flt_etag_destroy(etag);
        qn_rdr_destroy(ctrl_rdr);
        qn_fl_close(fl);
    } // if
    qn_stor_destroy(stor);
    qn_json_destroy_object(auth.server_end.put_policy);
    qn_mac_destroy(mac);
    return 0;
}
