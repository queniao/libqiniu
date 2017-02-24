#include <stdio.h>
#include <time.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/os/file.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    int start_idx;
    const char * bucket;
    const char * key;
    const char * fname;
    qn_stor_upload_progress_ptr up;
    qn_file_ptr fl;
    qn_mac_ptr mac;
    qn_string uptoken;
    qn_string up_ret_str;
    qn_json_object_ptr put_policy;
    qn_json_object_ptr up_ret;
    qn_storage_ptr stor;
    qn_stor_upload_extra_ptr ue;

    if (argc < 5) {
        printf("Demo qputh - Put single file in many HTTP sessions.\n");
        printf("Usage: qputh <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME> [PROGRESS_FNAME]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a mac object due to application error `%s`.\n", qn_err_get_message());
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
        printf("Cannot make a uptoken due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    fl = qn_fl_open(fname, NULL);
    if (! fl) {
        qn_str_destroy(uptoken);
        printf("Cannot open the uploading file due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    up = qn_stor_up_create(qn_fl_to_io_reader(fl));
    if (! up) {
        qn_str_destroy(uptoken);
        qn_fl_close(fl);
        printf("Cannot create a new upload progress object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    ue = qn_stor_ue_create();
    if (! ue) {
        qn_str_destroy(uptoken);
        qn_fl_close(fl);
        printf("Cannot create a upload extra due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    qn_stor_ue_set_final_key(ue, key);

    stor = qn_stor_create();
    if (! stor) {
        qn_stor_ue_destroy(ue);
        qn_str_destroy(uptoken);
        qn_stor_up_destroy(up);
        qn_fl_close(fl);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    start_idx = 0;
    up_ret = qn_stor_upload_huge(stor, uptoken, up, &start_idx, QN_STOR_UP_CHUNK_DEFAULT_SIZE, ue);
    qn_stor_ue_destroy(ue);
    qn_str_destroy(uptoken);
    qn_stor_up_destroy(up);
    qn_fl_close(fl);
    if (! up_ret) {
        qn_stor_destroy(stor);
        printf("Cannot upload the file `%s` to `%s:%s` due to application error `%s`.\n", fname, bucket, key, qn_err_get_message());
        return 2;
    } // if

    up_ret_str = qn_json_object_to_string(up_ret);
    qn_stor_destroy(stor);
    if (! up_ret_str) {
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", up_ret_str);
    qn_str_destroy(up_ret_str);

    return 0;
}
