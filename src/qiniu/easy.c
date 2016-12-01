#include "qiniu/base/errors.h"
#include "qiniu/os/file.h"
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
        unsigned int detect_fsize:1; // Detect the size of the local file automatically.

        // Specifies a const-volatile boolean variable to check if need to abort or not.
        const volatile qn_bool * abort;

        size_t min_resumable_fsize; // The minimum file size to enable resumable uploading.
                                    // It must be less than 500MB due to the limit set by the server, and larger than
                                    // 4MB since uploading a small file in multiple HTTP round trips is not efficient.
                                    // If set less than 4MB, the default size is 10MB.

        qn_uint32 fcrc32;           // The CRC-32 of the local file provided by the caller.
        qn_fsize fsize;             // The size of the local file provided by the caller.
        qn_io_reader_itf rdr;       // A customized data reader provided by the caller.

        qn_stor_rput_session_ptr * rput_ss;
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
        free(pe);
    } // if
}

QN_API void qn_easy_pe_reset(qn_easy_put_extra_ptr restrict pe)
{
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

QN_API void qn_easy_pe_set_source_reader(qn_easy_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize, qn_bool detect_fsize)
{
    pe->put_ctrl.rdr = rdr;
    pe->put_ctrl.fsize = fsize;
    pe->put_ctrl.detect_fsize = (detect_fsize) ? 1 : 0;
}

QN_API void qn_easy_pe_set_resumable_put_session(qn_easy_put_extra_ptr restrict pe, qn_stor_rput_session_ptr * restrict rput_ss)
{
    pe->put_ctrl.rput_ss = rput_ss;
}

typedef struct _QN_EASY
{
    qn_storage_ptr stor;
} qn_easy;

QN_API qn_easy_ptr qn_easy_create(void)
{
    qn_easy_ptr new_easy = calloc(1, sizeof(qn_easy));
    if (!new_easy) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_easy->stor = qn_stor_create();
    if (!new_easy->stor) {
        free(new_easy);
        return NULL;
    } // if
    return new_easy;
}

QN_API void qn_easy_destroy(qn_easy_ptr restrict easy)
{
    if (easy) {
        qn_stor_destroy(easy->stor);
        free(easy);
    } // if
}

#define QN_EASY_MB_UINT (2 << 20)

static void qn_easy_init_put_extra(qn_easy_put_extra_ptr ext, qn_easy_put_extra_ptr real_ext)
{
    memset(real_ext, 0, sizeof(qn_easy_put_extra_st));
    *real_ext = *ext;

    if (real_ext->put_ctrl.min_resumable_fsize < (4L * QN_EASY_MB_UINT)) {
        real_ext->put_ctrl.min_resumable_fsize = (10L * QN_EASY_MB_UINT);
    } else if (real_ext->put_ctrl.min_resumable_fsize > (500L * QN_EASY_MB_UINT)) {
        real_ext->put_ctrl.min_resumable_fsize = (500L * QN_EASY_MB_UINT);
    } // if
}

static qn_io_reader_itf qn_easy_create_reader(const char * restrict fname, qn_easy_put_extra_ptr real_ext)
{
    qn_file_ptr fl = NULL;
    qn_io_reader_itf io_rdr;
    qn_reader_ptr rdr;
    qn_rdr_pos filter_num = 0;

    if (real_ext->put_ctrl.rdr) {
        io_rdr = real_ext->put_ctrl.rdr;
    } else {
        if (!real_ext->put_ctrl.rdr) {
            fl = qn_fl_open(fname, NULL);
            if (! fl) return NULL;
        } // if
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
    qn_stor_pe_set_source_reader(put_ext, io_rdr, ext->put_ctrl.fsize, (ext->put_ctrl.detect_fsize) ? qn_true : qn_false);

    ret = qn_stor_put_file(easy->stor, uptoken, fname, put_ext);
    qn_stor_pe_destroy(put_ext);
    return ret;
}

static qn_json_object_ptr qn_easy_put_file_in_many_pieces(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_io_reader_itf restrict io_rdr, qn_easy_put_extra_ptr restrict ext)
{
    qn_json_object_ptr ret;
    qn_stor_rput_extra_ptr rput_ext;

    if (! (rput_ext = qn_stor_rpe_create())) return NULL;
    qn_stor_rpe_set_source_reader(rput_ext, io_rdr, ext->put_ctrl.fsize, (ext->put_ctrl.detect_fsize) ? qn_true : qn_false);

    ret = qn_stor_rp_put_file(easy->stor, uptoken, ext->put_ctrl.rput_ss, fname, rput_ext);
    qn_stor_rpe_destroy(rput_ext);
    return ret;
}

QN_API qn_json_object_ptr qn_easy_put_file(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_easy_put_extra_ptr restrict ext)
{
    qn_fl_info_ptr fi;
    qn_io_reader_itf io_rdr;
    qn_json_object_ptr put_ret;
    qn_easy_put_extra_st real_ext;

    qn_easy_init_put_extra(ext, &real_ext);
    
    if (real_ext.put_ctrl.fsize == 0 && real_ext.put_ctrl.detect_fsize) {
        fi = qn_fl_info_stat(fname);
        if (!fi) return NULL;

        real_ext.put_ctrl.fsize = qn_fl_info_fsize(fi);
        qn_fl_info_destroy(fi);
    } // if

    // TODO: Implement region selection.

    io_rdr = qn_easy_create_reader(fname, &real_ext);
    if (! io_rdr) return NULL;

    if (real_ext.put_ctrl.fsize <= real_ext.put_ctrl.min_resumable_fsize) {
        put_ret = qn_easy_put_file_in_one_piece(easy, uptoken, fname, io_rdr, &real_ext);
    } else {
        put_ret = qn_easy_put_file_in_many_pieces(easy, uptoken, fname, io_rdr, &real_ext);
    } // if
    if (io_rdr != real_ext.put_ctrl.rdr) qn_io_close(io_rdr);

    return put_ret;
}

#ifdef __cplusplus
}
#endif

