#ifndef __QN_JSON_H__
#define __QN_JSON_H__

#include <string.h>

#include "qiniu/base/basic_types.h"
#include "qiniu/base/string.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of JSON ----

struct _QN_JSON_OBJECT;
typedef struct _QN_JSON_OBJECT * qn_json_object_ptr;
struct _QN_JSON_ARRAY;
typedef struct _QN_JSON_ARRAY * qn_json_array_ptr;

typedef qn_bool qn_json_boolean;
typedef qn_string qn_json_string;
typedef qn_integer qn_json_integer;
typedef qn_number qn_json_number;
typedef qn_uint32 qn_json_hash;

typedef enum _QN_JSON_CLASS {
    QN_JSON_UNKNOWN = 0,
    QN_JSON_NULL = 1,
    QN_JSON_BOOLEAN = 2,
    QN_JSON_INTEGER = 3,
    QN_JSON_NUMBER = 4,
    QN_JSON_STRING = 5,
    QN_JSON_ARRAY = 6,
    QN_JSON_OBJECT = 7
} qn_json_class;

typedef union _QN_JSON_VARIANT
{
    qn_json_object_ptr object;
    qn_json_array_ptr array;
    qn_json_string string;
    qn_json_integer integer;
    qn_json_number number;
    qn_json_boolean boolean;
} qn_json_variant, *qn_json_variant_ptr;

QN_API extern qn_json_object_ptr qn_json_create_object(void);
QN_API extern qn_json_array_ptr qn_json_create_array(void);

QN_API extern qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr restrict obj, const char * restrict key);
QN_API extern qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr restrict obj, const char * restrict key);

QN_API extern qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr restrict arr);
QN_API extern qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr restrict arr);
QN_API extern qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr restrict arr);
QN_API extern qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr restrict arr);

QN_API extern void qn_json_destroy_object(qn_json_object_ptr restrict obj);
QN_API extern void qn_json_destroy_array(qn_json_array_ptr restrict arr);

QN_API extern int qn_json_size_object(qn_json_object_ptr restrict obj);
QN_API extern int qn_json_size_array(qn_json_array_ptr restrict arr);

static inline qn_bool qn_json_is_empty_object(qn_json_object_ptr restrict obj)
{
    return qn_json_size_object(obj) == 0;
}

static inline qn_bool qn_json_is_empty_array(qn_json_array_ptr restrict obj)
{
    return qn_json_size_array(obj) == 0;
}

QN_API extern qn_json_object_ptr qn_json_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict default_val);
QN_API extern qn_json_array_ptr qn_json_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict default_val);
QN_API extern qn_string qn_json_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict default_val);
QN_API extern qn_integer qn_json_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_integer default_val);
QN_API extern qn_number qn_json_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_number default_val);
QN_API extern qn_bool qn_json_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool default_val);

QN_API extern qn_json_object_ptr qn_json_pick_object(qn_json_array_ptr restrict arr, int n, qn_json_object_ptr restrict default_val);
QN_API extern qn_json_array_ptr qn_json_pick_array(qn_json_array_ptr restrict arr, int n, qn_json_array_ptr restrict default_val);
QN_API extern qn_string qn_json_pick_string(qn_json_array_ptr restrict arr, int n, qn_string restrict default_val);
QN_API extern qn_integer qn_json_pick_integer(qn_json_array_ptr restrict arr, int n, qn_integer default_val);
QN_API extern qn_number qn_json_pick_number(qn_json_array_ptr restrict arr, int n, qn_number default_val);
QN_API extern qn_bool qn_json_pick_boolean(qn_json_array_ptr restrict arr, int n, qn_bool default_val);

QN_API extern qn_bool qn_json_set_string(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val);
QN_API extern qn_bool qn_json_set_text(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val, int size);
QN_API extern qn_bool qn_json_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_integer val);
QN_API extern qn_bool qn_json_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_number val);
QN_API extern qn_bool qn_json_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val);
QN_API extern qn_bool qn_json_set_null(qn_json_object_ptr restrict obj, const char * restrict key);
QN_API extern void qn_json_unset(qn_json_object_ptr restrict obj, const char * restrict key);

QN_API extern qn_bool qn_json_push_string(qn_json_array_ptr restrict arr, const char * restrict val);
QN_API extern qn_bool qn_json_push_text(qn_json_array_ptr restrict arr, const char * restrict val, int size);
QN_API extern qn_bool qn_json_push_integer(qn_json_array_ptr restrict arr, qn_integer val);
QN_API extern qn_bool qn_json_push_number(qn_json_array_ptr restrict arr, qn_number val);
QN_API extern qn_bool qn_json_push_boolean(qn_json_array_ptr restrict arr, qn_bool val);
QN_API extern qn_bool qn_json_push_null(qn_json_array_ptr restrict arr);
QN_API extern void qn_json_pop(qn_json_array_ptr restrict arr);

QN_API extern qn_bool qn_json_unshift_string(qn_json_array_ptr restrict arr, const char * restrict val);
QN_API extern qn_bool qn_json_unshift_text(qn_json_array_ptr restrict arr, const char * restrict val, int size);
QN_API extern qn_bool qn_json_unshift_integer(qn_json_array_ptr restrict arr, qn_integer val);
QN_API extern qn_bool qn_json_unshift_number(qn_json_array_ptr restrict arr, qn_number val);
QN_API extern qn_bool qn_json_unshift_boolean(qn_json_array_ptr restrict arr, qn_bool val);
QN_API extern qn_bool qn_json_unshift_null(qn_json_array_ptr restrict arr);
QN_API extern void qn_json_shift(qn_json_array_ptr restrict arr);

// ---- Definition of iterator of JSON ----

struct _QN_JSON_ITERATOR;
typedef struct _QN_JSON_ITERATOR * qn_json_iterator_ptr;

QN_API extern qn_json_iterator_ptr qn_json_itr_create(void);
QN_API extern void qn_json_itr_destroy(qn_json_iterator_ptr restrict itr);
QN_API extern void qn_json_itr_reset(qn_json_iterator_ptr restrict itr);
QN_API extern void qn_json_itr_rewind(qn_json_iterator_ptr restrict itr);

QN_API extern qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr restrict itr);
QN_API extern int qn_json_itr_steps(qn_json_iterator_ptr restrict itr);
QN_API extern qn_string qn_json_itr_get_key(qn_json_iterator_ptr restrict itr);
QN_API extern void qn_json_itr_set_status(qn_json_iterator_ptr restrict itr, qn_uint32 sts);
QN_API extern qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr restrict itr);

QN_API extern qn_bool qn_json_itr_push_object(qn_json_iterator_ptr restrict itr, qn_json_object_ptr obj);
QN_API extern qn_bool qn_json_itr_push_array(qn_json_iterator_ptr restrict itr, qn_json_array_ptr arr);
QN_API extern void qn_json_itr_pop(qn_json_iterator_ptr restrict itr);
QN_API extern qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr restrict itr);
QN_API extern qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr restrict itr);

#define QN_JSON_ITR_OK (0)
#define QN_JSON_ITR_NO_MORE (-1)

typedef int (*qn_json_itr_callback)(void * data, qn_json_class cls, qn_json_variant_ptr restrict val);
QN_API extern int qn_json_itr_advance(qn_json_iterator_ptr restrict itr, void * data, qn_json_itr_callback cb);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_H__

