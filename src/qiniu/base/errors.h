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

extern void qn_err_json_set_bad_text_input(void);
extern void qn_err_json_set_too_many_parsing_levels(void);

extern qn_bool qn_err_is_succeed(void);
extern qn_bool qn_err_is_no_enough_memory(void);
extern qn_bool qn_err_is_try_again(void);
extern qn_bool qn_err_is_invalid_argument(void);
extern qn_bool qn_err_is_overflow_upper_bound(void);
extern qn_bool qn_err_is_overflow_lower_bound(void);
extern qn_bool qn_err_is_bad_utf8_sequence(void);

extern qn_bool qn_err_json_is_bad_text_input(void);
extern qn_bool qn_err_json_is_too_many_pasring_levels(void);

#ifdef __cplusplus
}
#endif

#endif // __QN_ERRORS_H__
