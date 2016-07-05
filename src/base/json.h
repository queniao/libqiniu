#ifndef __QN_JSON_H__
#define __QN_JSON_H__

#include <string.h>

#include "base/basic_types.h"
#include "base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
extern qn_json_ptr qn_json_pick(qn_json_ptr self, qn_size n);

extern qn_bool qn_json_is_object(qn_json_ptr self);
extern qn_bool qn_json_is_array(qn_json_ptr self);
extern qn_bool qn_json_is_string(qn_json_ptr self);
extern qn_bool qn_json_is_integer(qn_json_ptr self);
extern qn_bool qn_json_is_number(qn_json_ptr self);
extern qn_bool qn_json_is_boolean(qn_json_ptr self);
extern qn_bool qn_json_is_null(qn_json_ptr self);

extern qn_string qn_json_to_string(qn_json_ptr self);
extern qn_integer qn_json_to_integer(qn_json_ptr self);
extern qn_number qn_json_to_number(qn_json_ptr self);
extern qn_bool qn_json_to_boolean(qn_json_ptr self);

extern qn_size qn_json_size(qn_json_ptr self);
extern qn_string qn_json_key(qn_json_ptr self);
extern qn_bool qn_json_is_empty(qn_json_ptr self);

struct _QN_JSON_ITERATOR;
typedef struct _QN_JSON_ITERATOR * qn_json_iterator_ptr;

extern qn_json_iterator_ptr qn_json_itr_create(void);
extern void qn_json_itr_destroy(qn_json_iterator_ptr self);
extern void qn_json_itr_reset(qn_json_iterator_ptr self);
extern void qn_json_itr_rewind(qn_json_iterator_ptr self);
extern int qn_json_itr_count(qn_json_iterator_ptr self);

extern qn_bool qn_json_itr_push(qn_json_iterator_ptr self, qn_json_ptr owner);
extern void qn_json_itr_pop(qn_json_iterator_ptr self);
extern qn_json_ptr qn_json_itr_top(qn_json_iterator_ptr self);
extern qn_json_ptr qn_json_itr_next_child(qn_json_iterator_ptr self);
extern qn_json_ptr qn_json_itr_current_child(qn_json_iterator_ptr self);

struct _QN_JSON_PARSER;
typedef struct _QN_JSON_PARSER * qn_json_parser_ptr;

extern qn_json_parser_ptr qn_json_prs_create(void);
extern void qn_json_prs_destroy(qn_json_parser_ptr prs);
extern void qn_json_prs_reset(qn_json_parser_ptr prs);
extern qn_bool qn_json_prs_parse(qn_json_parser_ptr prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_ptr * root_element);

struct _QN_JSON_FORMATTER;
typedef struct _QN_JSON_FORMATTER * qn_json_formatter_ptr;

extern qn_json_formatter_ptr qn_json_fmt_create(void);
extern void qn_json_fmt_destroy(qn_json_formatter_ptr fmt);
extern qn_bool qn_json_fmt_format(qn_json_formatter_ptr fmt, qn_json_ptr root_element, char * restrict buf, qn_size * restrict buf_size);
extern qn_string qn_json_format_to_string(qn_json_ptr root_element);

// ---- Setting Functions ----

extern qn_bool qn_json_set_parsing_max_levels(qn_size count);

// ---- Wrapper Functions ----

static inline qn_bool qn_json_set_string(qn_json_ptr self, const char * key, qn_string val)
{
    qn_json_ptr elem = qn_json_create_string(qn_str_cstr(val), qn_str_size(val));
    if (!elem) return qn_false;
    return qn_json_set(self, key, elem);
} // qn_json_set_string

static inline qn_bool qn_json_set_string_raw(qn_json_ptr self, const char * key, const char * val)
{
    qn_json_ptr elem = qn_json_create_string(val, strlen(val));
    if (!elem) return qn_false;
    return qn_json_set(self, key, elem);
} // qn_json_set_string_raw

static inline  qn_bool qn_json_set_integer(qn_json_ptr self, const char * key, qn_integer val)
{
    qn_json_ptr elem = qn_json_create_integer(val);
    if (!elem) return qn_false;
    return qn_json_set(self, key, elem);
} // qn_json_set_integer

static inline qn_bool qn_json_set_number(qn_json_ptr self, const char * key, qn_number val)
{
    qn_json_ptr elem = qn_json_create_number(val);
    if (!elem) return qn_false;
    return qn_json_set(self, key, elem);
} // qn_json_set_number

static inline qn_bool qn_json_set_boolean(qn_json_ptr self, const char * key, qn_bool val)
{
    qn_json_ptr elem = qn_json_create_boolean(val);
    if (!elem) return qn_false;
    return qn_json_set(self, key, elem);
} // qn_json_set_boolean

static inline qn_bool qn_json_set_null(qn_json_ptr self, const char * key)
{
    qn_json_ptr elem = qn_json_create_null();
    if (!elem) return qn_false;
    return qn_json_set(self, key, elem);
} // qn_json_set_null

static inline qn_bool qn_json_push_string(qn_json_ptr self, qn_string val)
{
    qn_json_ptr elem = qn_json_create_string(qn_str_cstr(val), qn_str_size(val));
    if (!elem) return qn_false;
    return qn_json_push(self, elem);
} // qn_json_push_string

static inline qn_bool qn_json_push_string_raw(qn_json_ptr self, const char * val)
{
    qn_json_ptr elem = qn_json_create_string(val, strlen(val));
    if (!elem) return qn_false;
    return qn_json_push(self, elem);
} // qn_json_push_string_raw

static inline qn_bool qn_json_push_integer(qn_json_ptr self, qn_integer val)
{
    qn_json_ptr elem = qn_json_create_integer(val);
    if (!elem) return qn_false;
    return qn_json_push(self, elem);
} // qn_json_push_integer

static inline qn_bool qn_json_push_number(qn_json_ptr self, qn_number val)
{
    qn_json_ptr elem = qn_json_create_number(val);
    if (!elem) return qn_false;
    return qn_json_push(self, elem);
} // qn_json_push_number

static inline qn_bool qn_json_push_boolean(qn_json_ptr self, qn_bool val)
{
    qn_json_ptr elem = qn_json_create_boolean(val);
    if (!elem) return qn_false;
    return qn_json_push(self, elem);
} // qn_json_push_boolean

static inline qn_bool qn_json_push_null(qn_json_ptr self)
{
    qn_json_ptr elem = qn_json_create_null();
    if (!elem) return qn_false;
    return qn_json_push(self, elem);
} // qn_json_push_null

static inline qn_bool qn_json_unshift_string(qn_json_ptr self, qn_string val)
{
    qn_json_ptr elem = qn_json_create_string(qn_str_cstr(val), qn_str_size(val));
    if (!elem) return qn_false;
    return qn_json_unshift(self, elem);
} // qn_json_unshift_string

static inline qn_bool qn_json_unshift_string_raw(qn_json_ptr self, const char * val)
{
    qn_json_ptr elem = qn_json_create_string(val, strlen(val));
    if (!elem) return qn_false;
    return qn_json_unshift(self, elem);
} // qn_json_unshift_string_raw

static inline qn_bool qn_json_unshift_integer(qn_json_ptr self, qn_integer val)
{
    qn_json_ptr elem = qn_json_create_integer(val);
    if (!elem) return qn_false;
    return qn_json_unshift(self, elem);
} // qn_json_unshift_integer

static inline qn_bool qn_json_unshift_number(qn_json_ptr self, qn_number val)
{
    qn_json_ptr elem = qn_json_create_number(val);
    if (!elem) return qn_false;
    return qn_json_unshift(self, elem);
} // qn_json_unshift_number

static inline qn_bool qn_json_unshift_boolean(qn_json_ptr self, qn_bool val)
{
    qn_json_ptr elem = qn_json_create_boolean(val);
    if (!elem) return qn_false;
    return qn_json_unshift(self, elem);
} // qn_json_unshift_boolean

static inline qn_bool qn_json_unshift_null(qn_json_ptr self)
{
    qn_json_ptr elem = qn_json_create_null();
    if (!elem) return qn_false;
    return qn_json_unshift(self, elem);
} // qn_json_unshift_null

static inline qn_string qn_json_get_string(qn_json_ptr self, const char * key, qn_string default_val)
{
    qn_json_ptr elem = qn_json_get(self, key);
    if (!elem || !qn_json_is_string(elem)) return default_val;
    return qn_json_to_string(elem);
} // qn_json_get_string

static inline const char * qn_json_get_string_raw(qn_json_ptr self, const char * key, const char * default_val)
{
    qn_json_ptr elem = qn_json_get(self, key);
    if (!elem || !qn_json_is_string(elem)) return default_val;
    return qn_str_cstr(qn_json_to_string(elem));
} // qn_json_get_string_raw

static inline qn_integer qn_json_get_integer(qn_json_ptr self, const char * key, qn_integer default_val)
{
    qn_json_ptr elem = qn_json_get(self, key);
    if (!elem || !qn_json_is_integer(elem)) return default_val;
    return qn_json_to_integer(elem);
} // qn_json_get_integer

static inline qn_number qn_json_get_number(qn_json_ptr self, const char * key, qn_number default_val)
{
    qn_json_ptr elem = qn_json_get(self, key);
    if (!elem || !qn_json_is_number(elem)) return default_val;
    return qn_json_to_number(elem);
} // qn_json_get_number

static inline qn_bool qn_json_get_boolean(qn_json_ptr self, const char * key, qn_bool default_val)
{
    qn_json_ptr elem = qn_json_get(self, key);
    if (!elem || !qn_json_is_boolean(elem)) return default_val;
    return qn_json_to_boolean(elem);
} // qn_json_get_boolean

static inline qn_string qn_json_pick_string(qn_json_ptr self, int n, qn_string default_val)
{
    qn_json_ptr elem = qn_json_pick(self, n);
    if (!elem || !qn_json_is_string(elem)) return default_val;
    return qn_json_to_string(elem);
} // qn_json_pick_string

static inline const char * qn_json_pick_string_raw(qn_json_ptr self, int n, const char * default_val)
{
    qn_json_ptr elem = qn_json_pick(self, n);
    if (!elem || !qn_json_is_string(elem)) return default_val;
    return qn_str_cstr(qn_json_to_string(elem));
} // qn_json_pick_string_raw

static inline qn_integer qn_json_pick_integer(qn_json_ptr self, int n, qn_integer default_val)
{
    qn_json_ptr elem = qn_json_pick(self, n);
    if (!elem || !qn_json_is_integer(elem)) return default_val;
    return qn_json_to_integer(elem);
} // qn_json_pick_integer

static inline qn_number qn_json_pick_number(qn_json_ptr self, int n, qn_number default_val)
{
    qn_json_ptr elem = qn_json_pick(self, n);
    if (!elem || !qn_json_is_number(elem)) return default_val;
    return qn_json_to_number(elem);
} // qn_json_pick_number

static inline qn_bool qn_json_pick_boolean(qn_json_ptr self, int n, qn_bool default_val)
{
    qn_json_ptr elem = qn_json_pick(self, n);
    if (!elem || !qn_json_is_boolean(elem)) return default_val;
    return qn_json_to_boolean(elem);
} // qn_json_pick_boolean

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_H__

