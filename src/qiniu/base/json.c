#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned short int qn_json_pos;

// ---- Inplementation of object of JSON ----

typedef struct _QN_JSON_OBJ_INDEX
{
    qn_json_hash hash;
    qn_json_pos pos;
} qn_json_obj_index;

typedef struct _QN_JSON_OBJ_ITEM
{
    qn_string key;
    qn_json_class class;
    qn_json_variant elem;
} qn_json_obj_item;

typedef struct _QN_JSON_OBJECT
{
    qn_json_obj_index * idx;
    qn_json_obj_item * itm;
    qn_json_pos cnt;
    qn_json_pos cap;

    qn_json_obj_index init_idx[2];
    qn_json_obj_item init_itm[2];
} qn_json_object;

static qn_json_hash qn_json_obj_calculate_hash(const char * cstr)
{
    qn_json_hash hash = 5381;
    int c;

    while ((c = *cstr++) != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } // while
    return hash;
}

qn_json_object_ptr qn_json_create_object(void)
{
    qn_json_object_ptr new_obj = calloc(1, sizeof(qn_json_object));
    if (!new_obj) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_obj->idx = &new_obj->init_idx[0];
    new_obj->itm = &new_obj->init_itm[0];
    new_obj->cap = sizeof(new_obj->init_idx) / sizeof(new_obj->init_idx[0]);
    return new_obj;
}

void qn_json_destroy_object(qn_json_object_ptr obj)
{
    qn_json_pos i = 0;

    for (i = 0; i < obj->cnt; i += 1) {
        switch (obj->itm[i].class) {
            case QN_JSON_OBJECT: qn_json_destroy_object(obj->itm[i].elem.object); break;
            case QN_JSON_ARRAY:  qn_json_destroy_array(obj->itm[i].elem.array); break;
            case QN_JSON_STRING: qn_str_destroy(obj->itm[i].elem.string); break;
            default: break;
        } // switch
        free(obj->itm[i].key);
    } // for
    if (obj->idx != &obj->init_idx[0]) {
        free(obj->idx);
    } // if
    if (obj->itm != &obj->init_itm[0]) {
        free(obj->itm);
    } // if
    free(obj);
}

static qn_json_pos qn_json_obj_bsearch(qn_json_obj_index * idx, qn_json_pos cnt, qn_json_hash hash)
{
    qn_json_pos begin = 0;
    qn_json_pos end = cnt;
    qn_json_pos mid = 0;
    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        if (idx[mid].hash < hash) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    return begin;
}

static qn_json_pos qn_json_obj_find(qn_json_object_ptr obj, qn_json_hash hash, const char * key)
{
    qn_json_pos i = 0;

    i = qn_json_obj_bsearch(obj->idx, obj->cnt, hash);
    while (i < obj->cnt && obj->idx[i].hash == hash && qn_str_compare_raw(obj->itm[ obj->idx[i].pos ].key, key) != 0) {
        i += 1;
    } // while
    return obj->idx[i].pos;
}

static qn_bool qn_json_obj_augment(qn_json_object_ptr obj)
{
    qn_json_pos new_cap = obj->cap * 2;
    qn_json_obj_index * new_idx = NULL;
    qn_json_obj_item * new_itm = NULL;

    new_idx = calloc(1, sizeof(qn_json_obj_index) * new_cap);
    if (!new_idx) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    new_itm = calloc(1, sizeof(qn_json_obj_item) * new_cap);
    if (!new_itm) {
        free(new_idx);
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_idx, obj->idx, sizeof(qn_json_obj_index) * obj->cnt);
    if (obj->idx != &obj->init_idx[0]) free(obj->idx);

    memcpy(new_itm, obj->itm, sizeof(qn_json_obj_item) * obj->cnt);
    if (obj->itm != &obj->init_itm[0]) free(obj->itm);

    obj->idx = new_idx;
    obj->itm = new_itm;
    obj->cap = new_cap;
    return qn_true;
}

static qn_bool qn_json_obj_set(qn_json_object_ptr obj, const char * key, qn_json_class cls, qn_json_variant new_elem)
{
    qn_json_hash hash = 0;
    qn_size pos = 0;

    hash = qn_json_obj_calculate_hash(key);
    pos = qn_json_obj_find(obj, hash, key);
    if (pos != obj->cnt) {
        // There is an element according to the given key.
        if (obj->itm[pos].class == QN_JSON_OBJECT) {
            qn_json_destroy_object(obj->itm[pos].elem.object);
        } else if (obj->itm[pos].class == QN_JSON_ARRAY) {
            qn_json_destroy_array(obj->itm[pos].elem.array);
        } else if (obj->itm[pos].class == QN_JSON_STRING) {
            qn_str_destroy(obj->itm[pos].elem.string);
        } // if
        obj->itm[pos].class = cls;
        obj->itm[pos].elem = new_elem;
        return qn_true;
    } // if

    if ((obj->cap - obj->cnt) <= 0 && !qn_json_obj_augment(obj)) return qn_false;

    if (!(obj->itm[obj->cnt].key = qn_str_duplicate(key))) return qn_false;
    obj->itm[obj->cnt].class = cls;
    obj->itm[obj->cnt].elem = new_elem;

    pos = qn_json_obj_bsearch(obj->idx, obj->cnt, hash);
    if (pos < obj->cnt) memmove(&obj->idx[pos+1], &obj->idx[pos], sizeof(qn_json_obj_index) * (obj->cnt - pos));
    obj->idx[pos].hash = hash;
    obj->idx[pos].pos = obj->cnt;

    obj->cnt += 1;
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

qn_json_array_ptr qn_json_create_array(void)
{
    qn_json_array_ptr new_arr = calloc(1, sizeof(qn_json_array));
    if (!new_arr) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_arr->itm = &new_arr->init_itm[0];
    new_arr->cap = sizeof(new_arr->init_itm) / sizeof(new_arr->init_itm[0]);
    return new_arr;
}

void qn_json_destroy_array(qn_json_array_ptr arr)
{
    qn_json_pos i = 0;

    for (i = arr->begin; i < arr->end; i += 1) {
        switch (arr->itm[i].class) {
            case QN_JSON_OBJECT: qn_json_destroy_object(arr->itm[i].elem.object); break;
            case QN_JSON_ARRAY:  qn_json_destroy_array(arr->itm[i].elem.array); break;
            case QN_JSON_STRING: qn_str_destroy(arr->itm[i].elem.string); break;
            default: break;
        } // switch
    } // for
    if (arr->itm != &arr->init_itm[0]) {
        free(arr->itm);
    } // if
    free(arr);
}

enum
{
    QN_JSON_ARR_PUSHING = 0,
    QN_JSON_ARR_UNSHIFTING = 1
};

static qn_bool qn_json_arr_augment(qn_json_array_ptr arr, int direct)
{
    qn_json_pos new_cap = arr->cap * 2;
    qn_json_arr_item * new_itm = calloc(1, sizeof(qn_json_arr_item) * new_cap);
    if (!new_itm) {
        qn_err_set_no_enough_memory();
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

static inline qn_json_pos qn_json_arr_find(qn_json_array_ptr arr, int n)
{
    return (n < 0 || n >= arr->cnt) ? arr->cnt : (arr->begin + n);
}

static qn_bool qn_json_arr_push(qn_json_array_ptr arr, qn_json_class cls, qn_json_variant new_elem)
{
    if ((arr->end == arr->cap) && !qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;
    arr->itm[arr->end].elem = new_elem;
    arr->itm[arr->end].class = cls;
    arr->end += 1;
    arr->cnt += 1;
    return qn_true;
}

static qn_bool qn_json_arr_unshift(qn_json_array_ptr arr, qn_json_class cls, qn_json_variant new_elem)
{
    if ((arr->begin == 0) && !qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;
    arr->begin -= 1;
    arr->itm[arr->begin].elem = new_elem;
    arr->itm[arr->begin].class = cls;
    arr->cnt += 1;
    return qn_true;
}

// ---- Inplementation of JSON ----

qn_json_object_ptr qn_json_create_and_set_object(qn_json_object_ptr obj, const char * key)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && !qn_json_obj_set(obj, key, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

qn_json_array_ptr qn_json_create_and_set_array(qn_json_object_ptr obj, const char * key)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && !qn_json_obj_set(obj, key, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

qn_json_object_ptr qn_json_create_and_push_object(qn_json_array_ptr arr)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && !qn_json_arr_push(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

qn_json_array_ptr qn_json_create_and_push_array(qn_json_array_ptr arr)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && !qn_json_arr_push(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

qn_json_object_ptr qn_json_create_and_unshift_object(qn_json_array_ptr arr)
{
    qn_json_variant new_elem;
    new_elem.object = qn_json_create_object();
    if (new_elem.object && !qn_json_arr_unshift(arr, QN_JSON_OBJECT, new_elem)) {
        qn_json_destroy_object(new_elem.object);
        return NULL;
    } // if
    return new_elem.object;
}

qn_json_array_ptr qn_json_create_and_unshift_array(qn_json_array_ptr arr)
{
    qn_json_variant new_elem;
    new_elem.array = qn_json_create_array();
    if (new_elem.array && !qn_json_arr_unshift(arr, QN_JSON_ARRAY, new_elem)) {
        qn_json_destroy_array(new_elem.array);
        return NULL;
    } // if
    return new_elem.array;
}

qn_size qn_json_size_object(qn_json_object_ptr obj)
{
    return obj->cnt;
}

qn_size qn_json_size_array(qn_json_array_ptr arr)
{
    return arr->cnt;
}

qn_json_object_ptr qn_json_get_object(qn_json_object_ptr obj, const char * key, qn_json_object_ptr default_val)
{
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key);
    return (pos == obj->cnt || obj->itm[pos].class != QN_JSON_OBJECT) ? default_val : obj->itm[pos].elem.object;
}

qn_json_array_ptr qn_json_get_array(qn_json_object_ptr obj, const char * key, qn_json_array_ptr default_val)
{
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key);
    return (pos == obj->cnt || obj->itm[pos].class != QN_JSON_ARRAY) ? default_val : obj->itm[pos].elem.array;
}

qn_string qn_json_get_string(qn_json_object_ptr obj, const char * key, qn_string default_val)
{
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key);
    return (pos == obj->cnt || obj->itm[pos].class != QN_JSON_STRING) ? default_val : obj->itm[pos].elem.string;
}

qn_integer qn_json_get_integer(qn_json_object_ptr obj, const char * key, qn_integer default_val)
{
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key);
    return (pos == obj->cnt || obj->itm[pos].class != QN_JSON_INTEGER) ? default_val : obj->itm[pos].elem.integer;
}

qn_number qn_json_get_number(qn_json_object_ptr obj, const char * key, qn_number default_val)
{
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key);
    return (pos == obj->cnt || obj->itm[pos].class != QN_JSON_NUMBER) ? default_val : obj->itm[pos].elem.number;
}

qn_bool qn_json_get_boolean(qn_json_object_ptr obj, const char * key, qn_bool default_val)
{
    qn_json_pos pos = qn_json_obj_find(obj, qn_json_obj_calculate_hash(key), key);
    return (pos == obj->cnt || obj->itm[pos].class != QN_JSON_BOOLEAN) ? default_val : obj->itm[pos].elem.boolean;
}

qn_json_object_ptr qn_json_pick_object(qn_json_array_ptr arr, int n, qn_json_object_ptr default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_OBJECT) ? default_val : arr->itm[pos].elem.object;
}

qn_json_array_ptr qn_json_pick_array(qn_json_array_ptr arr, int n, qn_json_array_ptr default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_ARRAY) ? default_val : arr->itm[pos].elem.array;
}

qn_string qn_json_pick_string(qn_json_array_ptr arr, int n, qn_string default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_STRING) ? default_val : arr->itm[pos].elem.string;
}

qn_integer qn_json_pick_integer(qn_json_array_ptr arr, int n, qn_integer default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_INTEGER) ? default_val : arr->itm[pos].elem.integer;
}

qn_number qn_json_pick_number(qn_json_array_ptr arr, int n, qn_number default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_NUMBER) ? default_val : arr->itm[pos].elem.number;
}

qn_bool qn_json_pick_boolean(qn_json_array_ptr arr, int n, qn_bool default_val)
{
    qn_json_pos pos = qn_json_arr_find(arr, n);
    return (pos == arr->cnt || arr->itm[pos].class != QN_JSON_BOOLEAN) ? default_val : arr->itm[pos].elem.boolean;
}

qn_bool qn_json_set_string(qn_json_object_ptr obj, const char * key, qn_string val)
{
    qn_json_variant elem;
    elem.string = qn_str_duplicate(val);
    return (!elem.string) ? qn_false : qn_json_obj_set(obj, key, QN_JSON_STRING, elem);
}

qn_bool qn_json_set_string_raw(qn_json_object_ptr obj, const char * key, const char * val, int size)
{
    qn_json_variant elem;
    elem.string = qn_str_clone(val, (size < 0) ? strlen(val) : size);
    return (!elem.string) ? qn_false : qn_json_obj_set(obj, key, QN_JSON_STRING, elem);
}

qn_bool qn_json_set_integer(qn_json_object_ptr obj, const char * key, qn_integer val)
{
    qn_json_variant elem;
    elem.integer = val;
    return qn_json_obj_set(obj, key, QN_JSON_INTEGER, elem);
}

qn_bool qn_json_set_number(qn_json_object_ptr obj, const char * key, qn_number val)
{
    qn_json_variant elem;
    elem.number = val;
    return qn_json_obj_set(obj, key, QN_JSON_NUMBER, elem);
}

qn_bool qn_json_set_boolean(qn_json_object_ptr obj, const char * key, qn_bool val)
{
    qn_json_variant elem;
    elem.boolean = val;
    return qn_json_obj_set(obj, key, QN_JSON_BOOLEAN, elem);
}

qn_bool qn_json_set_null(qn_json_object_ptr obj, const char * key)
{
    qn_json_variant elem;
    elem.integer = 0;
    return qn_json_obj_set(obj, key, QN_JSON_NULL, elem);
}

void qn_json_unset(qn_json_object_ptr obj, const char * key)
{
    qn_json_hash hash = 0;
    qn_json_pos pos = 0;
    qn_json_pos i = 0;

    if (obj->cnt == 0) return;

    hash = qn_json_obj_calculate_hash(key);
    pos = qn_json_obj_find(obj, hash, key);
    if (pos == obj->cnt) return; // There is no element according to the given key.

    if (obj->itm[pos].class == QN_JSON_OBJECT) {
        qn_json_destroy_object(obj->itm[pos].elem.object);
    } else if (obj->itm[pos].class == QN_JSON_ARRAY) {
        qn_json_destroy_array(obj->itm[pos].elem.array);
    }
    free((void*)obj->itm[pos].key);

    for (i = pos; i + 1 < obj->cnt; i += 1) {
        obj->itm[i] = obj->itm[i + 1];
    } // for
    obj->cnt -= 1;
}

qn_bool qn_json_push_string(qn_json_array_ptr arr, qn_string val)
{
    qn_json_variant elem;
    elem.string = qn_str_duplicate(val);
    return (!elem.string) ? qn_false : qn_json_arr_push(arr, QN_JSON_STRING, elem);
}

qn_bool qn_json_push_string_raw(qn_json_array_ptr arr, const char * val, int size)
{
    qn_json_variant elem;
    elem.string = qn_str_clone(val, (size < 0) ? strlen(val) : size);
    return (!elem.string) ? qn_false : qn_json_arr_push(arr, QN_JSON_STRING, elem);
}

qn_bool qn_json_push_integer(qn_json_array_ptr arr, qn_integer val)
{
    qn_json_variant elem;
    elem.integer = val;
    return qn_json_arr_push(arr, QN_JSON_INTEGER, elem);
}

qn_bool qn_json_push_number(qn_json_array_ptr arr, qn_number val)
{
    qn_json_variant elem;
    elem.number = val;
    return qn_json_arr_push(arr, QN_JSON_NUMBER, elem);
}

qn_bool qn_json_push_boolean(qn_json_array_ptr arr, qn_bool val)
{
    qn_json_variant elem;
    elem.boolean = val;
    return qn_json_arr_push(arr, QN_JSON_BOOLEAN, elem);
}

qn_bool qn_json_push_null(qn_json_array_ptr arr)
{
    qn_json_variant elem;
    elem.integer = 0;
    return qn_json_arr_push(arr, QN_JSON_NULL, elem);
}

void qn_json_pop(qn_json_array_ptr arr)
{
    if (arr->cnt > 0) {
        arr->end -= 1;
        if (arr->itm[arr->end].class == QN_JSON_OBJECT) {
            qn_json_destroy_object(arr->itm[arr->end].elem.object);
        } else if (arr->itm[arr->end].class == QN_JSON_ARRAY) {
            qn_json_destroy_array(arr->itm[arr->end].elem.array);
        }
        arr->cnt -= 1;
    } // if
}

qn_bool qn_json_unshift_string(qn_json_array_ptr arr, qn_string val)
{
    qn_json_variant elem;
    elem.string = qn_str_duplicate(val);
    return (!elem.string) ? qn_false : qn_json_arr_unshift(arr, QN_JSON_STRING, elem);
}

qn_bool qn_json_unshift_string_raw(qn_json_array_ptr arr, const char * val, int size)
{
    qn_json_variant elem;
    elem.string = qn_str_clone(val, (size < 0) ? strlen(val) : size);
    return (!elem.string) ? qn_false : qn_json_arr_unshift(arr, QN_JSON_STRING, elem);
}

qn_bool qn_json_unshift_integer(qn_json_array_ptr arr, qn_integer val)
{
    qn_json_variant elem;
    elem.integer = val;
    return qn_json_arr_unshift(arr, QN_JSON_INTEGER, elem);
}

qn_bool qn_json_unshift_number(qn_json_array_ptr arr, qn_number val)
{
    qn_json_variant elem;
    elem.number = val;
    return qn_json_arr_unshift(arr, QN_JSON_NUMBER, elem);
}

qn_bool qn_json_unshift_boolean(qn_json_array_ptr arr, qn_bool val)
{
    qn_json_variant elem;
    elem.boolean = val;
    return qn_json_arr_unshift(arr, QN_JSON_BOOLEAN, elem);
}

qn_bool qn_json_unshift_null(qn_json_array_ptr arr)
{
    qn_json_variant elem;
    elem.integer = 0;
    return qn_json_arr_unshift(arr, QN_JSON_NULL, elem);
}

void qn_json_shift(qn_json_array_ptr arr)
{
    if (arr->cnt > 0) {
        if (arr->itm[arr->begin].class == QN_JSON_OBJECT) {
            qn_json_destroy_object(arr->itm[arr->begin].elem.object);
        } else if (arr->itm[arr->begin].class == QN_JSON_ARRAY) {
            qn_json_destroy_array(arr->itm[arr->begin].elem.array);
        }
        arr->begin += 1;
        arr->cnt -= 1;
    } // if
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
    int size;
    int cap;
    qn_json_itr_level * lvl;
    qn_json_itr_level init_lvl[3];
} qn_json_iterator;

qn_json_iterator_ptr qn_json_itr_create(void)
{
    qn_json_iterator_ptr new_itr = calloc(1, sizeof(qn_json_iterator));
    if (!new_itr) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_itr->cap = sizeof(new_itr->init_lvl) / sizeof(new_itr->init_lvl[0]);
    new_itr->lvl = &new_itr->init_lvl[0];
    return new_itr;
}

void qn_json_itr_destroy(qn_json_iterator_ptr itr)
{
    if (itr) {
        if (itr->lvl != &itr->init_lvl[0]) {
            free(itr->lvl);
        } // if
        free(itr);
    } // if
}

void qn_json_itr_reset(qn_json_iterator_ptr itr)
{
    itr->size = 0;
}

void qn_json_itr_rewind(qn_json_iterator_ptr itr)
{
    if (itr->size <= 0) return;
    itr->lvl[itr->size - 1].pos = 0;
}

qn_bool qn_json_itr_is_empty(qn_json_iterator_ptr itr)
{
    return itr->size == 0;
}

int qn_json_itr_steps(qn_json_iterator_ptr itr)
{
    return (itr->size <= 0) ? 0 : itr->lvl[itr->size - 1].pos;
}

qn_string qn_json_itr_get_key(qn_json_iterator_ptr itr)
{
    qn_json_itr_level_ptr lvl = NULL;

    if (itr->size == 0) return NULL;
    lvl = &itr->lvl[itr->size - 1];

    if (lvl->class != QN_JSON_OBJECT) return NULL;
    return lvl->parent.object->itm[lvl->pos - 1].key;
}

qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr itr)
{
    return (itr->size == 0) ? 0 : itr->lvl[itr->size - 1].status;
}

void qn_json_itr_set_status(qn_json_iterator_ptr itr, qn_uint32 sts)
{
    if (itr->size) {
        itr->lvl[itr->size - 1].status = sts;
    } // if
}

static qn_bool qn_json_itr_augment_levels(qn_json_iterator_ptr itr)
{
    int new_capacity = itr->cap + (itr->cap >> 1); // 1.5 times of the last stack capacity.
    qn_json_itr_level_ptr new_lvl = calloc(1, sizeof(qn_json_itr_level) * new_capacity);
    if (!new_lvl) {
        qn_err_set_no_enough_memory();
        return qn_false;
    }  // if

    memcpy(new_lvl, itr->lvl, sizeof(qn_json_itr_level) * itr->size);
    if (itr->lvl != &itr->init_lvl[0]) free(itr->lvl);
    itr->lvl = new_lvl;
    itr->cap = new_capacity;
    return qn_true;
}

qn_bool qn_json_itr_push_object(qn_json_iterator_ptr itr, qn_json_object_ptr obj)
{
    if ((itr->size + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->size].class = QN_JSON_OBJECT;
    itr->lvl[itr->size].parent.object = obj;
    itr->size += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

qn_bool qn_json_itr_push_array(qn_json_iterator_ptr itr, qn_json_array_ptr arr)
{
    if ((itr->size + 1) > itr->cap && !qn_json_itr_augment_levels(itr)) return qn_false;

    itr->lvl[itr->size].class = QN_JSON_ARRAY;
    itr->lvl[itr->size].parent.array = arr;
    itr->size += 1;
    qn_json_itr_rewind(itr);
    return qn_true;
}

void qn_json_itr_pop(qn_json_iterator_ptr itr)
{
    if (itr->size > 0) itr->size -= 1;
}

qn_json_object_ptr qn_json_itr_top_object(qn_json_iterator_ptr itr)
{
    return (itr->size <= 0 || itr->lvl[itr->size - 1].class != QN_JSON_OBJECT) ? NULL : itr->lvl[itr->size - 1].parent.object;
}

qn_json_array_ptr qn_json_itr_top_array(qn_json_iterator_ptr itr)
{
    return (itr->size <= 0 || itr->lvl[itr->size - 1].class != QN_JSON_ARRAY) ? NULL : itr->lvl[itr->size - 1].parent.array;
}

int qn_json_itr_advance(qn_json_iterator_ptr itr, void * data, qn_json_itr_callback cb)
{
    qn_json_class class;
    qn_json_variant elem;
    qn_json_itr_level_ptr lvl = NULL;

    if (itr->size <= 0) return QN_JSON_ITR_NO_MORE;

    lvl = &itr->lvl[itr->size - 1];
    if (lvl->class == QN_JSON_OBJECT) {
        if (lvl->pos < lvl->parent.object->cnt) {
            class = lvl->parent.object->itm[lvl->pos].class;
            elem = lvl->parent.object->itm[lvl->pos].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        }
    } else {
        if (lvl->pos < lvl->parent.array->cnt) {
            class = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].class;
            elem = lvl->parent.array->itm[lvl->pos + lvl->parent.array->begin].elem;
            lvl->pos += 1;
        } else {
            return QN_JSON_ITR_NO_MORE;
        }
    }
    return cb(data, class, &elem);
}

#ifdef __cplusplus
}
#endif
