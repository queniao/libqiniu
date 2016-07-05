#include <assert.h>

#include "base/string.h"
#include "base/json.h"
#include "base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned short int qn_json_pos;

// ---- Inplementation of object of JSON ----

typedef struct _QN_JSON_OBJ_INDEX
{
    qn_json_ptr elem;
    qn_string key;
    qn_json_hash hash;
} qn_json_obj_index;

typedef struct _QN_JSON_OBJECT
{
    qn_json_obj_index * idx;
    qn_json_pos cnt;
    qn_json_pos cap;

    qn_json_obj_index init_idx[2];
} qn_json_object, *qn_json_object_ptr;

static qn_json_hash qn_json_obj_calculate_hash(const char * cstr)
{
    qn_json_hash hash = 5381;
    int c;

    while ((c = *cstr++) != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } // while
    return hash;
}

static qn_json_object_ptr qn_json_obj_create(void)
{
    qn_json_object_ptr new_obj = calloc(1, sizeof(qn_json_object));
    if (!new_obj) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_obj->idx = &new_obj->init_idx[0];
    new_obj->cap = sizeof(new_obj->init_idx) / sizeof(new_obj->init_idx[0]);
    return new_obj;
}

static void qn_json_obj_destroy(qn_json_object_ptr obj)
{
    qn_json_pos i = 0;

    for (i = 0; i < obj->cnt; i += 1) {
        qn_json_destroy(obj->idx[i].elem);
    } // for
    if (obj->idx != &obj->init_idx[0]) {
        free(obj->idx);
    } // if
}

static qn_json_pos qn_json_obj_find(qn_json_object_ptr obj, qn_json_hash hash, const char * key)
{
    qn_json_pos i = 0;

    for (i = 0; i < obj->cnt; i += 1) {
        if (obj->idx[i].hash == hash && (qn_str_compare_raw(obj->idx[i].key, key)) == 0) {
            break;
        } // if
    } // for
    return i;
}

static qn_bool qn_json_obj_augment(qn_json_object_ptr obj)
{
    qn_json_pos new_cap = obj->cap * 2;
    qn_json_obj_index * new_idx = calloc(1, sizeof(qn_json_obj_index) * new_cap);
    if (!new_idx) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_idx, obj->idx, sizeof(qn_json_obj_index) * obj->cnt);
    if (obj->idx != &obj->init_idx[0]) free(obj->idx);

    obj->idx = new_idx;
    obj->cap = new_cap;
    return qn_true;
}

static qn_bool qn_json_obj_set(qn_json_object_ptr obj, const char * key, qn_json_ptr new_elem)
{
    qn_json_ptr elem = NULL;
    qn_json_hash hash = 0;
    qn_size pos = 0;

    assert(key && new_elem);

    hash = qn_json_obj_calculate_hash(key);
    pos = qn_json_obj_find(obj, hash, key);
    if (pos != obj->cnt) {
        // There is an element according to the given key.
        qn_json_destroy(obj->idx[pos].elem);
        obj->idx[pos].elem = new_elem;
        return qn_true;
    } // if

    if ((obj->cap - obj->cnt) <= 0 && !qn_json_obj_augment(obj)) return qn_false;

    obj->idx[obj->cnt].hash = hash;
    obj->idx[obj->cnt].elem = new_elem;
    if (!(obj->idx[obj->cnt].key = qn_str_duplicate(key))) return qn_false;
    obj->cnt += 1;
    return qn_true;
}

// ---- Inplementation of array of JSON ----

typedef struct _QN_JSON_ARR_INDEX
{
    qn_json_ptr elem;
} qn_json_arr_index;

typedef struct _QN_JSON_ARRAY
{
    qn_json_arr_index * idx;
    qn_json_pos begin;
    qn_json_pos end;
    qn_json_pos cnt;
    qn_json_pos cap;

    qn_json_arr_index init_idx[4];
} qn_json_array, *qn_json_array_ptr;

static qn_json_array_ptr qn_json_arr_create(void)
{
    qn_json_array_ptr new_arr = calloc(1, sizeof(qn_json_array));
    if (!new_arr) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_arr->idx = &new_arr->init_idx[0];
    new_arr->cap = sizeof(new_arr->init_idx) / sizeof(new_arr->init_idx[0]);
    return new_arr;
}

static void qn_json_arr_destroy(qn_json_array_ptr arr)
{
    qn_json_pos i = 0;

    for (i = arr->begin; i < arr->end; i += 1) {
        qn_json_destroy(arr->idx[i].elem);
    } // for
    if (arr->idx != &arr->init_idx[0]) {
        free(arr->idx);
    } // if
}

enum
{
    QN_JSON_ARR_PUSHING = 0,
    QN_JSON_ARR_UNSHIFTING = 1
};

static qn_bool qn_json_arr_augment(qn_json_array_ptr arr, int direct)
{
    qn_json_pos new_cap = arr->cap * 2;
    qn_json_arr_index * new_idx = calloc(1, sizeof(qn_json_arr_index) * new_cap);
    if (!new_idx) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    if (direct == QN_JSON_ARR_PUSHING) {
        memcpy(new_idx + arr->begin, arr->idx + arr->begin, sizeof(qn_json_arr_index) * arr->cnt);
    } else {
        memcpy(new_idx + arr->begin + arr->cap, arr->idx + arr->begin, sizeof(qn_json_arr_index) * arr->cnt);
        arr->begin += arr->cap;
        arr->end += arr->cap;
    } // if
    if (arr->idx != &arr->init_idx[0]) free(arr->idx);

    arr->idx = new_idx;
    arr->cap = new_cap;
    return qn_true;
}

static inline qn_json_ptr qn_json_arr_find(qn_json_array_ptr arr, int n)
{
    return (n < 0 || n >= arr->cnt) ? NULL : arr->idx[arr->begin + n].elem;
}

static qn_bool qn_json_arr_push(qn_json_array_ptr arr, qn_json_ptr new_elem)
{
    assert(new_elem);
    if ((arr->end == arr->cap) && !qn_json_arr_augment(arr, QN_JSON_ARR_PUSHING)) return qn_false;
    arr->idx[arr->end++].elem = new_elem;
    arr->cnt += 1;
    return qn_true;
}

static qn_bool qn_json_arr_unshift(qn_json_array_ptr arr, qn_json_ptr new_elem)
{
    assert(new_elem);
    if ((arr->begin == 0) && !qn_json_arr_augment(arr, QN_JSON_ARR_UNSHIFTING)) return qn_false;
    arr->idx[--arr->begin].elem = new_elem;
    arr->cnt += 1;
    return qn_true;
}

// ---- Inplementation of JSON ----

typedef struct _QN_JSON
{
    struct {
        qn_json_class class:3;
    };
    union {
        qn_json_object * object;
        qn_json_array * array;
        qn_json_string string;
        qn_json_integer integer;
        qn_json_number number;
        qn_json_boolean boolean;
    };
} qn_json; // _QN_JSON

static qn_json_ptr qn_json_create_string(const char * s, int sz)
{
    qn_json_ptr new_str = malloc(sizeof(qn_json));
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_str->class = QN_JSON_STRING;
    new_str->string = qn_str_clone(s, sz);
    if (!new_str->string) {
        qn_err_set_no_enough_memory();
        free(new_str);
        return NULL;
    } // if
    return new_str;
}

static qn_json_ptr qn_json_create_integer(qn_integer val)
{
    qn_json_ptr new_int = malloc(sizeof(qn_json));
    if (!new_int) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_int->class = QN_JSON_INTEGER;
    new_int->integer = val;
    return new_int;
}

static qn_json_ptr qn_json_create_number(qn_number val)
{
    qn_json_ptr new_num = malloc(sizeof(qn_json));
    if (!new_num) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_num->class = QN_JSON_NUMBER;
    new_num->number = val;
    return new_num;
}

static qn_json_ptr qn_json_create_boolean(qn_bool val)
{
    qn_json_ptr new_bool = malloc(sizeof(qn_json));
    if (!new_bool) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_bool->class = QN_JSON_BOOLEAN;
    new_bool->boolean = val;
    return new_bool;
}

static qn_json_ptr qn_json_create_null(void)
{
    qn_json_ptr new_null = malloc(sizeof(qn_json));
    if (!new_null) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_null->class = QN_JSON_NULL;
    return new_null;
}

qn_json_ptr qn_json_create_object(void)
{
    qn_json_ptr new_obj = malloc(sizeof(qn_json));
    if (!new_obj) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_obj->object = qn_json_obj_create();
    if (!new_obj->object) {
        free(new_obj);
        return NULL;
    } // if

    new_obj->class = QN_JSON_OBJECT;
    return new_obj;
}

qn_json_ptr qn_json_create_array(void)
{
    qn_json_ptr new_arr = malloc(sizeof(qn_json));
    if (!new_arr) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_arr->array = qn_json_arr_create();
    if (!new_arr->array) {
        return NULL;
    }

    new_arr->class = QN_JSON_ARRAY;
    return new_arr;
}

qn_json_ptr qn_json_create_and_set_object(qn_json_ptr parent, const char * key)
{
    qn_json_ptr new_obj = qn_json_create_object();
    if (new_obj && !qn_json_obj_set(parent->object, key, new_obj)) {
        qn_json_destroy(new_obj);
        return NULL;
    } // if
    return new_obj;
}

qn_json_ptr qn_json_create_and_set_array(qn_json_ptr parent, const char * key)
{
    qn_json_ptr new_arr = qn_json_create_array();
    if (new_arr && !qn_json_obj_set(parent->object, key, new_arr)) {
        qn_json_destroy(new_arr);
        return NULL;
    } // if
    return new_arr;
}

qn_json_ptr qn_json_create_and_push_object(qn_json_ptr parent)
{
    qn_json_ptr new_obj = qn_json_create_object();
    if (new_obj && !qn_json_arr_push(parent->array, new_obj)) {
        qn_json_destroy(new_obj);
        return NULL;
    } // if
    return new_obj;
}

qn_json_ptr qn_json_create_and_push_array(qn_json_ptr parent)
{
    qn_json_ptr new_arr = qn_json_create_array();
    if (new_arr && !qn_json_arr_push(parent->array, new_arr)) {
        qn_json_destroy(new_arr);
        return NULL;
    } // if
    return new_arr;
}

qn_json_ptr qn_json_create_and_unshift_object(qn_json_ptr parent)
{
    qn_json_ptr new_obj = qn_json_create_object();
    if (new_obj && !qn_json_arr_unshift(parent->array, new_obj)) {
        qn_json_destroy(new_obj);
        return NULL;
    } // if
    return new_obj;
}

qn_json_ptr qn_json_create_and_unshift_array(qn_json_ptr parent)
{
    qn_json_ptr new_arr = qn_json_create_array();
    if (new_arr && !qn_json_arr_unshift(parent->array, new_arr)) {
        qn_json_destroy(new_arr);
        return NULL;
    } // if
    return new_arr;
}

void qn_json_destroy(qn_json_ptr self) {
    if (self) {
        switch (self->class) {
            case QN_JSON_OBJECT:
                qn_json_obj_destroy(self->object);
                break;

            case QN_JSON_ARRAY:
                qn_json_arr_destroy(self->array);
                break;

            default:
                break;
        } // if
        free(self);
    } // if
}

qn_size qn_json_size(qn_json_ptr self)
{
    if (self->class == QN_JSON_OBJECT) {
        return self->object->cnt;
    } // if
    if (self->class == QN_JSON_ARRAY) {
        return self->array->cnt;
    } // if
    return 0;
}

qn_bool qn_json_is_empty(qn_json_ptr self)
{
    if (self->class == QN_JSON_OBJECT) {
        return self->object->cnt == 0;
    }
    if (self->class == QN_JSON_ARRAY) {
        return self->array->cnt == 0;
    }
    return qn_false;
}

qn_bool qn_json_is_object(qn_json_ptr self)
{
    return (self->class == QN_JSON_OBJECT);
}

qn_bool qn_json_is_array(qn_json_ptr self)
{
    return (self->class == QN_JSON_ARRAY);
}

qn_json_ptr qn_json_get_object(qn_json_ptr self, const char * key, qn_json_ptr default_val)
{
    qn_json_pos pos = qn_json_obj_find(self->object, qn_json_obj_calculate_hash(key), key);
    return (pos == self->object->cnt || self->object->idx[pos].elem->class != QN_JSON_OBJECT) ? default_val : self->object->idx[pos].elem;
}

qn_json_ptr qn_json_get_array(qn_json_ptr self, const char * key, qn_json_ptr default_val)
{
    qn_json_pos pos = qn_json_obj_find(self->object, qn_json_obj_calculate_hash(key), key);
    return (pos == self->object->cnt || self->object->idx[pos].elem->class != QN_JSON_ARRAY) ? default_val : self->object->idx[pos].elem;
}

qn_string qn_json_get_string(qn_json_ptr self, const char * key, qn_string default_val)
{
    qn_json_pos pos = qn_json_obj_find(self->object, qn_json_obj_calculate_hash(key), key);
    return (pos == self->object->cnt || self->object->idx[pos].elem->class != QN_JSON_STRING) ? default_val : self->object->idx[pos].elem->string;
}

qn_integer qn_json_get_integer(qn_json_ptr self, const char * key, qn_integer default_val)
{
    qn_json_pos pos = qn_json_obj_find(self->object, qn_json_obj_calculate_hash(key), key);
    return (pos == self->object->cnt || self->object->idx[pos].elem->class != QN_JSON_INTEGER) ? default_val : self->object->idx[pos].elem->integer;
}

qn_number qn_json_get_number(qn_json_ptr self, const char * key, qn_number default_val)
{
    qn_json_pos pos = qn_json_obj_find(self->object, qn_json_obj_calculate_hash(key), key);
    return (pos == self->object->cnt || self->object->idx[pos].elem->class != QN_JSON_NUMBER) ? default_val : self->object->idx[pos].elem->number;
}

qn_bool qn_json_get_boolean(qn_json_ptr self, const char * key, qn_bool default_val)
{
    qn_json_pos pos = qn_json_obj_find(self->object, qn_json_obj_calculate_hash(key), key);
    return (pos == self->object->cnt || self->object->idx[pos].elem->class != QN_JSON_BOOLEAN) ? default_val : self->object->idx[pos].elem->boolean;
}

qn_json_ptr qn_json_pick_object(qn_json_ptr self, int n, qn_json_ptr default_val)
{
    qn_json_ptr elem = qn_json_arr_find(self->array, n);
    return (!elem || elem->class != QN_JSON_OBJECT) ? default_val : elem;
}

qn_json_ptr qn_json_pick_array(qn_json_ptr self, int n, qn_json_ptr default_val)
{
    qn_json_ptr elem = qn_json_arr_find(self->array, n);
    return (!elem || elem->class != QN_JSON_ARRAY) ? default_val : elem;
}

qn_string qn_json_pick_string(qn_json_ptr self, int n, qn_string default_val)
{
    qn_json_ptr elem = qn_json_arr_find(self->array, n);
    return (!elem || elem->class != QN_JSON_STRING) ? default_val : elem->string;
}

qn_integer qn_json_pick_integer(qn_json_ptr self, int n, qn_integer default_val)
{
    qn_json_ptr elem = qn_json_arr_find(self->array, n);
    return (!elem || elem->class != QN_JSON_INTEGER) ? default_val : elem->integer;
}

qn_number qn_json_pick_number(qn_json_ptr self, int n, qn_number default_val)
{
    qn_json_ptr elem = qn_json_arr_find(self->array, n);
    return (!elem || elem->class != QN_JSON_NUMBER) ? default_val : elem->number;
}

qn_bool qn_json_pick_boolean(qn_json_ptr self, int n, qn_bool default_val)
{
    qn_json_ptr elem = qn_json_arr_find(self->array, n);
    return (!elem || elem->class != QN_JSON_BOOLEAN) ? default_val : elem->boolean;
}

void qn_json_unset(qn_json_ptr self, const char * key)
{
    qn_json_object_ptr obj = self->object;
    qn_json_hash hash = 0;
    qn_json_pos pos = 0;
    qn_json_pos i = 0;

    if (obj->cnt == 0) return;

    hash = qn_json_obj_calculate_hash(key);
    pos = qn_json_obj_find(obj, hash, key);
    if (pos == obj->cnt) return; // There is no element according to the given key.

    qn_json_destroy(obj->idx[pos].elem);
    free((void*)obj->idx[pos].key);

    for (i = pos; i + 1 < obj->cnt; i += 1) {
        obj->idx[i] = obj->idx[i + 1];
    } // for
    obj->cnt -= 1;
}

void qn_json_pop(qn_json_ptr self)
{
    if (self->array->cnt > 0) {
        qn_json_destroy(self->array->idx[self->array->end - 1].elem);
        self->array->idx[--self->array->end].elem = NULL;
        self->array->cnt -= 1;
    } // if
}

void qn_json_shift(qn_json_ptr self)
{
    if (self->array->cnt > 0) {
        qn_json_destroy(self->array->idx[self->array->begin].elem);
        self->array->idx[self->array->begin++].elem = NULL;
        self->array->cnt -= 1;
    } // if
}

qn_bool qn_json_set_string(qn_json_ptr self, const char * key, qn_string val)
{
    qn_json_ptr elem = qn_json_create_string(qn_str_cstr(val), qn_str_size(val));
    return (!elem) ? qn_false : qn_json_obj_set(self->object, key, elem);
}

qn_bool qn_json_set_string_raw(qn_json_ptr self, const char * key, const char * val, int size)
{
    qn_json_ptr elem = qn_json_create_string(val, (size < 0) ? strlen(val) : size);
    if (!elem) return qn_false;
    return qn_json_obj_set(self->object, key, elem);
}

qn_bool qn_json_set_integer(qn_json_ptr self, const char * key, qn_integer val)
{
    qn_json_ptr elem = qn_json_create_integer(val);
    if (!elem) return qn_false;
    return qn_json_obj_set(self->object, key, elem);
}

qn_bool qn_json_set_number(qn_json_ptr self, const char * key, qn_number val)
{
    qn_json_ptr elem = qn_json_create_number(val);
    if (!elem) return qn_false;
    return qn_json_obj_set(self->object, key, elem);
}

qn_bool qn_json_set_boolean(qn_json_ptr self, const char * key, qn_bool val)
{
    qn_json_ptr elem = qn_json_create_boolean(val);
    if (!elem) return qn_false;
    return qn_json_obj_set(self->object, key, elem);
}

qn_bool qn_json_set_null(qn_json_ptr self, const char * key)
{
    qn_json_ptr elem = qn_json_create_null();
    if (!elem) return qn_false;
    return qn_json_obj_set(self->object, key, elem);
}

qn_bool qn_json_push_string(qn_json_ptr self, qn_string val)
{
    qn_json_ptr elem = qn_json_create_string(qn_str_cstr(val), qn_str_size(val));
    return (!elem) ? qn_false : qn_json_arr_push(self->array, elem);
}

qn_bool qn_json_push_string_raw(qn_json_ptr self, const char * val, int size)
{
    qn_json_ptr elem = qn_json_create_string(val, (size < 0) ? strlen(val) : size);
    return (!elem) ? qn_false : qn_json_arr_push(self->array, elem);
}

qn_bool qn_json_push_integer(qn_json_ptr self, qn_integer val)
{
    qn_json_ptr elem = qn_json_create_integer(val);
    return (!elem) ? qn_false : qn_json_arr_push(self->array, elem);
}

qn_bool qn_json_push_number(qn_json_ptr self, qn_number val)
{
    qn_json_ptr elem = qn_json_create_number(val);
    return (!elem) ? qn_false : qn_json_arr_push(self->array, elem);
}

qn_bool qn_json_push_boolean(qn_json_ptr self, qn_bool val)
{
    qn_json_ptr elem = qn_json_create_boolean(val);
    return (!elem) ? qn_false : qn_json_arr_push(self->array, elem);
}

qn_bool qn_json_push_null(qn_json_ptr self)
{
    qn_json_ptr elem = qn_json_create_null();
    return (!elem) ? qn_false : qn_json_arr_push(self->array, elem);
}

qn_bool qn_json_unshift_string(qn_json_ptr self, qn_string val)
{
    qn_json_ptr elem = qn_json_create_string(qn_str_cstr(val), qn_str_size(val));
    return (!elem) ? qn_false : qn_json_arr_unshift(self->array, elem);
}

qn_bool qn_json_unshift_string_raw(qn_json_ptr self, const char * val, int size)
{
    qn_json_ptr elem = qn_json_create_string(val, (size < 0) ? strlen(val) : size);
    return (!elem) ? qn_false : qn_json_arr_unshift(self->array, elem);
}

qn_bool qn_json_unshift_integer(qn_json_ptr self, qn_integer val)
{
    qn_json_ptr elem = qn_json_create_integer(val);
    return (!elem) ? qn_false : qn_json_arr_unshift(self->array, elem);
}

qn_bool qn_json_unshift_number(qn_json_ptr self, qn_number val)
{
    qn_json_ptr elem = qn_json_create_number(val);
    return (!elem) ? qn_false : qn_json_arr_unshift(self->array, elem);
}

qn_bool qn_json_unshift_boolean(qn_json_ptr self, qn_bool val)
{
    qn_json_ptr elem = qn_json_create_boolean(val);
    return (!elem) ? qn_false : qn_json_arr_unshift(self->array, elem);
}

qn_bool qn_json_unshift_null(qn_json_ptr self)
{
    qn_json_ptr elem = qn_json_create_null();
    return (!elem) ? qn_false : qn_json_arr_unshift(self->array, elem);
}

// ---- Inplementation of iterator of JSON ----

typedef struct _QN_JSON_ITR_LEVEL
{
    qn_json_pos pos;
    qn_json_ptr parent;
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

void qn_json_itr_destroy(qn_json_iterator_ptr self)
{
    if (self) {
        if (self->lvl != &self->init_lvl[0]) {
            free(self->lvl);
        } // if
        free(self);
    } // if
}

void qn_json_itr_reset(qn_json_iterator_ptr self)
{
    self->size = 0;
}

void qn_json_itr_rewind(qn_json_iterator_ptr self)
{
    if (self->size <= 0) return;
    self->lvl[self->size - 1].pos = 0;
}

int qn_json_itr_steps(qn_json_iterator_ptr self)
{
    return (self->size <= 0) ? 0 : self->lvl[self->size - 1].pos;
}

qn_string qn_json_itr_get_key(qn_json_iterator_ptr self)
{
    qn_json_itr_level_ptr lvl = NULL;
    if (self->size == 0) return NULL;
    lvl = &self->lvl[self->size - 1];
    if (lvl->parent->class != QN_JSON_OBJECT) return NULL;
    return lvl->parent->object->idx[lvl->pos - 1].key;
}

qn_uint32 qn_json_itr_get_status(qn_json_iterator_ptr self)
{
    return (self->size == 0) ? 0 : self->lvl[self->size - 1].status;
}

void qn_json_itr_set_status(qn_json_iterator_ptr self, qn_uint32 sts)
{
    if (self->size) {
        self->lvl[self->size - 1].status = sts;
    } // if
}

static qn_bool qn_json_itr_augment_levels(qn_json_iterator_ptr self)
{
    int new_capacity = self->cap + (self->cap >> 1); // 1.5 times of the last stack capacity.
    qn_json_itr_level_ptr new_lvl = calloc(1, sizeof(qn_json_itr_level) * new_capacity);
    if (!new_lvl) {
        qn_err_set_no_enough_memory();
        return qn_false;
    }  // if

    memcpy(new_lvl, self->lvl, sizeof(qn_json_itr_level) * self->size);
    if (self->lvl != &self->init_lvl[0]) free(self->lvl);
    self->lvl = new_lvl;
    self->cap = new_capacity;
    return qn_true;
}

qn_bool qn_json_itr_push(qn_json_iterator_ptr self, qn_json_ptr parent)
{
    if ((self->size + 1) > self->cap && !qn_json_itr_augment_levels(self)) return qn_false;

    self->lvl[self->size++].parent = parent;
    qn_json_itr_rewind(self);
    return qn_true;
}

void qn_json_itr_pop(qn_json_iterator_ptr self)
{
    if (self->size > 0) self->size -= 1;
}

qn_json_ptr qn_json_itr_top(qn_json_iterator_ptr self)
{
    return (self->size <= 0) ? NULL : self->lvl[self->size - 1].parent;
}

int qn_json_itr_advance(qn_json_iterator_ptr self, void * data, qn_json_itr_callback cb)
{
    qn_json_variant val;
    qn_json_ptr elem = NULL;
    qn_json_itr_level_ptr lvl = NULL;

    if (self->size <= 0) return QN_JSON_ITR_NO_MORE;

    lvl = &self->lvl[self->size - 1];
    if (lvl->parent->class == QN_JSON_OBJECT) {
        if (lvl->pos < lvl->parent->object->cnt) {
            elem = lvl->parent->object->idx[lvl->pos].elem;
            lvl->pos += 1;
        }
    } else {
        if (lvl->pos < lvl->parent->array->cnt) {
            elem = lvl->parent->array->idx[lvl->pos + lvl->parent->array->begin].elem;
            lvl->pos += 1;
        }
    }
    if (!elem) return cb(data, QN_JSON_UNKNOWN, NULL);

    switch (elem->class) {
        case QN_JSON_OBJECT: val.object = elem; break;
        case QN_JSON_ARRAY: val.array = elem; break;
        case QN_JSON_STRING: val.string = elem->string; break;
        case QN_JSON_INTEGER: val.integer = elem->integer; break;
        case QN_JSON_NUMBER: val.number = elem->number; break;
        case QN_JSON_BOOLEAN: val.boolean = elem->boolean; break;
        default: break;
    } // switch
    return cb(data, elem->class, &val);
}

#ifdef __cplusplus
}
#endif
