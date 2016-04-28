#ifndef __QN_JSON_H__
#define __QN_JSON_H__

#include "base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define QN_JSON_ITR_RECUR 0
#define QN_JSON_ITR_END 1

struct _QN_JSON;
typedef struct _QN_JSON * qn_json_ptr;

extern qn_json_ptr qn_json_create_object(void);
extern qn_json_ptr qn_json_create_array(void);
extern qn_json_ptr qn_json_create_string(const char * cstr, qn_size cstr_size);
extern qn_json_ptr qn_json_create_integer(qn_integer val);
extern qn_json_ptr qn_json_create_number(qn_number val);
extern qn_json_ptr qn_json_create_boolean(qn_bool val);
extern qn_json_ptr qn_json_create_null(void);
extern void qn_json_destroy(qn_json_ptr self);

extern qn_bool qn_json_set(qn_json_ptr self, const char * key, qn_json_ptr val);
extern void qn_json_unset(qn_json_ptr self, const char * key);

extern qn_bool qn_json_push(qn_json_ptr self, qn_json_ptr val);
extern void qn_json_pop(qn_json_ptr self);

extern qn_bool qn_json_unshift(qn_json_ptr self, qn_json_ptr val);
extern void qn_json_shift(qn_json_ptr self);

extern qn_json_ptr qn_json_get(qn_json_ptr self, const char * key);
extern qn_json_ptr qn_json_get_at(qn_json_ptr self, qn_size n);

extern qn_bool qn_json_is_object(qn_json_ptr self);
extern qn_bool qn_json_is_array(qn_json_ptr self);
extern qn_bool qn_json_is_string(qn_json_ptr self);
extern qn_bool qn_json_is_integer(qn_json_ptr self);
extern qn_bool qn_json_is_number(qn_json_ptr self);
extern qn_bool qn_json_is_boolean(qn_json_ptr self);
extern qn_bool qn_json_is_null(qn_json_ptr self);

extern qn_string * qn_json_to_string(qn_json_ptr self);
extern qn_integer qn_json_to_integer(qn_json_ptr self);
extern qn_number qn_json_to_number(qn_json_ptr self);
extern qn_bool qn_json_to_boolean(qn_json_ptr self);

extern qn_bool qn_json_is_empty(qn_json_ptr self);

typedef int (*qn_json_iterate_func)(void *, char *, int, qn_json_ptr);

extern int qn_json_iterate(qn_json_ptr self, void * data, qn_json_iterate_func itr);

struct _QN_JSON_PARSER;
typedef struct _QN_JSON_PARSER * qn_json_parser_ptr;

extern qn_json_parser_ptr qn_json_prs_create(void);
extern void qn_json_prs_destroy(qn_json_parser_ptr prs);
extern qn_bool qn_json_prs_parse(qn_json_parser_ptr prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_ptr * root_element);

struct _QN_JSON_FORMATTER;
typedef struct _QN_JSON_FORMATTER * qn_json_formatter_ptr;

extern qn_json_formatter_ptr qn_json_fmt_create(void);
extern void qn_json_fmt_destroy(qn_json_formatter_ptr fmt);
extern void qn_json_fmt_reset(qn_json_formatter_ptr fmt);
extern qn_bool qn_json_fmt_format(qn_json_formatter_ptr fmt, qn_json_ptr root_element, const char ** restrict buf, qn_size * restrict buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_H__

