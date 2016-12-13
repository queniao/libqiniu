#include "qiniu/base/errors.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/os/file.h"
#include "qiniu/region.h"
#include "qiniu/storage.h"

#include "qiniu/easy.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_EASY_PUT_EXTRA
{
    struct {
        const char * final_key;     // Final key of the file in the bucket.
        const char * owner_desc;    // The owner description of the file, used for business identificaiton.
    } attr;

    struct {
        unsigned int check_crc32:1;  // Calculate the CRC-32 checksum locally to verify the uploaded content.
        unsigned int check_md5:1;    // Calculate the MD5 checksum locally to verify the uploaded content.
        unsigned int check_qetag:1;  // Calculate the Qiniu-ETAG checksum locally to verify the uploaded content.

        unsigned int extern_ss:1;    // Use an external storage session object.

        // Specifies a const-volatile boolean variable to check if need to abort or not.
        const volatile qn_bool * abort;

        size_t min_resumable_fsize; // The minimum file size to enable resumable uploading.
                                    // It must be less than 500MB due to the limit set by the server, and larger than
                                    // 4MB since uploading a small file in multiple HTTP round trips is not efficient.
                                    // If set less than 4MB, the default size is 10MB.

        qn_uint32 fcrc32;           // The CRC-32 of the local file provided by the caller.
        qn_fsize fsize;             // The size of the local file provided by the caller.
        qn_io_reader_itf rdr;       // A customized data reader provided by the caller.

        qn_stor_rput_session_ptr rput_ss;

        qn_rgn_entry_ptr rgn_entry;
        qn_rgn_host_ptr rgn_host;
    } put_ctrl;
} qn_easy_put_extra_st;

QN_API qn_easy_put_extra_ptr qn_easy_pe_create(void)
{
    qn_easy_put_extra_ptr new_pe = calloc(1, sizeof(qn_easy_put_extra_st));
    if (! new_pe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_pe;
}

QN_API void qn_easy_pe_destroy(qn_easy_put_extra_ptr restrict pe)
{
    if (pe) {
        if (! pe->put_ctrl.extern_ss && pe->put_ctrl.rput_ss) qn_stor_rs_destroy(pe->put_ctrl.rput_ss);
        free(pe);
    } // if
}

QN_API void qn_easy_pe_reset(qn_easy_put_extra_ptr restrict pe)
{
    if (! pe->put_ctrl.extern_ss && pe->put_ctrl.rput_ss) qn_stor_rs_destroy(pe->put_ctrl.rput_ss);
    memset(pe, 0, sizeof(qn_easy_put_extra_st));
}

QN_API void qn_easy_pe_set_final_key(qn_easy_put_extra_ptr restrict pe, const char * restrict key)
{
    pe->attr.final_key = key;
}

QN_API void qn_easy_pe_set_owner_description(qn_easy_put_extra_ptr restrict pe, const char * restrict desc)
{
    pe->attr.owner_desc = desc;
}

QN_API void qn_easy_pe_set_crc32_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check)
{
    pe->put_ctrl.check_crc32 = (check) ? 1 : 0;
}

QN_API void qn_easy_pe_set_md5_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check)
{
    pe->put_ctrl.check_md5 = (check) ? 1 : 0;
}

QN_API void qn_easy_pe_set_qetag_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check)
{
    pe->put_ctrl.check_qetag = (check) ? 1 : 0;
}

QN_API void qn_easy_pe_set_abort_variable(qn_easy_put_extra_ptr restrict pe, const volatile qn_bool * abort)
{
    pe->put_ctrl.abort = abort;
}

QN_API void qn_easy_pe_set_min_resumable_fsize(qn_easy_put_extra_ptr restrict pe, size_t fsize)
{
    pe->put_ctrl.min_resumable_fsize = fsize;
}

QN_API void qn_easy_pe_set_local_crc32(qn_easy_put_extra_ptr restrict pe, qn_uint32 crc32)
{
    pe->put_ctrl.fcrc32 = crc32;
}

QN_API void qn_easy_pe_set_source_reader(qn_easy_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize)
{
    pe->put_ctrl.rdr = rdr;
    pe->put_ctrl.fsize = fsize;
}

QN_API void qn_easy_pe_set_rput_session(qn_easy_put_extra_ptr restrict pe, qn_stor_rput_session_ptr restrict rput_ss)
{
    pe->put_ctrl.extern_ss = (rput_ss) ? 1 : 0;
    pe->put_ctrl.rput_ss = rput_ss;
}

QN_API qn_stor_rput_session_ptr qn_easy_pe_get_rput_session(qn_easy_put_extra_ptr restrict pe)
{
    return pe->put_ctrl.rput_ss;
}

QN_API void qn_easy_pe_set_region_host(qn_easy_put_extra_ptr restrict pe, qn_rgn_host_ptr restrict rgn_host)
{
    pe->put_ctrl.rgn_host = rgn_host;
}

QN_API void qn_easy_pe_set_region_entry(qn_easy_put_extra_ptr restrict pe, qn_rgn_entry_ptr restrict rgn_entry)
{
    pe->put_ctrl.rgn_entry = rgn_entry;
}

// ----

typedef struct _QN_EASY
{
    qn_storage_ptr stor;
    qn_json_parser_ptr json_prs;
    qn_rgn_service_ptr rgn_svc;
    qn_rgn_table_ptr rgn_tbl;
} qn_easy_st;

QN_API qn_easy_ptr qn_easy_create(void)
{
    qn_easy_ptr new_easy = calloc(1, sizeof(qn_easy_st));
    if (! new_easy) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_easy->stor = qn_stor_create();
    if (! new_easy->stor) {
        free(new_easy);
        return NULL;
    } // if
    return new_easy;
}

QN_API void qn_easy_destroy(qn_easy_ptr restrict easy)
{
    if (easy) {
        if (easy->rgn_tbl) qn_rgn_tbl_destroy(easy->rgn_tbl);
        if (easy->rgn_svc) qn_rgn_svc_destroy(easy->rgn_svc);
        if (easy->json_prs) qn_json_prs_destroy(easy->json_prs);
        qn_stor_destroy(easy->stor);
        free(easy);
    } // if
}

#define QN_EASY_MB_UNIT (1 << 20)

static void qn_easy_init_put_extra(qn_easy_put_extra_ptr ext, qn_easy_put_extra_ptr real_ext)
{
    memset(real_ext, 0, sizeof(qn_easy_put_extra_st));
    *real_ext = *ext;

    if (real_ext->put_ctrl.min_resumable_fsize < (4L * QN_EASY_MB_UNIT)) {
        real_ext->put_ctrl.min_resumable_fsize = (10L * QN_EASY_MB_UNIT);
    } else if (real_ext->put_ctrl.min_resumable_fsize > (500L * QN_EASY_MB_UNIT)) {
        real_ext->put_ctrl.min_resumable_fsize = (500L * QN_EASY_MB_UNIT);
    } // if
}

static qn_io_reader_itf qn_easy_create_put_reader(const char * restrict fname, qn_easy_put_extra_ptr real_ext)
{
    qn_file_ptr fl = NULL;
    qn_fl_info_ptr fi;
    qn_io_reader_itf io_rdr;
    qn_reader_ptr rdr;
    qn_rdr_pos filter_num = 0;

    if (real_ext->put_ctrl.rdr) {
        io_rdr = real_ext->put_ctrl.rdr;
    } else {
        fi = qn_fl_info_stat(fname);
        if (! fi) return NULL;

        real_ext->put_ctrl.fsize = qn_fl_info_fsize(fi);
        qn_fl_info_destroy(fi);

        fl = qn_fl_open(fname, NULL);
        if (! fl) return NULL;

        io_rdr = qn_fl_to_io_reader(fl);
    } // if

/*
    if (real_ext->put_ctrl.check_crc32) filter_num += 1;
    if (real_ext->put_ctrl.check_md5) filter_num += 1;
    if (real_ext->put_ctrl.check_qetag) filter_num += 1;
    if (real_ext->put_ctrl.abort) filter_num += 1;
*/

    if (filter_num == 0) return io_rdr;

    rdr = qn_rdr_create(io_rdr, filter_num);
    if (! rdr) {
        if (fl) qn_fl_close(fl);
        return NULL;
    } // if

    // TODO: Implement filters.

    return qn_rdr_to_io_reader(rdr);
}

static qn_json_object_ptr qn_easy_put_file_in_one_piece(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_io_reader_itf restrict io_rdr, qn_easy_put_extra_ptr restrict ext)
{
    qn_json_object_ptr ret;
    qn_stor_put_extra_ptr put_ext;

    if (! (put_ext = qn_stor_pe_create())) return NULL;

    qn_stor_pe_set_final_key(put_ext, ext->attr.final_key);
    qn_stor_pe_set_source_reader(put_ext, io_rdr, ext->put_ctrl.fsize);

    ret = qn_stor_put_file(easy->stor, uptoken, fname, put_ext);
    qn_stor_pe_destroy(put_ext);
    return ret;
}

static qn_json_object_ptr qn_easy_put_file_in_many_pieces(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_io_reader_itf restrict io_rdr, qn_easy_put_extra_ptr restrict ext)
{
    qn_json_object_ptr ret;
    qn_stor_rput_extra_ptr rput_ext;

    if (! (rput_ext = qn_stor_rpe_create())) return NULL;

    qn_stor_rpe_set_final_key(rput_ext, ext->attr.final_key);
    qn_stor_rpe_set_source_reader(rput_ext, io_rdr, ext->put_ctrl.fsize);

    ret = qn_stor_rp_put_file(easy->stor, uptoken, &ext->put_ctrl.rput_ss, fname, rput_ext);
    qn_stor_rpe_destroy(rput_ext);
    return ret;
}

static qn_rgn_host_ptr qn_easy_select_putting_region_host(qn_easy_ptr restrict easy, const char * restrict uptoken, qn_json_object_ptr * restrict pp, qn_easy_put_extra_ptr restrict ext)
{
    const char * pos;
    size_t pp_size;
    qn_bool ret;
    qn_string access_key;
    qn_string bucket;
    qn_string scope;
    qn_string pp_str;
    qn_rgn_auth rgn_auth;
    qn_region_ptr rgn;

    if (! easy->rgn_svc) {
        easy->rgn_svc = qn_rgn_svc_create();
        if (! easy->rgn_svc) return NULL;
    } // if

    if (! easy->rgn_tbl) {
        easy->rgn_tbl = qn_rgn_tbl_create();
        if (! easy->rgn_tbl) return NULL;
    } // if

    pos = qn_str_find_char(uptoken, ':');
    if (! pos) {
        // TODO: Set an appropriate error
        return NULL;
    } // if

    access_key = qn_cs_clone(uptoken, pos - uptoken);

    pos = qn_str_find_char(pos + 1, ':');
    if (! pos) {
        qn_str_destroy(access_key);
        // TODO: Set an appropriate error
        return NULL;
    } // if

    if (!*pp) {
        if (! easy->json_prs) {
            easy->json_prs = qn_json_prs_create();
            if (! easy->json_prs) {
                qn_str_destroy(access_key);
                return NULL;
            } // if
        } // if

        pp_str = qn_cs_decode_base64_urlsafe(pos + 1, posix_strlen(uptoken) - (pos + 1 - uptoken));
        if (! pp_str) {
            qn_str_destroy(access_key);
            // TODO: Set an appropriate error
            return NULL;
        } // if

        pp_size = qn_str_size(pp_str);
        ret = qn_json_prs_parse_object(easy->json_prs, qn_str_cstr(pp_str), &pp_size, pp);
        qn_str_destroy(pp_str);
        if (! ret) {
            qn_str_destroy(access_key);
            // TODO: Set an appropriate error
            return NULL;
        } // if
    } // if

    scope = qn_json_get_string(*pp, "scope", NULL);
    if (! scope) {
        qn_str_destroy(access_key);
        // TODO: Set an appropriate error
        return NULL;
    } // if

    pos = qn_str_find_char(qn_str_cstr(scope), ':');
    if (pos) {
        bucket = qn_cs_clone(qn_str_cstr(scope), pos - qn_str_cstr(scope));
    } else {
        bucket = qn_cs_duplicate(qn_str_cstr(scope));
    } // if

    memset(&rgn_auth, 0, sizeof(rgn_auth));
    rgn_auth.server_end.access_key = qn_str_cstr(access_key);

    ret = qn_rgn_svc_grab_bucket_region(easy->rgn_svc, &rgn_auth, bucket, easy->rgn_tbl);
    qn_str_destroy(access_key);
    if (! ret) return NULL;

    rgn = qn_rgn_tbl_get_region(easy->rgn_tbl, bucket);
    qn_str_destroy(bucket);
    if (! rgn) return NULL;

    return qn_rgn_get_up_host(rgn);
}

QN_API qn_json_object_ptr qn_easy_put_file(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_easy_put_extra_ptr restrict ext)
{
    int i;
    qn_io_reader_itf io_rdr;
    qn_json_object_ptr put_ret;
    qn_easy_put_extra_st real_ext;
    qn_rgn_host_ptr rgn_host;
    qn_json_object_ptr pp = NULL;

    qn_easy_init_put_extra(ext, &real_ext);
    
    // ---- Make some information checks and choose some strategies.
    // -- Select an upload region.
    if (! real_ext.put_ctrl.rgn_entry) {
        if (real_ext.put_ctrl.rgn_host) {
            rgn_host = real_ext.put_ctrl.rgn_host;
        } else {
            rgn_host = qn_easy_select_putting_region_host(easy, uptoken, &pp, &real_ext);
            if (! rgn_host) return NULL;
        } // if
    } // if

    if (pp) qn_json_destroy_object(pp);

    io_rdr = qn_easy_create_put_reader(fname, &real_ext);
    if (! io_rdr) return NULL;

    if (real_ext.put_ctrl.rgn_entry) {
        if (real_ext.put_ctrl.fsize <= real_ext.put_ctrl.min_resumable_fsize) {
            put_ret = qn_easy_put_file_in_one_piece(easy, uptoken, fname, io_rdr, &real_ext);
        } else {
            put_ret = qn_easy_put_file_in_many_pieces(easy, uptoken, fname, io_rdr, &real_ext);
        } // if
    } else {
        for (i = 0, put_ret = NULL; i < qn_rgn_host_entry_count(rgn_host) && ! put_ret; i += 1) {
            real_ext.put_ctrl.rgn_entry = qn_rgn_host_get_entry(rgn_host, i);

            if (real_ext.put_ctrl.fsize <= real_ext.put_ctrl.min_resumable_fsize) {
                put_ret = qn_easy_put_file_in_one_piece(easy, uptoken, fname, io_rdr, &real_ext);
            } else {
                put_ret = qn_easy_put_file_in_many_pieces(easy, uptoken, fname, io_rdr, &real_ext);
            } // if
        } // for
    } // if
    if (io_rdr != real_ext.put_ctrl.rdr) qn_io_close(io_rdr);

    return put_ret;
}

#ifdef __cplusplus
}
#endif

