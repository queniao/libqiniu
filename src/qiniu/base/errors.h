#ifndef __QN_ERRORS_H__
#define __QN_ERRORS_H__ 1

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_API extern ssize_t qn_err_format_message(char * buf, size_t buf_size);
QN_API extern const char * qn_err_get_message(void);

// ----

#define qn_err_set_succeed() qn_err_set_succeed_imp(__FILE__, __LINE__)
#define qn_err_set_out_of_memory() qn_err_set_out_of_memory_imp(__FILE__, __LINE__)
#define qn_err_set_try_again() qn_err_set_try_again_imp(__FILE__, __LINE__)
#define qn_err_set_invalid_argument() qn_err_set_invalid_argument_imp(__FILE__, __LINE__)
#define qn_err_set_overflow_upper_bound() qn_err_set_overflow_upper_bound_imp(__FILE__, __LINE__)
#define qn_err_set_overflow_lower_bound() qn_err_set_overflow_lower_bound_imp(__FILE__, __LINE__)
#define qn_err_set_bad_utf8_sequence() qn_err_set_bad_utf8_sequence_imp(__FILE__, __LINE__)
#define qn_err_set_out_of_buffer() qn_err_set_out_of_buffer_imp(__FILE__, __LINE__)
#define qn_err_set_out_of_capacity() qn_err_set_out_of_capacity_imp(__FILE__, __LINE__)
#define qn_err_set_no_such_entry() qn_err_set_no_such_entry_imp(__FILE__, __LINE__)
#define qn_err_set_out_of_range() qn_err_set_out_of_range_imp(__FILE__, __LINE__)

QN_API extern void qn_err_set_succeed_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_out_of_memory_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_try_again_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_invalid_argument_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_overflow_upper_bound_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_overflow_lower_bound_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_bad_utf8_sequence_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_out_of_buffer_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_out_of_capacity_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_no_such_entry_imp(const char * restrict file, int line);
QN_API extern void qn_err_set_out_of_range_imp(const char * restrict file, int line);

#define qn_err_json_set_bad_text_input() qn_err_json_set_bad_text_input_imp(__FILE__, __LINE__)
#define qn_err_json_set_too_many_parsing_levels() qn_err_json_set_too_many_parsing_levels_imp(__FILE__, __LINE__)
#define qn_err_json_set_need_more_text_input() qn_err_json_set_need_more_text_input_imp(__FILE__, __LINE__)
#define qn_err_json_set_modifying_immutable_object() qn_err_json_set_modifying_immutable_object_imp(__FILE__, __LINE__)
#define qn_err_json_set_modifying_immutable_array() qn_err_json_set_modifying_immutable_array_imp(__FILE__, __LINE__)
#define qn_err_json_set_out_of_index() qn_err_json_set_out_of_index_imp(__FILE__, __LINE__)

QN_API extern void qn_err_json_set_bad_text_input_imp(const char * restrict file, int line);
QN_API extern void qn_err_json_set_too_many_parsing_levels_imp(const char * restrict file, int line);
QN_API extern void qn_err_json_set_need_more_text_input_imp(const char * restrict file, int line);
QN_API extern void qn_err_json_set_modifying_immutable_object_imp(const char * restrict file, int line);
QN_API extern void qn_err_json_set_modifying_immutable_array_imp(const char * restrict file, int line);
QN_API extern void qn_err_json_set_out_of_index_imp(const char * restrict file, int line);

#define qn_err_http_set_invalid_header_syntax() qn_err_http_set_invalid_header_syntax_imp(__FILE__, __LINE__)
#define qn_err_http_set_adding_string_field_failed() qn_err_http_set_adding_string_field_failed_imp(__FILE__, __LINE__)
#define qn_err_http_set_adding_file_field_failed() qn_err_http_set_adding_file_field_failed_imp(__FILE__, __LINE__)
#define qn_err_http_set_adding_buffer_field_failed() qn_err_http_set_adding_buffer_field_failed_imp(__FILE__, __LINE__)

QN_API extern void qn_err_http_set_invalid_header_syntax_imp(const char * restrict file, int line);
QN_API extern void qn_err_http_set_adding_string_field_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_http_set_adding_file_field_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_http_set_adding_buffer_field_failed_imp(const char * restrict file, int line);

#define qn_err_fl_set_opening_file_failed() qn_err_fl_set_opening_file_failed_imp(__FILE__, __LINE__)
#define qn_err_fl_set_duplicating_file_failed() qn_err_fl_set_duplicating_file_failed_imp(__FILE__, __LINE__)
#define qn_err_fl_set_reading_file_failed() qn_err_fl_set_reading_file_failed_imp(__FILE__, __LINE__)
#define qn_err_fl_set_seeking_file_failed() qn_err_fl_set_seeking_file_failed_imp(__FILE__, __LINE__)

QN_API extern void qn_err_fl_set_opening_file_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_fl_set_duplicating_file_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_fl_set_reading_file_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_fl_set_seeking_file_failed_imp(const char * restrict file, int line);

#define qn_err_fl_info_set_stating_file_info_failed() qn_err_fl_info_set_stating_file_info_failed_imp(__FILE__, __LINE__)

QN_API extern void qn_err_fl_info_set_stating_file_info_failed_imp(const char * restrict file, int line);

#define qn_err_stor_set_lack_of_authorization_information() qn_err_stor_set_lack_of_authorization_information_imp(__FILE__, __LINE__)
#define qn_err_stor_set_invalid_resumable_session_information() qn_err_stor_set_invalid_resumable_session_information_imp(__FILE__, __LINE__)
#define qn_err_stor_set_invalid_list_result() qn_err_stor_set_invalid_list_result_imp(__FILE__, __LINE__)
#define qn_err_stor_set_upload_aborted_by_filter_pre_callback() qn_err_stor_set_upload_aborted_by_filter_pre_callback_imp(__FILE__, __LINE__)
#define qn_err_stor_set_upload_aborted_by_filter_post_callback() qn_err_stor_set_upload_aborted_by_filter_post_callback_imp(__FILE__, __LINE__)
#define qn_err_stor_set_invalid_chunk_put_result() qn_err_stor_set_invalid_chunk_put_result_imp(__FILE__, __LINE__)
#define qn_err_stor_set_api_return_no_value() qn_err_stor_set_api_return_no_value_imp(__FILE__, __LINE__)
#define qn_err_stor_set_lack_of_block_context() qn_err_stor_set_lack_of_block_context_imp(__FILE__, __LINE__)
#define qn_err_stor_set_lack_of_file_size() qn_err_stor_set_lack_of_file_size_imp(__FILE__, __LINE__)
#define qn_err_stor_set_lack_of_block_info() qn_err_stor_set_lack_of_block_info_imp(__FILE__, __LINE__)
#define qn_err_stor_set_invalid_upload_result() qn_err_stor_set_invalid_upload_result_imp(__FILE__, __LINE__)

QN_API extern void qn_err_stor_set_lack_of_authorization_information_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_invalid_resumable_session_information_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_invalid_list_result_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_upload_aborted_by_filter_pre_callback_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_upload_aborted_by_filter_post_callback_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_invalid_chunk_put_result_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_api_return_no_value_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_lack_of_block_context_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_lack_of_block_info_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_lack_of_file_size_imp(const char * restrict file, int line);
QN_API extern void qn_err_stor_set_invalid_upload_result_imp(const char * restrict file, int line);

#define qn_err_etag_set_initializing_context_failed() qn_err_etag_set_initializing_context_failed_imp(__FILE__, __LINE__)
#define qn_err_etag_set_updating_context_failed() qn_err_etag_set_updating_context_failed_imp(__FILE__, __LINE__)
#define qn_err_etag_set_initializing_block_failed() qn_err_etag_set_initializing_block_failed_imp(__FILE__, __LINE__)
#define qn_err_etag_set_updating_block_failed() qn_err_etag_set_updating_block_failed_imp(__FILE__, __LINE__)
#define qn_err_etag_set_making_digest_failed() qn_err_etag_set_making_digest_failed_imp(__FILE__, __LINE__)

QN_API extern void qn_err_etag_set_initializing_context_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_etag_set_updating_context_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_etag_set_initializing_block_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_etag_set_updating_block_failed_imp(const char * restrict file, int line);
QN_API extern void qn_err_etag_set_making_digest_failed_imp(const char * restrict file, int line);

#define qn_err_easy_set_invalid_uptoken() qn_err_easy_set_invalid_uptoken_imp(__FILE__, __LINE__)
#define qn_err_easy_set_invalid_put_policy() qn_err_easy_set_invalid_put_policy_imp(__FILE__, __LINE__)

QN_API extern void qn_err_easy_set_invalid_uptoken_imp(const char * restrict file, int line);
QN_API extern void qn_err_easy_set_invalid_put_policy_imp(const char * restrict file, int line);

// ----

QN_API extern qn_bool qn_err_is_succeed(void);
QN_API extern qn_bool qn_err_is_out_of_memory(void);
QN_API extern qn_bool qn_err_is_try_again(void);
QN_API extern qn_bool qn_err_is_invalid_argument(void);
QN_API extern qn_bool qn_err_is_overflow_upper_bound(void);
QN_API extern qn_bool qn_err_is_overflow_lower_bound(void);
QN_API extern qn_bool qn_err_is_bad_utf8_sequence(void);
QN_API extern qn_bool qn_err_is_out_of_buffer(void);
QN_API extern qn_bool qn_err_is_out_of_capacity(void);
QN_API extern qn_bool qn_err_is_no_such_entry(void);
QN_API extern qn_bool qn_err_is_out_of_range(void);

QN_API extern qn_bool qn_err_json_is_bad_text_input(void);
QN_API extern qn_bool qn_err_json_is_too_many_parsing_levels(void);
QN_API extern qn_bool qn_err_json_is_need_more_text_input(void);
QN_API extern qn_bool qn_err_json_is_modifying_immutable_object(void);
QN_API extern qn_bool qn_err_json_is_modifying_immutable_array(void);
QN_API extern qn_bool qn_err_json_is_out_of_index(void);

QN_API extern qn_bool qn_err_http_is_invalid_header_syntax(void);
QN_API extern qn_bool qn_err_http_is_adding_string_field_failed(void);
QN_API extern qn_bool qn_err_http_is_adding_file_field_failed(void);
QN_API extern qn_bool qn_err_http_is_adding_buffer_field_failed(void);

QN_API extern qn_bool qn_err_fl_is_opening_file_failed(void);
QN_API extern qn_bool qn_err_fl_is_duplicating_file_failed(void);
QN_API extern qn_bool qn_err_fl_is_reading_file_failed(void);
QN_API extern qn_bool qn_err_fl_is_seeking_file_failed(void);

QN_API extern qn_bool qn_err_fl_info_is_stating_file_info_failed(void);

QN_API extern qn_bool qn_err_stor_is_lack_of_authorization_information(void);
QN_API extern qn_bool qn_err_stor_is_invalid_resumable_session_information(void);
QN_API extern qn_bool qn_err_stor_is_invalid_list_result(void);
QN_API extern qn_bool qn_err_stor_is_upload_aborted_by_filter_pre_callback(void);
QN_API extern qn_bool qn_err_stor_is_upload_aborted_by_filter_post_callback(void);
QN_API extern qn_bool qn_err_stor_is_invalid_chunk_put_result(void);
QN_API extern qn_bool qn_err_stor_is_api_return_no_value(void);
QN_API extern qn_bool qn_err_stor_is_lack_of_block_context(void);
QN_API extern qn_bool qn_err_stor_is_lack_of_block_info(void);
QN_API extern qn_bool qn_err_stor_is_lack_of_file_size(void);
QN_API extern qn_bool qn_err_stor_is_invalid_upload_result(void);

QN_API extern qn_bool qn_err_etag_is_initializing_context_failed(void);
QN_API extern qn_bool qn_err_etag_is_updating_context_failed(void);
QN_API extern qn_bool qn_err_etag_is_initializing_block_failed(void);
QN_API extern qn_bool qn_err_etag_is_updating_block_failed(void);
QN_API extern qn_bool qn_err_etag_is_making_digest_failed(void);

QN_API extern qn_bool qn_err_easy_is_invalid_uptoken(void);
QN_API extern qn_bool qn_err_easy_is_invalid_put_policy(void);

#ifdef __cplusplus
}
#endif

#endif // __QN_ERRORS_H__
