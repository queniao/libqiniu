#include "qiniu/base/errors.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/http.h"
#include "qiniu/storage.h"
#include "qiniu/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_STORAGE
{
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
    qn_http_connection_ptr conn;
    qn_http_json_writer_ptr resp_json_wrt;
    qn_json_object_ptr obj_body;
    qn_json_array_ptr arr_body;
} qn_storage;

qn_storage_ptr qn_stor_create(void)
{
    qn_storage_ptr new_stor = NULL;

    new_stor = calloc(1, sizeof(qn_storage));
    if (!new_stor) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_stor->req = qn_http_req_create();
    if (!new_stor->req) {
        free(new_stor);
        return NULL;
    } // if

    new_stor->resp = qn_http_resp_create();
    if (!new_stor->resp) {
        qn_http_req_destroy(new_stor->req);
        free(new_stor);
        return NULL;
    } // if

    new_stor->conn = qn_http_conn_create();
    if (!new_stor->conn) {
        qn_http_resp_destroy(new_stor->resp);
        qn_http_req_destroy(new_stor->req);
        free(new_stor);
        return NULL;
    } // if

    new_stor->resp_json_wrt = qn_http_json_wrt_create();
    if (!new_stor->resp_json_wrt) {
        qn_http_conn_destroy(new_stor->conn);
        qn_http_resp_destroy(new_stor->resp);
        qn_http_req_destroy(new_stor->req);
        free(new_stor);
        return NULL;
    } // if

    return new_stor;
}

void qn_stor_destroy(qn_storage_ptr stor)
{
    if (stor) {
        if (stor->obj_body) qn_json_destroy_object(stor->obj_body);
        if (stor->arr_body) qn_json_destroy_array(stor->arr_body);
        qn_http_conn_destroy(stor->conn);
        qn_http_resp_destroy(stor->resp);
        qn_http_req_destroy(stor->req);
        free(stor);
    } // if
}

qn_json_object_ptr qn_stor_get_object_body(qn_storage_ptr stor)
{
    return stor->obj_body;
}

qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(qn_storage_ptr stor)
{
    return qn_http_resp_get_header_iterator(stor->resp);
}

static qn_bool qn_stor_prepare_common_request_headers(qn_storage_ptr stor)
{
    if (!qn_http_req_set_header(stor->req, "Expect", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Transfer-Encoding", "")) return qn_false;
    return qn_true;
}

// ---- Definition of Management ----

static qn_bool qn_stor_prepare_managment(qn_storage_ptr stor, const qn_string restrict url, qn_string restrict acctoken, const qn_mac_ptr restrict mac)
{
    qn_string auth_header;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (acctoken) {
        auth_header = qn_str_sprintf("QBox %s", acctoken);
    } else if (mac) {
        acctoken = qn_mac_make_acctoken(mac, url, NULL, 0);
        if (!acctoken) return qn_false;

        auth_header = qn_str_sprintf("QBox %s", acctoken);
        qn_str_destroy(acctoken);
    } else {
        qn_err_set_invalid_argument();
        return qn_false;
    } // if
    if (!auth_header) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Authorization", auth_header)) return qn_false;
    qn_str_destroy(auth_header);

    if (!qn_stor_prepare_common_request_headers(stor)) return qn_false;

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);
    return qn_true;
}

qn_bool qn_stor_stat(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, qn_stor_query_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_uri;
    qn_string url;

    // ---- Prepare the query URL
    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return qn_false;

    url = qn_str_sprintf("%s/stat/%s", "http://rs.qiniu.com", qn_str_cstr(encoded_uri));
    qn_str_destroy(encoded_uri);
    if (!url) return qn_false;

    if (!qn_stor_prepare_managment(stor, url, ext->client_end.acctoken, ext->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    ret = qn_http_conn_get(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

qn_bool qn_stor_copy(qn_storage_ptr stor, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_copy_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_src_uri;
    qn_string encoded_dest_uri;
    qn_string url;
    qn_string url2;

    // ---- Prepare the query URL
    encoded_src_uri = qn_misc_encode_uri(src_bucket, src_key);
    if (!encoded_src_uri) return qn_false;

    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) {
        qn_str_destroy(encoded_src_uri);
        return qn_false;
    } // if

    url = qn_str_sprintf("%s/copy/%s/%s", "http://rs.qiniu.com", qn_str_cstr(encoded_src_uri), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_src_uri);
    qn_str_destroy(encoded_dest_uri);
    if (!url) return qn_false;

    if (ext) {
        if (ext->force) {
            url2 = qn_str_sprintf("%s/force/true", url);
            qn_str_destroy(url);
            if (!url2) return qn_false;
            url = url2;
        } // if
    } // if

    if (!qn_stor_prepare_managment(stor, url, ext->client_end.acctoken, ext->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    qn_http_req_set_body_data(stor->req, "", 0);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

qn_bool qn_stor_move(qn_storage_ptr stor, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_move_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_src_uri;
    qn_string encoded_dest_uri;
    qn_string url;

    // ---- Prepare the query URL
    encoded_src_uri = qn_misc_encode_uri(src_bucket, src_key);
    if (!encoded_src_uri) return qn_false;

    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) {
        qn_str_destroy(encoded_src_uri);
        return qn_false;
    } // if

    url = qn_str_sprintf("%s/move/%s/%s", "http://rs.qiniu.com", qn_str_cstr(encoded_src_uri), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_src_uri);
    qn_str_destroy(encoded_dest_uri);
    if (!url) return qn_false;

    if (!qn_stor_prepare_managment(stor, url, ext->client_end.acctoken, ext->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    qn_http_req_set_body_data(stor->req, "", 0);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

qn_bool qn_stor_delete(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, qn_stor_delete_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_uri;
    qn_string url;

    // ---- Prepare the query URL
    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return qn_false;

    url = qn_str_sprintf("%s/delete/%s", "http://rs.qiniu.com", qn_str_cstr(encoded_uri));
    qn_str_destroy(encoded_uri);
    if (!url) return qn_false;

    if (!qn_stor_prepare_managment(stor, url, ext->client_end.acctoken, ext->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    qn_http_req_set_body_data(stor->req, "", 0);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

qn_bool qn_stor_change_mime(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_change_mime_extra_ptr restrict ext)
{
    return qn_true;
}

// ---- Definition of Upload ----

static qn_bool qn_stor_prepare_for_putting_file(qn_storage_ptr stor, qn_stor_put_extra_ptr ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_http_form_ptr form;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    if (!qn_stor_prepare_common_request_headers(stor)) return qn_false;

    // ----
    if (! (form = qn_http_req_prepare_form(stor->req))) return qn_false;

    // uptoken MUST be the first form item.
    if (ext->client_end.uptoken) {
        if (!qn_http_form_add_string(form, "token", ext->client_end.uptoken, strlen(ext->client_end.uptoken))) return qn_false;
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!uptoken) return qn_false;

        ret = qn_http_form_add_string(form, "token", qn_str_cstr(uptoken), qn_str_size(uptoken));
        qn_str_destroy(uptoken);
        if (!ret) return qn_false;
    } else {
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if

    if (ext->final_key && !qn_http_form_add_string(form, "key", ext->final_key, strlen(ext->final_key))) return qn_false;
    if (ext->crc32 && !qn_http_form_add_string(form, "crc32", ext->crc32, strlen(ext->crc32))) return qn_false;
    if (ext->accept_type && !qn_http_form_add_string(form, "accept", ext->accept_type, strlen(ext->accept_type))) return qn_false;

    // TODO: User defined variabales.

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    return qn_true;
}

qn_bool qn_stor_put_file(qn_storage_ptr stor, const char * fname, qn_stor_put_extra_ptr ext)
{
    qn_bool ret;
    qn_fl_info_ptr fi;
    qn_http_form_ptr form;

    if (!qn_stor_prepare_for_putting_file(stor, ext)) return qn_false;

    form = qn_http_req_get_form(stor->req);

    fi = qn_fl_info_stat(fname);
    if (!fi) return qn_false;

    ret = qn_http_form_add_file(form, "file", qn_str_cstr(qn_fl_info_fname(fi)), NULL, qn_fl_info_fsize(fi));
    qn_fl_info_destroy(fi);
    if (!ret) return qn_false;

    // ----
    return qn_http_conn_post(stor->conn, "http://up.qiniu.com", stor->req, stor->resp);
}

qn_bool qn_stor_put_buffer(qn_storage_ptr stor, const char * buf, int buf_size, qn_stor_put_extra_ptr ext)
{
    qn_http_form_ptr form;

    if (!qn_stor_prepare_for_putting_file(stor, ext)) return qn_false;

    form = qn_http_req_get_form(stor->req);

    if (!qn_http_form_add_buffer(form, "file", "<null>", buf, buf_size)) return qn_false;

    // ----
    return qn_http_conn_post(stor->conn, "http://up.qiniu.com", stor->req, stor->resp);
}

// ----

typedef struct _QN_STOR_RESUMABLE_PUT_SESSION
{
    qn_fsize fsize;
    qn_json_array_ptr blk_info_list;
} qn_stor_rput_session;

#define QN_STOR_RPUT_BLOCK_MAX_SIZE (1L << 22)
#define QN_STOR_RPUT_CHUNK_DEFAULT_SIZE (1024 * 256)

qn_stor_rput_session_ptr qn_stor_rs_create(qn_fsize fsize)
{
    int i;
    int blk_count;
    int last_blk_size;
    qn_json_object_ptr blk_info;
    qn_stor_rput_session_ptr new_ss = calloc(1, sizeof(qn_stor_rput_session));
    if (!new_ss) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_ss->blk_info_list = qn_json_create_array();
    if (!new_ss->blk_info_list) {
        free(new_ss);
        return NULL;
    } // if
    new_ss->fsize = fsize;

    last_blk_size = fsize % QN_STOR_RPUT_BLOCK_MAX_SIZE;
    blk_count = (fsize / QN_STOR_RPUT_BLOCK_MAX_SIZE);
    for (i = 0; i < blk_count; i += 1) {
        if (! (blk_info = qn_json_create_and_push_object(new_ss->blk_info_list))) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        if (!qn_json_set_integer(blk_info, "bsize", QN_STOR_RPUT_BLOCK_MAX_SIZE)) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
    } // for
    if (last_blk_size > 0) {
        if (! (blk_info = qn_json_create_and_push_object(new_ss->blk_info_list))) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        if (!qn_json_set_integer(blk_info, "bsize", last_blk_size)) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
    } // if
    return new_ss;
}

void qn_stor_rs_destroy(qn_stor_rput_session_ptr ss)
{
    if (ss) {
        qn_json_destroy_array(ss->blk_info_list);
        free(ss);
    } // if
}

qn_stor_rput_session_ptr qn_stor_rs_from_string(const char * str, int str_size)
{
    qn_bool ret;
    qn_size ret_size;
    qn_json_parser_ptr prs;
    qn_stor_rput_session_ptr new_ss = calloc(1, sizeof(qn_stor_rput_session));

    if (!new_ss) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if
    
    prs = qn_json_prs_create();
    if (!prs) {
        free(new_ss);
        return NULL;
    } // if

    if (str_size <= 0) str_size = strlen(str);
    ret_size = str_size;

    ret = qn_json_prs_parse_array(prs, str, &ret_size, &new_ss->blk_info_list);
    qn_json_prs_destroy(prs);
    if (!ret) {
        free(new_ss);
        return NULL;
    } // if
    return new_ss;
}

qn_string qn_stor_rs_to_string(qn_stor_rput_session_ptr ss)
{
    return qn_json_array_to_string(ss->blk_info_list);
}

int qn_stor_rs_block_count(qn_stor_rput_session_ptr ss)
{
    return qn_json_size_array(ss->blk_info_list);
}

int qn_stor_rs_block_size(qn_stor_rput_session_ptr ss, int n)
{
    qn_json_object_ptr blk_info = qn_json_pick_object(ss->blk_info_list, n, NULL);
    if (!blk_info) return 0;
    return qn_json_get_integer(blk_info, "bsize", 0);
}

qn_json_object_ptr qn_stor_rs_block_info(qn_stor_rput_session_ptr ss, int n)
{
    return qn_json_pick_object(ss->blk_info_list, n, NULL);
}

qn_bool qn_stor_rs_is_putting_block_done(qn_stor_rput_session_ptr ss, int n)
{
    qn_json_object_ptr blk_info = qn_json_pick_object(ss->blk_info_list, n, NULL);
    if (!blk_info) return qn_false;
    return (qn_json_get_integer(blk_info, "offset", 0) == qn_json_get_integer(blk_info, "bsize", 0));
}

// ----

typedef struct _QN_STOR_RESUMABLE_PUT_READER
{
    qn_io_reader_ptr rdr;
    qn_stor_rput_extra_ptr ext;
} qn_stor_rput_reader, *qn_stor_rput_reader_ptr;

static qn_size qn_stor_rp_chunk_body_reader_callback(void * user_data, char * buf, qn_size size)
{
    qn_size ret;
    qn_stor_rput_reader_ptr chk_rdr = (qn_stor_rput_reader_ptr) user_data;
    ret = chk_rdr->rdr->read(chk_rdr->rdr->user_data, buf, size);
    return ret;
}

static qn_bool qn_stor_rp_put_chunk_in_one_piece(qn_storage_ptr stor, qn_json_object_ptr blk_info, qn_io_reader_ptr rdr, int chk_size, qn_string url, qn_stor_rput_extra_ptr ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_string auth_header;
    qn_string content_length;
    qn_string ctx;
    qn_string checksum;
    qn_string host;
    qn_integer crc32;
    qn_integer offset;
    qn_io_section_reader srdr;
    qn_io_reader rdr2;
    qn_stor_rput_reader chk_rdr;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    if (ext->client_end.uptoken) {
        auth_header = qn_str_sprintf("UpToken %s", ext->client_end.uptoken);
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!uptoken) return qn_false;

        auth_header = qn_str_sprintf("UpToken %s", uptoken);
        qn_str_destroy(uptoken);
    } else {
        // TODO: Set an appropriate error.
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if
    if (!auth_header) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Authorization", auth_header)) {
        qn_str_destroy(auth_header);
        return qn_false;
    } // if
    qn_str_destroy(auth_header);

    if (!qn_http_req_set_header(stor->req, "Expect", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Transfer-Encoding", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Type", "application/octet-stream")) return qn_false;

    content_length = qn_str_sprintf("%d", chk_size);
    if (!content_length) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Length", qn_str_cstr(content_length))) {
        qn_str_destroy(content_length);
        return qn_false;
    } // if
    qn_str_destroy(content_length);

    qn_io_srd_init(&srdr, rdr, chk_size);
    rdr2.user_data = &srdr;
    rdr2.read = &qn_io_srd_read;

    memset(&chk_rdr, 0, sizeof(qn_stor_rput_reader));
    chk_rdr.rdr = &rdr2;
    chk_rdr.ext = ext;
    qn_http_req_set_body_reader(stor->req, &chk_rdr, qn_stor_rp_chunk_body_reader_callback, chk_size);

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ----
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    if (ret) {
        ctx = qn_json_get_string(stor->obj_body, "ctx", NULL);
        checksum = qn_json_get_string(stor->obj_body, "checksum", NULL);
        host = qn_json_get_string(stor->obj_body, "host", NULL);
        crc32 = qn_json_get_integer(stor->obj_body, "crc32", -1);
        offset = qn_json_get_integer(stor->obj_body, "offset", -1);

        if (!ctx || !checksum || !host || crc32 < 0 || offset < 0) {
            // TODO: Set an appropriate error.
            return qn_false;
        } // if

        if (!qn_json_set_string(blk_info, "ctx", ctx)) return qn_false;
        if (!qn_json_set_string(blk_info, "checksum", checksum)) return qn_false;
        if (!qn_json_set_string(blk_info, "host", host)) return qn_false;

        if (!qn_json_set_integer(blk_info, "crc32", crc32)) return qn_false;
        if (!qn_json_set_integer(blk_info, "offset", offset)) return qn_false;
    } // if
    return ret;
}

qn_bool qn_stor_rp_put_chunk(qn_storage_ptr stor, qn_json_object_ptr blk_info, qn_io_reader_ptr rdr, int chk_size, qn_stor_rput_extra_ptr ext)
{
    qn_bool ret;
    qn_string host;
    qn_string ctx;
    qn_string url;
    int chk_offset;
    int blk_size;
    
    chk_offset = qn_json_get_integer(blk_info, "offset", 0);
    blk_size = qn_json_get_integer(blk_info, "bsize", 0);

    if (chk_offset == blk_size) return qn_true;

    if (chk_offset == 0) {
        url = qn_str_sprintf("http://up.qiniu.com/mkblk/%d", blk_size);
    } else {
        host = qn_json_get_string(blk_info, "host", "");
        ctx = qn_json_get_string(blk_info, "ctx", "");
        url = qn_str_sprintf("%s/bput/%s/%d", host, qn_str_cstr(ctx), chk_offset);
    } // if
    ret = qn_stor_rp_put_chunk_in_one_piece(stor, blk_info, rdr, chk_size, url, ext);
    qn_str_destroy(url);
    return ret;
}

qn_bool qn_stor_rp_put_block(qn_storage_ptr stor, qn_json_object_ptr blk_info, qn_io_reader_ptr rdr, qn_stor_rput_extra_ptr ext)
{
    int chk_size;
    int chk_offset;
    int blk_size;
    int sending_bytes;

    chk_size = ext->chk_size;
    if (chk_size <= 0) chk_size = QN_STOR_RPUT_CHUNK_DEFAULT_SIZE;

    chk_offset = qn_json_get_integer(blk_info, "offset", 0);
    blk_size = qn_json_get_integer(blk_info, "bsize", 0);

    while (chk_offset < blk_size) {
        sending_bytes = blk_size - chk_offset;
        if (sending_bytes > chk_size) sending_bytes = chk_size;
        if (!qn_stor_rp_put_chunk(stor, blk_info, rdr, sending_bytes, ext)) return qn_false;
        chk_offset += sending_bytes;
    } // while
    return qn_true;
}

static qn_bool qn_stor_rp_make_file_to_one_piece(qn_storage_ptr stor, qn_stor_rput_session_ptr ss, const qn_string url, const qn_string ctx_info, qn_stor_rput_extra_ptr ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_string auth_header;
    qn_string content_length;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    if (ext->client_end.uptoken) {
        auth_header = qn_str_sprintf("UpToken %s", ext->client_end.uptoken);
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!uptoken) return qn_false;

        auth_header = qn_str_sprintf("UpToken %s", uptoken);
        qn_str_destroy(uptoken);
    } else {
        // TODO: Set an appropriate error;
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if
    if (!auth_header) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Authorization", auth_header)) {
        qn_str_destroy(auth_header);
        return qn_false;
    } // if
    qn_str_destroy(auth_header);

    if (!qn_http_req_set_header(stor->req, "Expect", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Type", "text/plain")) return qn_false;

    content_length = qn_str_sprintf("%d", qn_str_size(ctx_info));
    if (!content_length) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Length", qn_str_cstr(content_length))) {
        qn_str_destroy(content_length);
        return qn_false;
    } // if
    qn_str_destroy(content_length);

    qn_http_req_set_body_data(stor->req, qn_str_cstr(ctx_info), qn_str_size(ctx_info));

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ----
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    return ret;
}

qn_bool qn_stor_rp_make_file(qn_storage_ptr stor, qn_stor_rput_session_ptr ss, qn_stor_rput_extra_ptr ext)
{
    qn_bool ret;
    qn_json_object_ptr blk_info;
    int blk_count;
    int i;
    qn_integer chk_offset;
    qn_integer blk_size;
    qn_string encoded_key;
    qn_string host;
    qn_string url;
    qn_string url2;
    qn_string ctx_info;
    qn_string * ctx_list;

    blk_count = qn_stor_rs_block_count(ss);
    ctx_list = calloc(blk_count, sizeof(qn_string));
    if (!ctx_list) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    for (i = 0; i < blk_count; i += 1) {
        blk_info = qn_stor_rs_block_info(ss, i);
        if (!blk_info) {
            // TODO: Set an apprepriate error.
            free(ctx_list);
            return qn_false;
        } // if
        
        chk_offset = qn_json_get_integer(blk_info, "offset", 0);
        blk_size = qn_json_get_integer(blk_info, "bsize", 0);

        if (chk_offset > 0 && blk_size > 0 && chk_offset == blk_size) {
            ctx_list[i] = qn_json_get_string(blk_info, "ctx", NULL);
            if (!ctx_list[i]) {
                // TODO: Set an apprepriate error.
                free(ctx_list);
                return qn_false;
            } // if
        } else {
            // TODO: Set an apprepriate error.
            free(ctx_list);
            return qn_false;
        } // if
    } // for

    ctx_info = qn_str_join_list(",", ctx_list, blk_count);
    free(ctx_list);
    if (!ctx_info) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    // Here the blk_info variable is pointing to the last block.
    host = qn_json_get_string(blk_info, "host", NULL);
    if (!host) {
        // TODO: Set an apprepriate error.
        return qn_false;
    } // if

    url = qn_str_sprintf("%s/mkfile/%ld", qn_str_cstr(host), ss->fsize); // TODO: Use correct directive for large numbers.
    if (!url) {
        // TODO: Set an apprepriate error.
        return qn_false;
    } // if

    if (ext->final_key) {
        encoded_key = qn_str_encode_base64_urlsafe(ext->final_key, strlen(ext->final_key));
        if (!encoded_key) return qn_false;

        url2 = qn_str_sprintf("%s/key/%s", qn_str_cstr(url), qn_str_cstr(encoded_key));
        qn_str_destroy(encoded_key);
        qn_str_destroy(url);
        if (!url2) return qn_false;
        url = url2;
    } // if

    ret = qn_stor_rp_make_file_to_one_piece(stor, ss, url, ctx_info, ext);
    qn_str_destroy(url);
    qn_str_destroy(ctx_info);
    return ret;
}

static qn_bool qn_stor_rp_put_file_in_serial_blocks(qn_storage_ptr stor, qn_stor_rput_session_ptr ss, const char * fname, qn_stor_rput_extra_ptr ext)
{
    qn_bool ret;
    qn_bool skip;
    qn_file_ptr fl;
    qn_json_object_ptr blk_info;
    qn_integer blk_count;
    qn_integer blk_size;
    qn_integer chk_offset;
    int i;
    qn_fsize blk_offset;
    qn_io_reader rdr;

    fl = qn_fl_open(fname, NULL);
    if (!fl) return qn_false;

    memset(&rdr, 0, sizeof(qn_io_reader));
    rdr.user_data = fl;
    rdr.read = (qn_io_read) &qn_fl_read;
    rdr.advance = (qn_io_advance) &qn_fl_advance;

    blk_offset = 0;
    blk_count = qn_stor_rs_block_count(ss);
    for (i = 0; i < blk_count; i += 1) {
        blk_info = qn_stor_rs_block_info(ss, i);
        blk_size = qn_json_get_integer(blk_info, "bsize", 0);
        chk_offset = qn_json_get_integer(blk_info, "offset", 0);

        if (chk_offset > 0 && blk_size > 0 && chk_offset == blk_size) {
            skip = qn_true;
            blk_offset += blk_size;
            continue;
        } // if

        if (skip) {
            if (!qn_fl_seek(fl, blk_offset)) {
                qn_fl_close(fl);
                return qn_false;
            } // if
            skip = qn_false;
        } // if

        ret = qn_stor_rp_put_block(stor, blk_info, &rdr, ext);
        if (!ret) {
            qn_fl_close(fl);
            return qn_false;
        } // if
        blk_offset += blk_size;
    } // for
    return qn_true;
}

qn_bool qn_stor_rp_put_file(qn_storage_ptr stor, qn_stor_rput_session_ptr * ss, const char * fname, qn_stor_rput_extra_ptr ext)
{
    qn_bool ret;
    qn_fl_info_ptr fi;

    if (!*ss) {
        // A new session to put file.
        fi = qn_fl_info_stat(fname);
        if (!fi) return qn_false;

        *ss = qn_stor_rs_create(qn_fl_info_fsize(fi));
        qn_fl_info_destroy(fi);
        if (!*ss) return qn_false;
    } // if

    if (!qn_stor_rp_put_file_in_serial_blocks(stor, *ss, fname, ext)) return qn_false;

    ret = qn_stor_rp_make_file(stor, *ss, ext);
    if (ret) {
        qn_stor_rs_destroy(*ss);
        *ss = NULL;
    } // if
    return ret;
}

#ifdef __cplusplus
}
#endif

