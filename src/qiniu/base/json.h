/***************************************************************************//**
* @file qiniu/base/json.h
* @brief The header file declares all JSON-related basic types and functions.
*
* AUTHOR      : liangtao@qiniu.com (QQ: 510857)
* COPYRIGHT   : 2016(c) Shanghai Qiniu Information Technologies Co., Ltd.
* DESCRIPTION :
*
* This header file declares all JSON-related basic functions, like that create
* and manipulate JSON objects or arrays. A set of iterating functions are also
* included for traversing each element in objects or arrays.
*******************************************************************************/

#ifndef __QN_BASE_JSON_H__
#define __QN_BASE_JSON_H__ 1

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of JSON ----

/***************************************************************************//**
* @defgroup JSON-Object Implementation of JSON Object
*
* The **qn_json_object_ptr** type represents a JSON object. Use this type to
* collect a set of pairs of key and value. The maximum quantity of pairs is
* 65535.
*******************************************************************************/

struct _QN_JSON_OBJECT;
typedef struct _QN_JSON_OBJECT * qn_json_object_ptr;

/***************************************************************************//**
* @defgroup JSON-Array Implementation of JSON Array
*
* The **qn_json_array_ptr** type represents a JSON array. Use this type to
* collect a list of values. The maximum quantity of values is 65535.
* This type can be used as a **bidirectional queue**.
*******************************************************************************/

struct _QN_JSON_ARRAY;
typedef struct _QN_JSON_ARRAY * qn_json_array_ptr;

/***************************************************************************//**
* @defgroup JSON-Variant Implementation of JSON Variant
*
* The **qn_json_variant_ptr** type represents a JSON variant used to save a
* JSON value.
*******************************************************************************/

typedef qn_bool qn_json_boolean;
typedef qn_string qn_json_string;
typedef qn_integer qn_json_integer;
typedef qn_number qn_json_number;

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

// ---- Declaration of JSON Object ----

QN_SDK extern qn_json_object_ptr qn_json_immutable_empty_object(void);
QN_SDK extern qn_json_object_ptr qn_json_create_object(void);
QN_SDK extern qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr restrict obj, const char * restrict key);
QN_SDK extern qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr restrict obj, const char * restrict key);
QN_SDK extern void qn_json_destroy_object(qn_json_object_ptr restrict obj);

QN_SDK extern int qn_json_size_object(qn_json_object_ptr restrict obj);

static inline qn_bool qn_json_is_empty_object(qn_json_object_ptr restrict obj)
{
    return qn_json_size_object(obj) == 0;
}

QN_SDK extern qn_json_variant_ptr qn_json_get_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_class cls);

static inline qn_json_object_ptr qn_json_get_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_OBJECT);
    if (! var) return default_val;
    return var->object;
}

static inline qn_json_array_ptr qn_json_get_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_ARRAY);
    if (! var) return default_val;
    return var->array;
}

static inline qn_string qn_json_get_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_STRING);
    if (! var) return default_val;
    return var->string;
}

static inline const char * qn_json_get_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_STRING);
    if (! var) return default_val;
    return qn_str_cstr(var->string);
}

static inline qn_json_integer qn_json_get_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_INTEGER);
    if (! var) return default_val;
    return var->integer;
}

static inline qn_json_number qn_json_get_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_NUMBER);
    if (! var) return default_val;
    return var->number;
}

static inline qn_bool qn_json_get_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool default_val)
{
    qn_json_variant_ptr var = qn_json_get_variant(obj, key, QN_JSON_BOOLEAN);
    if (! var) return default_val;
    return var->boolean;
}

QN_SDK extern qn_bool qn_json_set_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_class cls, qn_json_variant var);

static inline qn_bool qn_json_set_object(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_object_ptr restrict val)
{
    qn_json_variant var;
    var.object = val;
    return qn_json_set_variant(obj, key, QN_JSON_OBJECT, var);
}

static inline qn_bool qn_json_set_array(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_array_ptr restrict val)
{
    qn_json_variant var;
    var.array = val;
    return qn_json_set_variant(obj, key, QN_JSON_ARRAY, var);
}

static inline qn_bool qn_json_set_string(qn_json_object_ptr restrict obj, const char * restrict key, qn_string restrict val)
{
    qn_json_variant var;
    return (var.string = qn_str_duplicate(val)) ? qn_json_set_variant(obj, key, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_set_cstr(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val)
{
    qn_json_variant var;
    return (var.string = qn_cs_duplicate(val)) ? qn_json_set_variant(obj, key, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_set_text(qn_json_object_ptr restrict obj, const char * restrict key, const char * restrict val, qn_size size)
{
    qn_json_variant var;
    var.string = qn_cs_clone(val, size);
    return (var.string = qn_cs_clone(val, size)) ? qn_json_set_variant(obj, key, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_set_integer(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_integer val)
{
    qn_json_variant var;
    var.integer = val;
    return qn_json_set_variant(obj, key, QN_JSON_INTEGER, var);
}

static inline qn_bool qn_json_set_number(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_number val)
{
    qn_json_variant var;
    var.number = val;
    return qn_json_set_variant(obj, key, QN_JSON_NUMBER, var);
}

static inline qn_bool qn_json_set_boolean(qn_json_object_ptr restrict obj, const char * restrict key, qn_bool val)
{
    qn_json_variant var;
    var.boolean = val;
    return qn_json_set_variant(obj, key, QN_JSON_BOOLEAN, var);
}

static inline qn_bool qn_json_set_null(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant var;
    var.integer = 0;
    return qn_json_set_variant(obj, key, QN_JSON_NULL, var);
}


QN_SDK extern void qn_json_unset(qn_json_object_ptr restrict obj, const char * restrict key);

QN_SDK extern qn_bool qn_json_rename(qn_json_object_ptr restrict obj, const char * restrict old_key, const char * new_key);

// ---- Declaration of JSON Array ----

QN_SDK extern qn_json_array_ptr qn_json_immutable_empty_array(void);
QN_SDK extern qn_json_array_ptr qn_json_create_array(void);

QN_SDK extern qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr restrict arr);
QN_SDK extern qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr restrict arr);
QN_SDK extern qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr restrict arr);
QN_SDK extern qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr restrict arr);

QN_SDK extern void qn_json_destroy_array(qn_json_array_ptr restrict arr);

QN_SDK extern int qn_json_size_array(qn_json_array_ptr restrict arr);

static inline qn_bool qn_json_is_empty_array(qn_json_array_ptr restrict obj)
{
    return qn_json_size_array(obj) == 0;
}

QN_SDK extern qn_json_variant_ptr qn_json_pick_variant(qn_json_array_ptr restrict arr, int n, qn_json_class cls);

static inline qn_json_object_ptr qn_json_pick_object(qn_json_array_ptr restrict arr, int n, qn_json_object_ptr restrict default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_OBJECT);
    if (! var) return default_val;
    return var->object;
}

static inline qn_json_array_ptr qn_json_pick_array(qn_json_array_ptr restrict arr, int n, qn_json_array_ptr restrict default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_ARRAY);
    if (! var) return default_val;
    return var->array;
}

static inline qn_string qn_json_pick_string(qn_json_array_ptr restrict arr, int n, qn_string restrict default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_STRING);
    if (! var) return default_val;
    return var->string;
}

static inline const char * qn_json_pick_cstr(qn_json_array_ptr restrict arr, int n, const char * restrict default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_STRING);
    if (! var) return default_val;
    return qn_str_cstr(var->string);
}

static inline qn_json_integer qn_json_pick_integer(qn_json_array_ptr restrict arr, int n, qn_json_integer default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_INTEGER);
    if (! var) return default_val;
    return var->integer;
}

static inline qn_json_number qn_json_pick_number(qn_json_array_ptr restrict arr, int n, qn_json_number default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_NUMBER);
    if (! var) return default_val;
    return var->number;
}

static inline qn_bool qn_json_pick_boolean(qn_json_array_ptr restrict arr, int n, qn_bool default_val)
{
    qn_json_variant_ptr var = qn_json_pick_variant(arr, n, QN_JSON_BOOLEAN);
    if (! var) return default_val;
    return var->boolean;
}

QN_SDK extern qn_bool qn_json_push_variant(qn_json_array_ptr restrict arr, qn_json_class cls, qn_json_variant var);

static inline qn_bool qn_json_push_object(qn_json_array_ptr restrict arr, qn_json_object_ptr restrict val)
{
    qn_json_variant var;
    var.object = val;
    return qn_json_push_variant(arr, QN_JSON_OBJECT, var);
}

static inline qn_bool qn_json_push_array(qn_json_array_ptr restrict arr, qn_json_array_ptr restrict val)
{
    qn_json_variant var;
    var.array = val;
    return qn_json_push_variant(arr, QN_JSON_ARRAY, var);
}

static inline qn_bool qn_json_push_string(qn_json_array_ptr restrict arr, qn_string restrict val)
{
    qn_json_variant var;
    return (var.string = qn_str_duplicate(val)) ? qn_json_push_variant(arr, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_push_cstr(qn_json_array_ptr restrict arr, const char * restrict val)
{
    qn_json_variant var;
    return (var.string = qn_cs_duplicate(val)) ? qn_json_push_variant(arr, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_push_text(qn_json_array_ptr restrict arr, const char * restrict val, qn_size size)
{
    qn_json_variant var;
    return (var.string = qn_cs_clone(val, size)) ? qn_json_push_variant(arr, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_push_integer(qn_json_array_ptr restrict arr, qn_json_integer val)
{
    qn_json_variant var;
    var.integer = val;
    return qn_json_push_variant(arr, QN_JSON_INTEGER, var);
}

static inline qn_bool qn_json_push_number(qn_json_array_ptr restrict arr, qn_json_number val)
{
    qn_json_variant var;
    var.number = val;
    return qn_json_push_variant(arr, QN_JSON_NUMBER, var);
}

static inline qn_bool qn_json_push_boolean(qn_json_array_ptr restrict arr, qn_bool val)
{
    qn_json_variant var;
    var.boolean = val;
    return qn_json_push_variant(arr, QN_JSON_BOOLEAN, var);
}

static inline qn_bool qn_json_push_null(qn_json_array_ptr restrict arr)
{
    qn_json_variant var;
    var.integer = 0;
    return qn_json_push_variant(arr, QN_JSON_NULL, var);
}

QN_SDK extern void qn_json_pop(qn_json_array_ptr restrict arr);

QN_SDK extern qn_bool qn_json_unshift_variant(qn_json_array_ptr restrict arr, qn_json_class cls, qn_json_variant var);

static inline qn_bool qn_json_unshift_object(qn_json_array_ptr restrict arr, qn_json_object_ptr restrict val)
{
    qn_json_variant var;
    var.object = val;
    return qn_json_unshift_variant(arr, QN_JSON_OBJECT, var);
}

static inline qn_bool qn_json_unshift_array(qn_json_array_ptr restrict arr, qn_json_array_ptr restrict val)
{
    qn_json_variant var;
    var.array = val;
    return qn_json_unshift_variant(arr, QN_JSON_ARRAY, var);
}

static inline qn_bool qn_json_unshift_string(qn_json_array_ptr restrict arr, qn_string restrict val)
{
    qn_json_variant var;
    return (var.string = qn_str_duplicate(val)) ? qn_json_unshift_variant(arr, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_unshift_cstr(qn_json_array_ptr restrict arr, const char * restrict val)
{
    qn_json_variant var;
    return (var.string = qn_cs_duplicate(val)) ? qn_json_unshift_variant(arr, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_unshift_text(qn_json_array_ptr restrict arr, const char * restrict val, qn_size size)
{
    qn_json_variant var;
    return (var.string = qn_cs_clone(val, size)) ? qn_json_unshift_variant(arr, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_unshift_integer(qn_json_array_ptr restrict arr, qn_json_integer val)
{
    qn_json_variant var;
    var.integer = val;
    return qn_json_unshift_variant(arr, QN_JSON_INTEGER, var);
}

static inline qn_bool qn_json_unshift_number(qn_json_array_ptr restrict arr, qn_json_number val)
{
    qn_json_variant var;
    var.number = val;
    return qn_json_unshift_variant(arr, QN_JSON_NUMBER, var);
}

static inline qn_bool qn_json_unshift_boolean(qn_json_array_ptr restrict arr, qn_bool val)
{
    qn_json_variant var;
    var.boolean = val;
    return qn_json_unshift_variant(arr, QN_JSON_BOOLEAN, var);
}

static inline qn_bool qn_json_unshift_null(qn_json_array_ptr restrict arr)
{
    qn_json_variant var;
    var.integer = 0;
    return qn_json_unshift_variant(arr, QN_JSON_NULL, var);
}

QN_SDK extern void qn_json_shift(qn_json_array_ptr restrict arr);

QN_SDK extern qn_bool qn_json_replace_variant(qn_json_array_ptr restrict arr, int n, qn_json_class cls, qn_json_variant var);

static inline qn_bool qn_json_replace_object(qn_json_array_ptr restrict arr, int n, qn_json_object_ptr restrict val)
{
    qn_json_variant var;
    var.object = val;
    return qn_json_replace_variant(arr, n, QN_JSON_OBJECT, var);
}

static inline qn_bool qn_json_replace_array(qn_json_array_ptr restrict arr, int n, qn_json_array_ptr restrict val)
{
    qn_json_variant var;
    var.array = val;
    return qn_json_replace_variant(arr, n, QN_JSON_ARRAY, var);
}

static inline qn_bool qn_json_replace_string(qn_json_array_ptr restrict arr, int n, qn_string restrict val)
{
    qn_json_variant var;
    return (var.string = qn_str_duplicate(val)) ? qn_json_replace_variant(arr, n, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_replace_cstr(qn_json_array_ptr restrict arr, int n, const char * restrict val)
{
    qn_json_variant var;
    return (var.string = qn_cs_duplicate(val)) ? qn_json_replace_variant(arr, n, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_replace_text(qn_json_array_ptr restrict arr, int n, const char * restrict val, qn_size size)
{
    qn_json_variant var;
    return (var.string = qn_cs_clone(val, size)) ? qn_json_replace_variant(arr, n, QN_JSON_STRING, var) : qn_false;
}

static inline qn_bool qn_json_replace_integer(qn_json_array_ptr restrict arr, int n, qn_json_integer val)
{
    qn_json_variant var;
    var.integer = val;
    return qn_json_replace_variant(arr, n, QN_JSON_INTEGER, var);
}

static inline qn_bool qn_json_replace_number(qn_json_array_ptr restrict arr, int n, qn_json_number val)
{
    qn_json_variant var;
    var.number = val;
    return qn_json_replace_variant(arr, n, QN_JSON_NUMBER, var);
}

static inline qn_bool qn_json_replace_boolean(qn_json_array_ptr restrict arr, int n, qn_bool val)
{
    qn_json_variant var;
    var.boolean = val;
    return qn_json_replace_variant(arr, n, QN_JSON_BOOLEAN, var);
}

static inline qn_bool qn_json_replace_null(qn_json_array_ptr restrict arr, int n)
{
    qn_json_variant var;
    var.integer = 0;
    return qn_json_replace_variant(arr, n, QN_JSON_NULL, var);
}


/***************************************************************************//**
* @defgroup JSON-Iterator Implementation of JSON Object and Array Iterator
*
* The **qn_json_iterator_ptr** represents a JSON iterator.  Use this
* iterator type to iterate all elements reside in a JSON object or array.
* Moreover, the iterator can hold one status value for each level of the
* hierarchy, so the caller can implement some useful features based on this, 
* like the JSON formatter and parser provided by this SDK.
*******************************************************************************/

struct _QN_JSON_ITERATOR;
typedef struct _QN_JSON_ITERATOR * qn_json_iterator_ptr;

QN_SDK extern qn_json_iterator_ptr qn_json_itr_create(void);
QN_SDK extern void qn_json_itr_destroy(qn_json_iterator_ptr restrict itr);
QN_SDK extern void qn_json_itr_reset(qn_json_iterator_ptr restrict itr);
QN_SDK extern void qn_json_itr_rewind(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr restrict itr);
QN_SDK extern int qn_json_itr_done_steps(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_string qn_json_itr_get_key(qn_json_iterator_ptr restrict itr);

QN_SDK extern void qn_json_itr_set_status(qn_json_iterator_ptr restrict itr, qn_uint32 sts);
QN_SDK extern qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_bool qn_json_itr_push_object(qn_json_iterator_ptr restrict itr, qn_json_object_ptr restrict obj);
QN_SDK extern qn_bool qn_json_itr_push_array(qn_json_iterator_ptr restrict itr, qn_json_array_ptr restrict arr);
QN_SDK extern void qn_json_itr_pop(qn_json_iterator_ptr restrict itr);

QN_SDK extern qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr restrict itr);
QN_SDK extern qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr restrict itr);

enum
{
    QN_JSON_ITR_OK = 0,
    QN_JSON_ITR_NO_MORE = 1
};

typedef int (*qn_json_itr_callback_fn)(void * data, qn_json_class cls, qn_json_variant_ptr restrict val);
QN_SDK extern int qn_json_itr_advance(qn_json_iterator_ptr restrict itr, void * data, qn_json_itr_callback_fn cb);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE_JSON_H__

