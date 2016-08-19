#ifndef __QN_ERRORS_H__
#define __QN_ERRORS_H__ 1

#include "qiniu/base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

const char * qn_err_get_message(void);

extern void qn_err_set_succeed(void);
extern void qn_err_set_no_enough_memory(void);
extern void qn_err_set_try_again(void);
extern void qn_err_set_invalid_argument(void);
extern void qn_err_set_overflow_upper_bound(void);
extern void qn_err_set_overflow_lower_bound(void);
extern void qn_err_set_bad_utf8_sequence(void);
extern void qn_err_set_no_enough_buffer(void);

extern void qn_err_json_set_bad_text_input(void);
extern void qn_err_json_set_too_many_parsing_levels(void);

extern void qn_err_http_set_invalid_header_syntax(void);

extern void qn_err_etag_set_initializing_context_failed(void);
extern void qn_err_etag_set_updating_context_failed(void);
extern void qn_err_etag_set_initializing_block_failed(void);
extern void qn_err_etag_set_updating_block_failed(void);
extern void qn_err_etag_set_making_digest_failed(void);

extern qn_bool qn_err_is_succeed(void);
extern qn_bool qn_err_is_no_enough_memory(void);
extern qn_bool qn_err_is_try_again(void);
extern qn_bool qn_err_is_invalid_argument(void);
extern qn_bool qn_err_is_overflow_upper_bound(void);
extern qn_bool qn_err_is_overflow_lower_bound(void);
extern qn_bool qn_err_is_bad_utf8_sequence(void);
extern qn_bool qn_err_is_no_enough_buffer(void);

extern qn_bool qn_err_json_is_bad_text_input(void);
extern qn_bool qn_err_json_is_too_many_pasring_levels(void);

extern qn_bool qn_err_http_is_invalid_header_syntax(void);

extern qn_bool qn_err_etag_is_initializing_context_failed(void);
extern qn_bool qn_err_etag_is_updating_context_failed(void);
extern qn_bool qn_err_etag_is_initializing_block_failed(void);
extern qn_bool qn_err_etag_is_updating_block_failed(void);
extern qn_bool qn_err_etag_is_making_digest_failed(void);

#ifdef __cplusplus
}
#endif

#endif // __QN_ERRORS_H__
