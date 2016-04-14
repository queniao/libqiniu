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

extern qn_json_ptr qn_json_new_object(void);
extern qn_json_ptr qn_json_new_array(void);
extern void qn_json_delete(qn_json_ptr self);

extern qn_bool qn_json_set(qn_json_ptr self, const char * key, qn_json_ptr val);
extern void qn_json_unset(qn_json_ptr self, const char * key);

extern qn_bool qn_json_push(qn_json_ptr self, qn_json_ptr val);
extern void qn_json_pop(qn_json_ptr self);

extern qn_bool qn_json_unshift(qn_json_ptr self, qn_json_ptr val);
extern void qn_json_shift(qn_json_ptr self);

typedef void * qn_json_handle;

extern qn_json_ptr qn_json_get(qn_json_ptr self, qn_json_handle h);

extern qn_bool qn_json_is_object(qn_json_ptr self);
extern qn_bool qn_json_is_array(qn_json_ptr self);
extern qn_bool qn_json_is_string(qn_json_ptr self);
extern qn_bool qn_json_is_integer(qn_json_ptr self);
extern qn_bool qn_json_is_number(qn_json_ptr self);
extern qn_bool qn_json_is_boolean(qn_json_ptr self);
extern qn_bool qn_json_is_null(qn_json_ptr self);

extern qn_string qn_json_cast_to_string(qn_json_ptr self);
extern qn_integer qn_json_cast_to_integer(qn_json_ptr self);
extern qn_number qn_json_cast_to_number(qn_json_ptr self);
extern qn_bool qn_json_cast_to_boolean(qn_json_ptr self);

extern qn_bool qn_json_is_empty(qn_json_ptr self);

typedef int (*qn_json_iterate_func)(void *, char *, int, qn_json_ptr);

extern int qn_json_iterate(qn_json_ptr self, void * data, qn_json_iterate_func itr);

struct _QN_JSON_PARSER;
typedef struct _QN_JSON_PARSER * qn_json_parser_ptr;

extern qn_bool qn_json_new_parser(qn_json_parser_ptr * prs);
extern void qn_json_delete_parser(qn_json_parser_ptr prs);
extern qn_bool qn_json_parse(qn_json_parser_ptr prs, char * buf, size_t * buf_len, qn_json_ptr * self);

struct _QN_JSON_FORMATTER;
typedef struct _QN_JSON_FORMATTER * qn_json_formatter_ptr;

extern qn_bool qn_json_new_formatter(qn_json_formatter_ptr * fmt, qn_json_ptr self);
extern void qn_json_delete_formatter(qn_json_formatter_ptr fmt);
extern qn_bool qn_json_format(qn_json_formatter_ptr fmt, char * buf, size_t * buf_cap);
extern qn_bool qn_json_format_to_string(qn_json_ptr self, char ** str, size_t * str_len);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_H__

