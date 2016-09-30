#ifndef __QN_ERRORS_H__
#define __QN_ERRORS_H__ 1

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_API extern const char * qn_err_get_message(void);

QN_API extern void qn_err_set_succeed(void);
QN_API extern void qn_err_set_no_enough_memory(void);
QN_API extern void qn_err_set_try_again(void);
QN_API extern void qn_err_set_invalid_argument(void);
QN_API extern void qn_err_set_overflow_upper_bound(void);
QN_API extern void qn_err_set_overflow_lower_bound(void);
QN_API extern void qn_err_set_bad_utf8_sequence(void);
QN_API extern void qn_err_set_no_enough_buffer(void);

QN_API extern void qn_err_json_set_bad_text_input(void);
QN_API extern void qn_err_json_set_too_many_parsing_levels(void);
QN_API extern void qn_err_json_set_need_more_text_input(void);

QN_API extern void qn_err_http_set_invalid_header_syntax(void);
QN_API extern void qn_err_http_set_adding_string_field_failed(void);
QN_API extern void qn_err_http_set_adding_file_field_failed(void);
QN_API extern void qn_err_http_set_adding_buffer_field_failed(void);

QN_API extern void qn_err_fl_set_opening_file_failed(void);
QN_API extern void qn_err_fl_set_duplicating_file_failed(void);
QN_API extern void qn_err_fl_set_reading_file_failed(void);
QN_API extern void qn_err_fl_set_seeking_file_failed(void);

QN_API extern void qn_err_fl_info_set_stating_file_info_failed(void);

QN_API extern void qn_err_stor_set_lack_of_authorization_information(void);
QN_API extern void qn_err_stor_set_invalid_resumable_session_information(void);
QN_API extern void qn_err_stor_set_invalid_list_result(void);
QN_API extern void qn_err_stor_set_putting_aborted_by_data_checker_pre_callback(void);
QN_API extern void qn_err_stor_set_putting_aborted_by_data_checker_post_callback(void);
QN_API extern void qn_err_stor_set_invalid_chunk_put_result(void);

// ----

QN_API extern qn_bool qn_err_is_succeed(void);
QN_API extern qn_bool qn_err_is_no_enough_memory(void);
QN_API extern qn_bool qn_err_is_try_again(void);
QN_API extern qn_bool qn_err_is_invalid_argument(void);
QN_API extern qn_bool qn_err_is_overflow_upper_bound(void);
QN_API extern qn_bool qn_err_is_overflow_lower_bound(void);
QN_API extern qn_bool qn_err_is_bad_utf8_sequence(void);
QN_API extern qn_bool qn_err_is_no_enough_buffer(void);

QN_API extern qn_bool qn_err_json_is_bad_text_input(void);
QN_API extern qn_bool qn_err_json_is_too_many_parsing_levels(void);
QN_API extern qn_bool qn_err_json_is_need_more_text_input(void);

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
QN_API extern qn_bool qn_err_stor_is_putting_aborted_by_data_checker_pre_callback(void);
QN_API extern qn_bool qn_err_stor_is_putting_aborted_by_data_checker_post_callback(void);
QN_API extern qn_bool qn_err_stor_is_invalid_chunk_put_result(void);

#ifdef __cplusplus
}
#endif

#endif // __QN_ERRORS_H__
