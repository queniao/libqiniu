/***************************************************************************//**
* @file qiniu/base/json.c
* @brief The header file declares all JSON-related basic types and functions.
*
* AUTHOR      : liangtao@qiniu.com (QQ: 510857)
* COPYRIGHT   : 2017(c) Shanghai Qiniu Information Technologies Co., Ltd.
* DESCRIPTION :
*
* This source file define all JSON-related basic functions, like that create
* and manipulate JSON objects or arrays. A set of iterating functions are also
* included for traversing each element in objects or arrays.
*******************************************************************************/

#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef qn_uint32 qn_json_hash;
typedef unsigned short int qn_json_pos;

static inline void qn_json_destroy_element(qn_json_class cls, qn_json_variant_ptr restrict elem)
{
    switch (cls) {
        case QN_JSON_OBJECT: qn_json_destroy_object(elem->object); break;
        case QN_JSON_ARRAY: qn_json_destroy_array(elem->array); break;
        case QN_JSON_STRING: qn_str_destroy(elem->string); break;
        default: break;
    } // switch
}

// ---- Inplementation of object of JSON ----

typedef struct _QN_JSON_OBJ_ITEM
{
    qn_string key;
    qn_json_class class;
    qn_json_variant elem;
} qn_json_obj_item;

typedef struct _QN_JSON_OBJECT
{
    qn_json_obj_item * itm;
    qn_json_pos cnt;
    qn_json_pos cap;

    qn_json_obj_item init_itm[2];
} qn_json_object;

// static qn_json_hash qn_json_obj_calculate_hash(const char * restrict cstr)
// {
//     qn_json_hash hash = 5381;
//     int c;
// 
//     while ((c = *cstr++) != '\0') {
//         hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//     } // while
//     return hash;
// }

/***************************************************************************//**
* @ingroup JSON-Object
*
* Return a static and immutable empty object.
*
* @retval non-NULL A pointer to the that static and immutable empty object.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_immutable_empty_object(void)
{
    static qn_json_object obj;
    return &obj;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Allocate and construct a new JSON object.
*
* @retval non-NULL A pointer to the new JSON object.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_object(void)
{
    qn_json_object_ptr new_obj = calloc(1, sizeof(qn_json_object));
    if (!new_obj) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_obj->itm = &new_obj->init_itm[0];
    new_obj->cap = sizeof(new_obj->init_itm) / sizeof(new_obj->init_itm[0]);
    return new_obj;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Destruct and deallocate a JSON object.
*
* @param [in] obj The pointer to the object to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_destroy_object(qn_json_object_ptr restrict obj)
{
    qn_json_pos i;

    for (i = 0; i < obj->cnt; i += 1) {
        qn_json_destroy_element(obj->itm[i].class, &obj->itm[i].elem);
        qn_str_destroy(obj->itm[i].key);
    } // for
    if (obj->itm != &obj->init_itm[0]) {
        free(obj->itm);
    } // if
    free(obj);
}

static qn_json_pos qn_json_obj_bsearch(qn_json_obj_item * restrict itm, qn_json_pos cnt, const char * restrict key, int * restrict existence)
{
    qn_json_pos begin = 0;
    qn_json_pos end = cnt;
    qn_json_pos mid = 0;
    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        if (strcmp(itm[mid].key, key) < 0) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    // -- Finally, the `begin` variable points to the first key that is equal to or larger than the given key,
    //    as an insert point.
    *existence = (begin < cnt) ? strcmp(itm[begin].key, key) : 1;
    return begin;
}

static qn_bool qn_json_obj_augment(qn_json_object_ptr restrict obj)
{
    qn_json_pos new_cap = obj->cap * 2;
    qn_json_obj_item * new_itm = calloc(1, sizeof(qn_json_obj_item) * new_cap);
    if (!new_itm) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_itm, obj->itm, sizeof(qn_json_obj_item) * obj->cnt);
    if (obj->itm != &obj->init_itm[0]) free(obj->itm);

    obj->itm = new_itm;
    obj->cap = new_cap;
    return qn_true;
}

// ---- Inplementation of array of JSON ----

typedef struct _QN_JSON_ARR_ITEM
{
    qn_json_class class;
    qn_json_variant elem;
} qn_json_arr_item;

typedef struct _QN_JSON_ARRAY
{
    qn_json_arr_item * itm;
    qn_json_pos begin;
    qn_json_pos end;
    qn_json_pos cnt;
    qn_json_pos cap;

    qn_json_arr_item init_itm[4];
} qn_json_array, *qn_json_array_ptr;

/***************************************************************************//**
* @ingroup JSON-Array
*
* Return a static and immutable empty array.
*
* @retval non-NULL A pointer to the that static and immutable empty array.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_immutable_empty_array(void)
{
    static qn_json_array arr;
    return &arr;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Allocate and construct a new JSON array.
*
* @retval non-NULL A pointer to the new JSON array.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_array(void)
{
    qn_json_array_ptr new_arr = calloc(1, sizeof(qn_json_array));
    if (!new_arr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_arr->itm = &new_arr->init_itm[0];
    new_arr->cap = sizeof(new_arr->init_itm) / sizeof(new_arr->init_itm[0]);
    return new_arr;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Destruct and deallocate a JSON array.
*
* @param [in] arr The pointer to the array to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_destroy_array(qn_json_array_ptr restrict arr)
{
    qn_json_pos i;

    for (i = arr->begin; i < arr->end; i += 1) {
        qn_json_destroy_element(arr->itm[i].class, &arr->itm[i].elem);
    } // for
    if (arr->itm != &arr->init_itm[0]) free(arr->itm);
    free(arr);
}

enum
{
    QN_JSON_ARR_PUSHING = 0,
    QN_JSON_ARR_UNSHIFTING = 1
};

static qn_bool qn_json_arr_augment(qn_json_array_ptr restrict arr, int direct)
{
    qn_json_pos new_cap = arr->cap * 2;
    qn_json_arr_item * new_itm = calloc(1, sizeof(qn_json_arr_item) * new_cap);
    if (!new_itm) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    if (direct == QN_JSON_ARR_PUSHING) {
        memcpy(new_itm + arr->begin, arr->itm + arr->begin, sizeof(qn_json_arr_item) * arr->cnt);
    } else {
        memcpy(new_itm + arr->begin + arr->cap, arr->itm + arr->begin, sizeof(qn_json_arr_item) * arr->cnt);
        arr->begin += arr->cap;
        arr->end += arr->cap;
    } // if
    if (arr->itm != &arr->init_itm[0]) free(arr->itm);

    arr->itm = new_itm;
    arr->cap = new_cap;
    return qn_true;
}

static inline qn_json_pos qn_json_arr_find(qn_json_array_ptr restrict arr, int n)
{
    return (n < 0 || n >= arr->cnt) ? arr->cnt : (arr->begin + n);
}

// ---- Inplementation of JSON ----

/***************************************************************************//**
* @ingroup JSON-Object
*
* Create a new object and then set it as an element into the target object.
*
* @param [in] obj The non-NULL pointer to the target object.
* @param [in] key The key of the new object.
* @retval non-NULL The pointer to the new object.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && ! qn_json_set_variant(obj, key, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Create a new array and then set it as an element into the target object.
*
* @param [in] obj The non-NULL pointer to the target object.
* @param [in] key The key of the new array.
* @retval non-NULL The pointer to the new array.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && ! qn_json_set_variant(obj, key, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new object and then push it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new object.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && ! qn_json_push_variant(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new array and then push it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new array.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && ! qn_json_push_variant(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new object and then unshift it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new object.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && ! qn_json_unshift_variant(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Create a new array and then unshift it as an element into the target array.
*
* @param [in] obj The non-NULL pointer to the target array.
* @retval non-NULL The pointer to the new array.
* @retval NULL Failed in creation or setting, and an error code is set.
*******************************************************************************/
QN_SDK qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr restrict arr)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && ! qn_json_unshift_variant(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Return the current quantity of pairs of the object.
*
* @param [in] obj The non-NULL pointer to the object.
* @retval Integer-Value The current quantity of pairs of the object.
*******************************************************************************/
QN_SDK int qn_json_size_object(qn_json_object_ptr restrict obj)
{
    return obj->cnt;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Return the current quantity of values of the array.
*
* @param [in] obj The non-NULL pointer to the array.
* @retval Integer-Value The current quantity of values of the array.
*******************************************************************************/
QN_SDK int qn_json_size_array(qn_json_array_ptr restrict arr)
{
    return arr->cnt;
}

QN_SDK qn_json_variant_ptr qn_json_get_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_class cls)
{
    int existence;
    qn_json_pos pos;
    if (obj->cnt == 0) return NULL;
    pos = qn_json_obj_bsearch(obj->itm, obj->cnt, key, &existence);
    return (existence != 0 || obj->itm[pos].class != cls) ? NULL : &obj->itm[pos].elem;
}

QN_SDK qn_json_variant_ptr qn_json_pick_variant(qn_json_array_ptr restrict arr, int n, qn_json_class cls)
{
    qn_json_pos pos;
    if (arr->cnt == 0) return NULL;
    pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != cls) ? NULL : &arr->itm[pos].elem;
}

QN_SDK qn_bool qn_json_set_variant(qn_json_object_ptr restrict obj, const char * restrict key, qn_json_class cls, qn_json_variant new_elem)
{
    qn_json_pos pos;
    qn_string new_key;
    int existence;

    assert(obj);
    assert(key);

    if (obj->cap == 0) {
        qn_err_json_set_modifying_immutable_object();
        return qn_false;
    } // if

    pos = qn_json_obj_bsearch(obj->itm, obj->cnt, key, &existence);
    if (existence == 0) {
        // There is an element according to the given key.
        qn_json_destroy_element(obj->itm[pos].class, &obj->itm[pos].elem);
        obj->itm[pos].class = cls;
        obj->itm[pos].elem = new_elem;
        return qn_true;
    } // if

    if ((obj->cap - obj->cnt) <= 0 && !qn_json_obj_augment(obj)) return qn_false;
    if (!(new_key = qn_cs_duplicate(key))) return qn_false;

    if (pos < obj->cnt) memmove(&obj->itm[pos+1], &obj->itm[pos], sizeof(qn_json_obj_item) * (obj->cnt - pos));

    obj->itm[pos].class = cls;
    obj->itm[pos].key = new_key;
    obj->itm[pos].elem = new_elem;

    obj->cnt += 1;
    return qn_true;
}

/***************************************************************************//**
* @ingroup JSON-Object
*
* Unset the element which corresponds to the key.
*
* @param [in] obj The non-NULL pointer to the target object.
* @param [in] key The key of the unsetting element.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_unset(qn_json_object_ptr restrict obj, const char * restrict key)
{
    qn_json_pos pos;
    int existence;

    if (obj->cnt == 0) return;

    pos = qn_json_obj_bsearch(obj->itm, obj->cnt, key, &existence);
    if (existence != 0) return; // There is no element corresponds to the key.

    qn_json_destroy_element(obj->itm[pos].class, &obj->itm[pos].elem);
    qn_str_destroy(obj->itm[pos].key);
    if (pos < obj->cnt - 1) memmove(&obj->itm[pos], &obj->itm[pos+1], sizeof(qn_json_obj_item) * (obj->cnt - pos - 1));
    obj->cnt -= 1;
}

QN_SDK qn_bool qn_json_rename(qn_json_object_ptr restrict obj, const char * restrict old_key, const char * new_key)
{
    qn_json_pos old_pos;
    qn_json_pos new_pos;
    qn_json_obj_item tmp_item;
    qn_string new_key_str;
    int existence;

    if (obj->cnt == 0) {
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    if (strcmp(old_key, new_key) == 0) return qn_true; // The old key is exactly the same to the new key.

    old_pos = qn_json_obj_bsearch(obj->itm, obj->cnt, old_key, &existence);
    if (existence != 0) {
        // ---- There is no element corresponds to the old key.
        qn_err_set_no_such_entry();
        return qn_false;
    } // if

    new_pos = qn_json_obj_bsearch(obj->itm, obj->cnt, new_key, &existence);
    if (existence == 0) {
        // ---- There is an element corresponds to the new key.
        // -- Destroy the element to be replaced.
        qn_json_destroy_element(obj->itm[new_pos].class, &obj->itm[new_pos].elem);

        // -- Replace the element.
        obj->itm[new_pos].class = obj->itm[old_pos].class;
        obj->itm[new_pos].elem = obj->itm[old_pos].elem;

        // -- Destroy the old key.
        qn_str_destroy(obj->itm[old_pos].key);

        if (old_pos < obj->cnt - 1) memmove(&obj->itm[old_pos], &obj->itm[old_pos+1], sizeof(qn_json_obj_item) * (obj->cnt - old_pos - 1));
        obj->cnt -= 1;

        return qn_true;
    } // if

    // ---- There is no element corresponds to the new key.
    // -- Replace the old key.
    new_key_str = qn_cs_duplicate(new_key);
    if (!new_key_str) return qn_false;

    qn_str_destroy(obj->itm[old_pos].key);
    obj->itm[old_pos].key = new_key_str;

    if (old_pos == new_pos) return qn_true; // -- The two keys reside in the same position.

    tmp_item = obj->itm[old_pos];
    if (old_pos < new_pos) {
        // -- The old key resides in a position before the new key.
        new_pos -= 1;
        if (old_pos < new_pos) memmove(&obj->itm[old_pos], &obj->itm[old_pos+1], sizeof(qn_json_obj_item) * (new_pos - old_pos));
    } else {
        // -- The old key resides in a position after the new key.
        memmove(&obj->itm[new_pos+1], &obj->itm[new_pos], sizeof(qn_json_obj_item) * (old_pos - new_pos));
    } // if
    obj->itm[new_pos] = tmp_item;
    return qn_true;
}

QN_SDK qn_bool qn_json_push_variant(qn_json_array_ptr restrict arr, qn_json_class cls, qn_json_variant new_elem)
{
    assert(arr);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if ((arr->end == arr->cap) && !qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;

    arr->itm[arr->end].elem = new_elem;
    arr->itm[arr->end].class = cls;
    arr->end += 1;
    arr->cnt += 1;
    return qn_true;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Pop and destroy the tail element of the array.
*
* @param [in] arr The non-NULL pointer to the target array.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_pop(qn_json_array_ptr restrict arr)
{
    if (arr->cnt > 0) {
        arr->end -= 1;
        qn_json_destroy_element(arr->itm[arr->end].class, &arr->itm[arr->end].elem);
        arr->cnt -= 1;
    } // if
}

QN_SDK qn_bool qn_json_unshift_variant(qn_json_array_ptr restrict arr, qn_json_class cls, qn_json_variant new_elem)
{
    assert(arr);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if ((arr->begin == 0) && !qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;

    arr->begin -= 1;
    arr->itm[arr->begin].elem = new_elem;
    arr->itm[arr->begin].class = cls;
    arr->cnt += 1;
    return qn_true;
}

/***************************************************************************//**
* @ingroup JSON-Array
*
* Shift and destroy the head element of the array.
*
* @param [in] arr The non-NULL pointer to the target array.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_shift(qn_json_array_ptr restrict arr)
{
    if (arr->cnt > 0) {
        qn_json_destroy_element(arr->itm[arr->begin].class, &arr->itm[arr->begin].elem);
        arr->begin += 1;
        arr->cnt -= 1;
    } // if
}

QN_SDK qn_bool qn_json_replace_variant(qn_json_array_ptr restrict arr, int n, qn_json_class cls, qn_json_variant new_elem)
{
    assert(arr);
    assert(0 <= n);

    if (arr->cap == 0) {
        qn_err_json_set_modifying_immutable_array();
        return qn_false;
    } // if

    if (n < 0 || (arr->end - arr->begin) <= n) {
        qn_err_json_set_out_of_index();
        return qn_false;
    } // if

    qn_json_destroy_element(arr->itm[arr->begin + n].class, &arr->itm[arr->begin + n].elem);
    arr->itm[arr->begin + n].elem = new_elem;
    arr->itm[arr->begin + n].class = cls;
    return qn_true;
}

// ---- Inplementation of iterator of JSON ----

typedef struct _QN_JSON_ITR_LEVEL
{
    qn_json_pos pos;
    qn_json_class class;
    qn_json_variant parent;
    qn_uint32 status;
} qn_json_itr_level, *qn_json_itr_level_ptr;

typedef struct _QN_JSON_ITERATOR
{
    int cnt;
    int cap;
    qn_json_itr_level * lvl;
    qn_json_itr_level init_lvl[3];
} qn_json_iterator;

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Allocate and construct a new JSON iterator.
*
* @retval non-NULL A pointer to the new iterator.
* @retval NULL Failed in creation and an error code is set.
*******************************************************************************/
QN_SDK qn_json_iterator_ptr qn_json_itr_create(void)
{
    qn_json_iterator_ptr new_itr = calloc(1, sizeof(qn_json_iterator));
    if (!new_itr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_itr->cap = sizeof(new_itr->init_lvl) / sizeof(new_itr->init_lvl[0]);
    new_itr->lvl = &new_itr->init_lvl[0];
    return new_itr;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Destruct and deallocate a JSON iterator.
*
* @param [in] itr The pointer to the iterator to destroy.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_itr_destroy(qn_json_iterator_ptr restrict itr)
{
    if (itr) {
        if (itr->lvl != &itr->init_lvl[0]) {
            free(itr->lvl);
        } // if
        free(itr);
    } // if
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Reset the given JSON iterator for next iteration.
*
* @param [in] itr The pointer to the iterator to reset.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_itr_reset(qn_json_iterator_ptr restrict itr)
{
    itr->cnt = 0;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Rewind the current level for a new iteration.
*
* @param [in] itr The pointer to the iterator to rewind.
* @retval NONE
*******************************************************************************/
QN_SDK void qn_json_itr_rewind(qn_json_iterator_ptr restrict itr)
{
    if (itr->cnt <= 0) return;
    itr->lvl[itr->cnt - 1].pos = 0;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Test the given iterator whether it is in use.
*
* @param [in] itr The pointer to the iterator to test.
* @retval true The iterator is not in use.
* @retval false The iterator is in use.
*******************************************************************************/
QN_SDK qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr restrict itr)
{
    return itr->cnt == 0;
}

/***************************************************************************//**
* @ingroup JSON-Iterator
*
* Get the count that how many pairs or values of the current level has been
* iterated.
*
* @param [in] itr The pointer to the iterator.
* @retval ANY The count of iterated pairs or values of the current level.
*******************************************************************************/
QN_SDK int qn_json_itr_done_steps(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0) ? 0 : itr->lvl[itr->cnt - 1].pos;
}

QN_SDK qn_string qn_json_itr_get_key(qn_json_iterator_ptr restrict itr)
{
    qn_json_itr_level_ptr lvl = NULL;

    if (itr->cnt == 0) return NULL;
    lvl = &itr->lvl[itr->cnt - 1];

    if (lvl->class != QN_JSON_OBJECT) return NULL;
    return lvl->parent.object->itm[lvl->pos - 1].key;
}

QN_SDK void qn_json_itr_set_status(qn_json_iterator_ptr restrict itr, qn_uint32 sts)
{
    if (itr->cnt) itr->lvl[itr->cnt - 1].status = sts;
}

QN_SDK qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt == 0) ? 0 : itr->lvl[itr->cnt - 1].status;
}

static qn_bool qn_json_itr_augment_levels(qn_json_iterator_ptr restrict itr)
{
    int new_capacity = itr->cap + (itr->cap >> 1); // 1.5 times of the last stack capacity.
    qn_json_itr_level_ptr new_lvl = calloc(1, sizeof(qn_json_itr_level) * new_capacity);
    if (!new_lvl) {
        qn_err_set_out_of_memory();
        return qn_false;
    }  // if

    memcpy(new_lvl, itr->lvl, sizeof(qn_json_itr_level) * itr->cnt);
    if (itr->lvl != &itr->init_lvl[0]) free(itr->lvl);
    itr->lvl = new_lvl;
    itr->cap = new_capacity;
    return qn_true;
}

QN_SDK qn_bool qn_json_itr_push_object(qn_json_iterator_ptr restrict itr, qn_json_object_ptr restrict obj)
{
    if ((itr->cnt + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->cnt].class = QN_JSON_OBJECT;
    itr->lvl[itr->cnt].parent.object = obj;
    itr->cnt += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

QN_SDK qn_bool qn_json_itr_push_array(qn_json_iterator_ptr restrict itr, qn_json_array_ptr restrict arr)
{
    if ((itr->cnt + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->cnt].class = QN_JSON_ARRAY;
    itr->lvl[itr->cnt].parent.array = arr;
    itr->cnt += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

QN_SDK void qn_json_itr_pop(qn_json_iterator_ptr restrict itr)
{
    if (itr->cnt > 0) itr->cnt -= 1;
}

QN_SDK qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0 || itr->lvl[itr->cnt - 1].class != QN_JSON_OBJECT) ? NULL : itr->lvl[itr->cnt - 1].parent.object;
}

QN_SDK qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr restrict itr)
{
    return (itr->cnt <= 0 || itr->lvl[itr->cnt - 1].class != QN_JSON_ARRAY) ? NULL : itr->lvl[itr->cnt - 1].parent.array;
}

QN_SDK int qn_json_itr_advance(qn_json_iterator_ptr restrict itr, void * data, qn_json_itr_callback_fn cb)
{
    qn_json_class class;
    qn_json_variant elem;
    qn_json_itr_level_ptr lvl;

    if (itr->cnt <= 0) return QN_JSON_ITR_NO_MORE;

    lvl = &itr->lvl[itr->cnt - 1];
    if (lvl->class == QN_JSON_OBJECT) {
        if (lvl->pos < lvl->parent.object->cnt) {
            class = lvl->parent.object->itm[lvl->pos].class;
            elem = lvl->parent.object->itm[lvl->pos].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        } // if
    } else {
        if (lvl->pos < lvl->parent.array->cnt) {
            class = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].class;
            elem = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        } // if
    } // if
    return cb(data, class, &elem);
}

#ifdef __cplusplus
}
#endif
