#include <stdio.h>
#include <time.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

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
    qn_file_ptr fl;
    qn_fl_info_ptr fi;
    qn_storage_ptr stor;
    qn_stor_put_extra ext;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;
    char * buf;
    int buf_size;

    if (argc < 5) {
        printf("qputfb - Put single file by reading it into buffer first.");
        printf("Usage: qputfb <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME>\n");
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

    put_policy = qn_pp_create(bucket, key, time(NULL) + 3600);
    if (! put_policy) {
        qn_mac_destroy(mac);
        printf("Cannot create a put policy due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    uptoken = qn_pp_to_uptoken(put_policy, mac);
    qn_json_destroy_object(put_policy);
    qn_mac_destroy(mac);
    if (! uptoken) {
        printf("Cannot make an uptoken due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    fi = qn_fl_info_stat(fname);
    if (! fi) {
        qn_str_destroy(uptoken);
        printf("Cannot stat the file `%s` due to application error `%s`.\n", fname, qn_err_get_message());
        return 1;
    } // if

    buf_size = qn_fl_info_fsize(fi);
    qn_fl_info_destroy(fi);

    buf = malloc(buf_size);
    if (! buf) {
        qn_str_destroy(uptoken);
        printf("Cannot allocate enough memory to load the content of the file `%s`.\n", fname);
        return 1;
    } // if

    fl = qn_fl_open(fname, NULL);
    if (! fl) {
        free(buf);
        qn_str_destroy(uptoken);
        printf("Cannot open the file `%s` due to application error `%s`.\n", fname, qn_err_get_message());
        return 1;
    } // if

    if (qn_fl_read(fl, buf, buf_size) <= 0) {
        qn_fl_close(fl);
        free(buf);
        qn_str_destroy(uptoken);
        printf("Cannot read the file `%s`, or it is an empty one due to application error `%s`.\n", fname, qn_err_get_message());
        return 1;
    } // if
    qn_fl_close(fl);

    memset(&ext, 0, sizeof(ext));
    ext.final_key = key;

    stor = qn_stor_create();
    if (!stor) {
        free(buf);
        qn_str_destroy(uptoken);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    put_ret = qn_stor_put_buffer(stor, uptoken, buf, buf_size, &ext);
    free(buf);
    qn_str_destroy(uptoken);
    if (! put_ret) {
        qn_stor_destroy(stor);
        printf("Cannot put the file `%s` to `%s:%s` due to application error `%s`.\n", fname, bucket, key, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
    qn_http_hdr_itr_destroy(hdr_itr);

    put_ret_str = qn_json_object_to_string(put_ret);
    qn_stor_destroy(stor);
    if (!put_ret_str) {
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", put_ret_str);
    qn_str_destroy(put_ret_str);

    return 0;
}
