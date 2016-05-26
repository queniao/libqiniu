#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "base/ds/dqueue.h"

#include "base/string.h"
#include "base/json.h"
#include "base/errors.h"

#define qn_json_assert_complex_element(elem) assert(elem && (elem->class == QN_JSON_OBJECT || elem->class == QN_JSON_ARRAY))

#ifdef __cplusplus
extern "C"
{
#endif

typedef qn_bool qn_json_boolean;
typedef qn_string_ptr qn_json_string;
typedef qn_integer qn_json_integer;
typedef qn_number qn_json_number;
typedef qn_uint32 qn_json_hash;

#ifdef QN_JSON_BIG_OBJECT

typedef struct _QN_JSON_OBJECT
{
#define QN_JSON_OBJECT_MAX_BUCKETS 5
    qn_size size;
    qn_dqueue_ptr queues[QN_JSON_OBJECT_MAX_BUCKETS];
} qn_json_object, *qn_json_object_ptr;

#else

typedef struct _QN_JSON_OBJECT
{
    qn_dqueue_ptr queue;
} qn_json_object, *qn_json_object_ptr;

#endif // QN_JSON_BIG_OBJECT

typedef struct _QN_JSON_ARRAY
{
    qn_dqueue_ptr queue;
} qn_json_array, *qn_json_array_ptr;

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

typedef enum _QN_JSON_STATUS
{
    QN_JSON_PARSING_DONE = 0,
    QN_JSON_PARSING_KEY = 1,
    QN_JSON_PARSING_COLON = 2,
    QN_JSON_PARSING_VALUE = 3,
    QN_JSON_PARSING_COMMA = 4
} qn_json_status;

typedef struct _QN_JSON
{
    qn_json_hash hash;
    qn_string_ptr key;
    struct {
        qn_json_class class:3;
        qn_json_status status:3;
    };
    union {
        qn_json_object * object;
        qn_json_array * array;
        qn_json_string string;
        qn_json_integer integer;
        qn_json_number number;
        qn_json_boolean boolean;
    };
    union {
        qn_json_object obj_data[0];
        qn_json_array arr_data[0];
    };
} qn_json; // _QN_JSON

typedef struct _QN_JSON_ELEM_ITERATOR
{
    qn_dqueue_ptr * queue;
    qn_dqueue_ptr * end;
    qn_size pos;
} qn_json_elem_iterator, *qn_json_elem_iterator_ptr;

static
qn_json_ptr qn_json_elem_itr_next(qn_json_elem_iterator_ptr itr)
{
    while (itr->queue != itr->end && itr->pos >= qn_dqueue_size(*itr->queue)) {
        itr->queue += 1;
        itr->pos = 0;
    } // while
    if (itr->queue == itr->end) {
        return NULL;
    } // if
    return qn_dqueue_get(*itr->queue, itr->pos++);
} // qn_json_elem_itr_next

//-- Implementation of qn_json_object

static inline
void qn_json_obj_itr_init(qn_json_elem_iterator_ptr itr, qn_json_ptr parent)
{
#ifdef QN_JSON_BIG_OBJECT
    itr->queue = &parent->object->queues[0];
    itr->end = itr->queue + QN_JSON_OBJECT_MAX_BUCKETS;
#else
    itr->queue = &parent->object->queue;
    itr->end = itr->queue + 1;
#endif // QN_JSON_BIG_OBJECT

    itr->pos = 0;
} // qn_json_obj_itr_init

static
void qn_json_obj_destroy(qn_json_object_ptr obj_data)
{
    qn_json_ptr ce = NULL;
    qn_dqueue_ptr * queue = NULL;
    qn_dqueue_ptr * begin = NULL;
    qn_dqueue_ptr * end = NULL;

    assert(obj_data);

#ifdef QN_JSON_BIG_OBJECT
    if (obj_data->size == 0) {
        return;
    } // if

    begin = &obj_data->queues[0];
    end = begin + QN_JSON_OBJECT_MAX_BUCKETS;
#else
    if (qn_dqueue_is_empty(obj_data->queue)) {
        return;
    } // if

    begin = &obj_data->queue;
    end = begin + 1;
#endif // QN_JSON_BIG_OBJECT

    for (queue = begin; queue < end; queue += 1) {
        while (!qn_dqueue_is_empty(*queue)) {
            ce = qn_dqueue_pop(*queue);
            qn_json_destroy(ce);
        } // while
        qn_dqueue_destroy(*queue);
    } // for

#ifdef QN_JSON_BIG_OBJECT
    obj_data->size = 0;
#endif // QN_JSON_BIG_OBJECT
} // qn_json_obj_destroy

static
qn_json_hash qn_json_obj_calculate_hash(const char * cstr)
{
    qn_json_hash hash = 5381;
    int c;

    while ((c = *cstr++) != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } // while
    return hash;
} // qn_json_obj_calculate_hash

static
qn_json_ptr qn_json_obj_find(qn_dqueue_ptr queue, qn_json_hash hash, const char * key, qn_size * pos)
{
    qn_size i = 0;
    qn_json_ptr ce = NULL;

    for (i = 0; i < qn_dqueue_size(queue); i += 1) {
        ce = qn_dqueue_get(queue, i);
        if (ce->hash == hash && (qn_str_compare_raw(ce->key, key)) == 0) {
            *pos = i;
            return ce;
        } // if
    } // for
    return NULL;
} // qn_json_obj_find

//-- Implementation of qn_json_array

static inline
void qn_json_arr_itr_init(qn_json_elem_iterator_ptr itr, qn_json_ptr parent)
{
    itr->queue = &parent->array->queue;
    itr->end = itr->queue + 1;
    itr->pos = 0;
} // qn_json_arr_itr_init

static
void qn_json_arr_destroy(qn_json_array_ptr arr_data)
{
    qn_size i = 0;
    qn_json_ptr ce = NULL;

    assert(arr_data);

    for (i = 0; i < qn_dqueue_size(arr_data->queue); i += 1) {
        ce = qn_dqueue_pop(arr_data->queue);
        qn_json_destroy(ce);
    } // for
    qn_dqueue_destroy(arr_data->queue);
} // qn_json_arr_destroy

//-- Implementation of qn_json

qn_json_ptr qn_json_create_string(const char * cstr, qn_size cstr_size)
{
    qn_json_ptr new_str = NULL;

    new_str = calloc(1, sizeof(*new_str));
    if (new_str == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_str->class = QN_JSON_STRING;
    new_str->string = qn_str_create(cstr, cstr_size);
    if (!new_str->string) {
        return NULL;
    } // if
    return new_str;
} // qn_json_create_string

qn_json_ptr qn_json_create_integer(qn_integer val)
{
    qn_json_ptr new_int = NULL;

    new_int = calloc(1, sizeof(*new_int));
    if (new_int == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_int->class = QN_JSON_INTEGER;
    new_int->integer = val;
    return new_int;
} // qn_json_create_integer

qn_json_ptr qn_json_create_number(qn_number val)
{
    qn_json_ptr new_num = NULL;

    new_num = calloc(1, sizeof(*new_num));
    if (new_num == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_num->class = QN_JSON_NUMBER;
    new_num->number = val;
    return new_num;
} // qn_json_create_number

qn_json_ptr qn_json_create_boolean(qn_bool val)
{
    qn_json_ptr new_bool = NULL;

    new_bool = calloc(1, sizeof(*new_bool));
    if (new_bool == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_bool->class = QN_JSON_BOOLEAN;
    new_bool->boolean = val;
    return new_bool;
} // qn_json_create_boolean

qn_json_ptr qn_json_create_null(void)
{
    qn_json_ptr new_null = NULL;

    new_null = calloc(1, sizeof(*new_null));
    if (new_null == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_null->class = QN_JSON_NULL;
    return new_null;
} // qn_json_create_null

qn_json_ptr qn_json_create_object(void)
{
    qn_json_ptr new_obj = NULL;
    qn_json_object * obj_data = NULL;

#ifdef QN_JSON_BIG_OBJECT
    int i = 0;
#endif

    new_obj = calloc(1, sizeof(*new_obj) + sizeof(new_obj->obj_data[0]));
    if (new_obj == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_obj->class = QN_JSON_OBJECT;
    new_obj->object = obj_data = &new_obj->obj_data[0];

#ifdef QN_JSON_BIG_OBJECT
    for (i = 0; i < QN_JSON_OBJECT_MAX_BUCKETS; i += 1) {
        obj_data->queues[i] = qn_dqueue_create(4);
        if (!obj_data->queues[i]) {
            i -= 1;
            while (i >= 0) {
                qn_dqueue_destroy(obj_data->queues[i--]);
            } // while

            free(new_obj);
            return NULL;
        } // if
    } // for
#else
    obj_data->queue = qn_dqueue_create(4);
    if (!obj_data->queue) {
        free(new_obj);
        return NULL;
    } // if
#endif // QN_JSON_BIG_OBJECT

    return new_obj;
} // qn_json_create_object

qn_bool qn_json_set(qn_json_ptr self, const char * key, qn_json_ptr new_child)
{
    qn_json_ptr child = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_dqueue_ptr queue = NULL;
    qn_json_hash hash = 0;
    qn_size pos = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key && new_child);

    obj_data = &self->obj_data[0];
    hash = qn_json_obj_calculate_hash(key);

#ifdef QN_JSON_BIG_OBJECT
    queue = obj_data->queues[hash % QN_JSON_OBJECT_MAX_BUCKETS];
#else
    queue = obj_data->queue;
#endif // QN_JSON_BIG_OBJECT

    child = qn_json_obj_find(queue, hash, key, &pos);
    if (child) {
        // The element according to the given key has been existing.
        new_child->hash = child->hash;
        new_child->key = child->key;

        qn_dqueue_replace(obj_data->queue, pos, new_child);

        child->key = NULL;
        qn_json_destroy(child);
        return qn_true;
    } // if

    new_child->hash = hash;
    new_child->key = qn_str_clone_raw(key);
    if (!new_child->key) {
        return qn_false;
    } // if

    qn_dqueue_push(obj_data->queue, new_child);
#ifdef QN_JSON_BIG_OBJECT
    obj_data->size += 1;
#endif
    return qn_true;
} // qn_json_set

void qn_json_unset(qn_json_ptr self, const char * key)
{
    qn_json_ptr child = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_dqueue_ptr queue = NULL;
    qn_json_hash hash = 0;
    qn_size pos = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key);

    obj_data = &self->obj_data[0];
#ifdef QN_JSON_BIG_OBJECT
    if (obj_data->size == 0) {
        return;
    } // if
#else
    if (qn_dqueue_is_empty(obj_data->queue)) {
        return;
    } // if
#endif // QN_JSON_BIG_OBJECT

    hash = qn_json_obj_calculate_hash(key);

#ifdef QN_JSON_BIG_OBJECT
    queue = obj_data->queues[hash % QN_JSON_OBJECT_MAX_BUCKETS];
#else
    queue = obj_data->queue;
#endif // QN_JSON_BIG_OBJECT

    child = qn_json_obj_find(queue, hash, key, &pos);
    if (!child) {
        // The element according to the given key does not exist.
        return;
    } // if

    qn_dqueue_remove(queue, pos);
    qn_json_destroy(child);
#ifdef QN_JSON_BIG_OBJECT
    obj_data->size -= 1;
#endif
} // qn_json_unset

qn_json_ptr qn_json_create_array(void)
{
    qn_json_ptr new_arr = NULL;
    qn_json_array_ptr arr_data = NULL;

    new_arr = calloc(1, sizeof(*new_arr) + sizeof(new_arr->arr_data[0]));
    if (new_arr == NULL) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    new_arr->class = QN_JSON_ARRAY;
    new_arr->array = arr_data = &new_arr->arr_data[0];

    arr_data->queue = qn_dqueue_create(4);
    if (!arr_data->queue) {
        free(new_arr);
        return NULL;
    } // if
    return new_arr;
} // qn_json_create_array

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
        }

        if (self->key) {
            qn_str_destroy(self->key);
            self->key = NULL;
        } // if
        free(self);
    } // if
} // qn_json_destroy

qn_bool qn_json_push(qn_json_ptr self, qn_json_ptr new_child)
{
    assert(self && self->class == QN_JSON_ARRAY);
    assert(new_child);
    return qn_dqueue_push(self->array->queue, new_child);
} // qn_json_push

void qn_json_pop(qn_json_ptr self)
{
    qn_json_ptr child = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    if (qn_dqueue_is_empty(self->array->queue)) {
        return;
    } // if

    child = qn_dqueue_pop(self->array->queue);
    qn_json_destroy(child);
    return;
} // qn_json_pop

qn_bool qn_json_unshift(qn_json_ptr self, qn_json_ptr new_child)
{
    assert(self && self->class == QN_JSON_ARRAY);
    assert(new_child);
    return qn_dqueue_unshift(self->array->queue, new_child);
} // qn_json_unshift

void qn_json_shift(qn_json_ptr self)
{
    qn_json_ptr child = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    if (qn_dqueue_is_empty(self->array->queue)) {
        return;
    } // if

    child = qn_dqueue_shift(self->array->queue);
    qn_json_destroy(child);
    return;
} // qn_json_shift

qn_json_ptr qn_json_get(qn_json_ptr self, const char * key)
{
    qn_json_ptr child = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_dqueue_ptr queue = NULL;
    qn_json_hash hash = 0;
    qn_size pos = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key);

    obj_data = &self->obj_data[0];

#ifdef QN_JSON_BIG_OBJECT
    if (obj_data->size == 0) {
        // The object is empty.
        return NULL;
    } // if
#else
    if (qn_dqueue_is_empty(obj_data->queue)) {
        // The object is empty.
        return NULL;
    } // if
#endif // QN_JSON_BIG_OBJECT

    hash = qn_json_obj_calculate_hash(key);

#ifdef QN_JSON_BIG_OBJECT
    queue = obj_data->queues[hash % QN_JSON_OBJECT_MAX_BUCKETS];
#else
    queue = obj_data->queue;
#endif // QN_JSON_BIG_OBJECT

    child = qn_json_obj_find(queue, hash, key, &pos);
    if (!child) {
        // The element according to the given key does not exist.
        return NULL;
    } // if
    return child;
} // qn_json_get

qn_json_ptr qn_json_get_at(qn_json_ptr self, qn_size n)
{
    assert(self && self->class == QN_JSON_ARRAY);
    return qn_dqueue_get(self->array->queue, n);
} // qn_json_get_at

qn_string_ptr qn_json_to_string(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_STRING);
    return self->string;
} // qn_json_to_string

qn_integer qn_json_to_integer(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_INTEGER);
    return self->integer;
} // qn_json_to_integer

qn_number qn_json_to_number(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_NUMBER);
    return self->number;
} // qn_json_to_number

qn_bool qn_json_to_boolean(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_BOOLEAN);
    return self->boolean;
} // qn_json_to_boolean

qn_bool qn_json_is_object(qn_json_ptr self)
{
    return (self->class == QN_JSON_OBJECT);
} // qn_json_is_object

qn_bool qn_json_is_array(qn_json_ptr self)
{
    return (self->class == QN_JSON_ARRAY);
} // qn_json_is_array

qn_bool qn_json_is_string(qn_json_ptr self)
{
    return (self->class == QN_JSON_STRING);
} // qn_json_is_string

qn_bool qn_json_is_integer(qn_json_ptr self)
{
    return (self->class == QN_JSON_INTEGER);
} // qn_json_is_integer

qn_bool qn_json_is_number(qn_json_ptr self)
{
    return (self->class == QN_JSON_NUMBER);
} // qn_json_is_number

qn_bool qn_json_is_boolean(qn_json_ptr self)
{
    return (self->class == QN_JSON_BOOLEAN);
} // qn_json_is_boolean

qn_bool qn_json_is_null(qn_json_ptr self)
{
    return (self->class == QN_JSON_NULL);
} // qn_json_is_null

qn_string_ptr qn_json_key(qn_json_ptr self)
{
    return self->key;
} // qh_json_key

qn_bool qn_json_is_empty(qn_json_ptr self)
{
    if (self->class == QN_JSON_OBJECT) {
#ifdef QN_JSON_BIG_OBJECT
        return self->obj_data[0].size == 0;
#else
        return qn_dqueue_is_empty(self->obj_data[0].queue);
#endif
    }
    if (self->class == QN_JSON_ARRAY) {
        return qn_dqueue_is_empty(self->arr_data[0].queue);
    }
    return qn_false;
} // qn_json_is_empty

//-- Implementation of qn_json_iterator

typedef struct _QN_JSON_ITR_LEVEL
{
    qn_json_elem_iterator iterator;
    qn_json_ptr parent;
    int count;
} qn_json_itr_level, *qn_json_itr_level_ptr;

typedef struct _QN_JSON_ITERATOR
{
    int size;
    int capacity;
    qn_json_itr_level * levels;
    qn_json_itr_level lvl_data[3];
} qn_json_iterator;

qn_json_iterator_ptr qn_json_itr_create(void)
{
    qn_json_iterator_ptr new_itr = NULL;

    new_itr = calloc(1, sizeof(*new_itr));
    if (!new_itr) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_itr->capacity = sizeof(new_itr->lvl_data) / sizeof(new_itr->lvl_data[0]);
    new_itr->levels = &new_itr->lvl_data[0];
    return new_itr;
} // qn_json_itr_create

void qn_json_itr_destroy(qn_json_iterator_ptr self)
{
    if (self->levels != &self->lvl_data[0]) {
        free(self->levels);
    } // if
    free(self);
} // qn_json_itr_destroy

void qn_json_itr_reset(qn_json_iterator_ptr self)
{
    self->size = 0;
} // qn_json_itr_reset

void qn_json_itr_rewind(qn_json_iterator_ptr self)
{
    qn_json_itr_level_ptr curr_level = NULL;

    if (self->size <= 0) {
        return;
    } // if

    curr_level = &self->levels[self->size - 1];
    curr_level->count = 0;
    if (curr_level->parent->class == QN_JSON_OBJECT) {
        qn_json_obj_itr_init(&curr_level->iterator, curr_level->parent);
    } else {
        qn_json_arr_itr_init(&curr_level->iterator, curr_level->parent);
    } // if
} // qn_json_itr_rewind

int qn_json_itr_count(qn_json_iterator_ptr self)
{
    if (self->size <= 0) {
        return 0;
    } // if
    return self->levels[self->size - 1].count;
} // qn_json_itr_count

static
qn_bool qn_json_itr_augment_levels(qn_json_iterator_ptr self)
{
    int new_capacity = 0;
    qn_json_itr_level_ptr new_levels = NULL;

    new_capacity = self->capacity + (self->capacity >> 1); // 1.5 times of the last stack capacity.
    new_levels = calloc(1, sizeof(new_levels[0]) * new_capacity);
    if (!new_levels) {
        qn_err_set_no_enough_memory();
        return qn_false;
    }  // if

    memcpy(new_levels, self->levels, sizeof(self->levels[0]) * self->size);
    if (self->levels != &self->lvl_data[0]) {
        free(self->levels);
    } // if
    self->levels = new_levels;
    self->capacity = new_capacity;
    return qn_true;
} // qn_json_itr_augment_levels

qn_bool qn_json_itr_push(qn_json_iterator_ptr self, qn_json_ptr element)
{
    assert(self);
    qn_json_assert_complex_element(element);

    if ((self->size + 1) > self->capacity && !qn_json_itr_augment_levels(self)) {
        return qn_false;
    } // if

    self->levels[self->size++].parent = element;
    qn_json_itr_rewind(self);
    return qn_true;
} // qn_json_itr_push

void qn_json_itr_pop(qn_json_iterator_ptr self)
{
    if (self->size > 0) {
        self->size -= 1;
    } // if
} // qn_json_itr_pop

qn_json_ptr qn_json_itr_top(qn_json_iterator_ptr self)
{
    if (self->size <= 0) {
        return NULL;
    } // if
    return self->levels[self->size - 1].parent;
} // qn_json_itr_top

qn_json_ptr qn_json_itr_next(qn_json_iterator_ptr self)
{
    qn_json_ptr element = NULL;
    qn_json_itr_level_ptr curr_level = NULL;

    if (self->size <= 0) {
        // No levels in the stack.
        return NULL;
    } // if

    curr_level = &self->levels[self->size - 1];
    element = qn_json_elem_itr_next(&curr_level->iterator);
    if (element) {
        curr_level->count += 1;
    } // if
    return element;
} // qn_json_itr_next

//-- Implementation of qn_json_scanner

typedef enum _QN_JSON_TOKEN
{
    QN_JSON_TKN_UNKNOWN = 0,
    QN_JSON_TKN_STRING,
    QN_JSON_TKN_COLON,
    QN_JSON_TKN_COMMA,
    QN_JSON_TKN_INTEGER,
    QN_JSON_TKN_NUMBER,
    QN_JSON_TKN_TRUE,
    QN_JSON_TKN_FALSE,
    QN_JSON_TKN_NULL,
    QN_JSON_TKN_OPEN_BRACE,
    QN_JSON_TKN_CLOSE_BRACE,
    QN_JSON_TKN_OPEN_BRACKET,
    QN_JSON_TKN_CLOSE_BRACKET,
    QN_JSON_TKN_INPUT_END,
    QN_JSON_TKNERR_MALFORMED_TEXT,
    QN_JSON_TKNERR_NEED_MORE_TEXT,
    QN_JSON_TKNERR_NO_ENOUGH_MEMORY
} qn_json_token;

typedef struct _QN_JSON_SCANNER
{
    const char * buf;
    qn_size buf_size;
    qn_size pos;
} qn_json_scanner, *qn_json_scanner_ptr;

static inline
qn_uint32 qn_json_hex_to_dec(const char ch)
{
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    } // if
    if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 10;
    } // if
    if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 10;
    } // if
    return 0;
} // qn_json_hex_to_dec

static
qn_bool qn_json_unescape_to_utf8(char * cstr, qn_size * i, qn_size * m)
{
    qn_uint32 head_code = 0;
    qn_uint32 tail_code = 0;
    qn_uint32 wch = 0;

#define h1 qn_json_hex_to_dec(cstr[*i+2])
#define h2 qn_json_hex_to_dec(cstr[*i+3])
#define h3 qn_json_hex_to_dec(cstr[*i+4])
#define h4 qn_json_hex_to_dec(cstr[*i+5])

    // Chech if all four hexidecimal characters are valid.
    if (!ishexnumber(h1) || !ishexnumber(h2) || !ishexnumber(h3) || !ishexnumber(h4)) {
        qn_err_set_bad_text_input();
        return qn_false;
    } // if

    head_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (head_code <= 127) {
        *i += 6;
        cstr[*m++] = head_code & 0x7F;
        return qn_true;
    } // if

    if (head_code < 0xD800 || 0xDBFF < head_code) {
        qn_err_set_bad_text_input();
        return qn_false;
    } // if

    *i += 6;
    if (cstr[*i] != '\\' || cstr[*i+1] != 'u') {
        qn_err_set_bad_text_input();
        return qn_false;
    } // if

    tail_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (tail_code < 0xDC00 || 0xDFFF < tail_code) {
        qn_err_set_bad_text_input();
        return qn_false;
    } // if

    *i += 6;
    wch = ((head_code - 0xD800) << 10) + (tail_code - 0xDC00) + 0x10000;
    if (0x80 <= wch && wch <= 0x07FF) {
        // From : 00000000 00000yyy yyzzzzzz
        // To   : 110yyyyy（C0-DF) 10zzzzzz(80-BF）
        cstr[*m] = 0xC0 | ((wch >> 6) & 0x1F);
        cstr[*m+1] = 0x80 | (wch & 0x3F);
        *m += 2;
    } else if ((0x0800 <= wch && wch <= 0xD7FF) || (0xE000 <= wch && wch <= 0xFFFF)) {
        // From : 00000000 xxxxyyyy yyzzzzzz
        // To   : 1110xxxx(E0-EF) 10yyyyyy 10zzzzzz
        cstr[*m] = 0xE0 | ((wch >> 12) & 0x0F);
        cstr[*m+1] = 0x80 | ((wch >> 6) & 0x3F);
        cstr[*m+2] = 0x80 | (wch & 0x3F);
        *m += 3;
    } else if (0x10000 <= wch && wch <= 0x10FFFF) {
        // From : 000wwwxx xxxxyyyy yyzzzzzz
        // To   : 11110www(F0-F7) 10xxxxxx 10yyyyyy 10zzzzzz
        cstr[*m] = 0xF0 | ((wch >> 18) & 0x07);
        cstr[*m+1] = 0x80 | ((wch >> 12) & 0x3F);
        cstr[*m+2] = 0x80 | ((wch >> 6) & 0x3F);
        cstr[*m+3] = 0x80 | (wch & 0x3F);
        *m += 4;
    } // if
    return qn_true;

#undef h1
#undef h2
#undef h3
#undef h4
} // qn_json_unescape_to_utf8

static
qn_json_token qn_json_scan_string(qn_json_scanner_ptr s, qn_string_ptr * txt)
{
    qn_string_ptr new_str = NULL;
    char * cstr = NULL;
    qn_size primitive_len = 0;
    qn_size i = 0;
    qn_size m = 0;
    qn_size pos = s->pos + 1; // 跳过起始双引号

    for (; pos < s->buf_size; pos += 1) {
        if (s->buf[pos] == '\\') { m += 1; continue; }
        if (s->buf[pos] == '"' && s->buf[pos - 1] != '\\') { break; }
    } // for
    if (pos == s->buf_size) {
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    pos += 1; // 跳过终止双引号

    primitive_len = pos - s->pos - 2;
    cstr = calloc(1, primitive_len + 1);
    if (!cstr) {
        qn_err_set_no_enough_memory();
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    memcpy(cstr, s->buf + s->pos + 1, primitive_len);
    if (m > 0) {
        // 处理转义码
        i = 0;
        m = 0;
        while (i < primitive_len) {
            if (cstr[i] == '\\') {
                switch (cstr[i+1]) {
                    case 't': cstr[m++] = '\t'; i += 2; break;
                    case 'n': cstr[m++] = '\n'; i += 2; break;
                    case 'r': cstr[m++] = '\r'; i += 2; break;
                    case 'f': cstr[m++] = '\f'; i += 2; break;
                    case 'v': cstr[m++] = '\v'; i += 2; break;
                    case 'b': cstr[m++] = '\b'; i += 2; break;
                    case 'u':
                        if (!qn_json_unescape_to_utf8(cstr, &i, &m)) {
                            return qn_false;
                        } // if
                        break;

                    default:
                        // Process cases '"', '\\', '/', and other characters.
                        cstr[m++] = cstr[i+1];
                        i += 2;
                        break;
                } // switch
            } else {
                cstr[m++] = cstr[i++];
            } // if
        } // while
        cstr[m] = '\0';
    } else {
        m = primitive_len;
    } // if

    new_str = qn_str_create(cstr, m);
    free(cstr);

    if (!new_str) {
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    *txt = new_str;
    s->pos = pos;
    return QN_JSON_TKN_STRING;
} // qn_json_scan_string

static
qn_json_token qn_json_scan_number(qn_json_scanner_ptr s, qn_string_ptr * txt)
{
    qn_string_ptr new_str = NULL;
    qn_json_token tkn = QN_JSON_TKN_INTEGER;
    qn_size primitive_len = 0;
    qn_size pos = s->pos + 1; // 跳过已经被识别的首字符

    for (; pos < s->buf_size; pos += 1) {
        if (s->buf[pos] == '.') {
            if (tkn == QN_JSON_TKN_NUMBER) { break; }
            tkn = QN_JSON_TKN_NUMBER;
            continue;
        } // if
        if (s->buf[pos] < '0' || s->buf[pos] > '9') { break; }
    } // for
    if (pos == s->buf_size) {
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    primitive_len = pos - s->pos;
    new_str = qn_str_create(s->buf + s->pos, primitive_len);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    *txt = new_str;
    s->pos = pos;
    return tkn;
} // qn_json_scan_number

static
qn_json_token qn_json_scan(qn_json_scanner_ptr s, qn_string_ptr * txt)
{
    qn_size pos = s->pos;

    for (; pos < s->buf_size && isspace(s->buf[pos]); pos += 1) {
        // do nothing
    } // for
    if (pos == s->buf_size) {
        s->pos = pos;
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    // 此处 s->pos 应指向第一个非空白符的字符
    switch (s->buf[pos]) {
        case ':': s->pos = pos + 1; return QN_JSON_TKN_COLON;
        case ',': s->pos = pos + 1; return QN_JSON_TKN_COMMA;
        case '{': s->pos = pos + 1; return QN_JSON_TKN_OPEN_BRACE;
        case '}': s->pos = pos + 1; return QN_JSON_TKN_CLOSE_BRACE;
        case '[': s->pos = pos + 1; return QN_JSON_TKN_OPEN_BRACKET;
        case ']': s->pos = pos + 1; return QN_JSON_TKN_CLOSE_BRACKET;

        case '"':
            s->pos = pos;
            return qn_json_scan_string(s, txt);

        case '+': case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            s->pos = pos;
            return qn_json_scan_number(s, txt);

        case 't': case 'T':
            if (pos + 4 <= s->buf_size) {
                if (strncmp(s->buf + pos, "true", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_TRUE;
                } // if
                if (strncmp(s->buf + pos, "TRUE", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_TRUE;
                } // if
            } // if
            break;

        case 'f': case 'F':
            if (pos + 5 <= s->buf_size) {
                if (strncmp(s->buf + pos, "false", 5) == 0) {
                    s->pos = pos + 5;
                    return QN_JSON_TKN_FALSE;
                } // if
                if (strncmp(s->buf + pos, "FALSE", 5) == 0) {
                    s->pos = pos + 5;
                    return QN_JSON_TKN_FALSE;
                } // if
            } // if
            break;

        case 'n': case 'N':
            if (pos + 4 <= s->buf_size) {
                if (strncmp(s->buf + pos, "null", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_NULL;
                } // if
                if (strncmp(s->buf + pos, "NULL", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_NULL;
                } // if
            } // if
            break;
    } // switch
    return QN_JSON_TKNERR_MALFORMED_TEXT;
} // qn_json_scan

typedef struct _QN_JSON_PARSER
{
    qn_dqueue_ptr queue;
    qn_json_scanner scanner;
} qn_json_parser;

qn_json_parser_ptr qn_json_prs_create(void)
{
    qn_json_parser_ptr new_prs = NULL;

    new_prs = calloc(1, sizeof(*new_prs));
    if (!new_prs) {
        qn_err_set_no_enough_memory();
        return NULL;
    }
    new_prs->queue = qn_dqueue_create(4);
    if (!new_prs->queue) {
        free(new_prs);
        return NULL;
    } // if
    return new_prs;
} // qn_json_prs_create

void qn_json_prs_destroy(qn_json_parser_ptr prs)
{
    qn_json_ptr ce = NULL;

    if (prs) {
        while (!qn_dqueue_is_empty(prs->queue)) {
            ce = qn_dqueue_pop(prs->queue);
            qn_json_destroy(ce);
        } // while
        qn_dqueue_destroy(prs->queue);
        free(prs);
    } // if
} // qn_json_prs_destroy

void qn_json_prs_reset(qn_json_parser_ptr prs)
{
    if (prs) {
        qn_dqueue_reset(prs->queue);
    } // if
} // qn_json_prs_reset

static
qn_bool qn_json_put_in(
    qn_json_parser_ptr prs,
    qn_json_token tkn,
    qn_string_ptr txt,
    qn_json_ptr parent)
{
    qn_json_integer integer = 0L;
    qn_json_number number = 0.0L;
    qn_json_ptr new_child = NULL;
    char * end_txt = NULL;

    switch (tkn) {
        case QN_JSON_TKN_OPEN_BRACE:
            if (! (new_child = qn_json_create_object()) ) {
                return qn_false;
            } // if
            new_child->status = QN_JSON_PARSING_KEY;
            qn_dqueue_push(prs->queue, new_child);
            return qn_true;

        case QN_JSON_TKN_OPEN_BRACKET:
            if (! (new_child = qn_json_create_array()) ) {
                return qn_false;
            } // if
            new_child->status = QN_JSON_PARSING_VALUE;
            qn_dqueue_push(prs->queue, new_child);
            return qn_true;

        case QN_JSON_TKN_STRING:
            if (! (new_child = qn_json_create_string(qn_str_cstr(txt), qn_str_size(txt))) ) {
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_INTEGER:
            integer = strtoll(qn_str_cstr(txt), &end_txt, 10);
            if (end_txt == qn_str_cstr(txt)) {
                // 未解析
                qn_err_set_bad_text_input();
                return qn_false;
            } // if
            if ((integer == LLONG_MAX || integer == LLONG_MIN) && errno == ERANGE) {
                // 溢出
                qn_err_set_overflow_upper_bound();
                return qn_false;
            } // if
            if (! (new_child = qn_json_create_integer(integer)) ) {
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_NUMBER:
            number = strtold(qn_str_cstr(txt), &end_txt);
            if (end_txt == qn_str_cstr(txt)) {
                // 未解析
                qn_err_set_bad_text_input();
                return qn_false;
            } // if
            if (number >= HUGE_VALL && number <= HUGE_VALL && errno == ERANGE) {
                // 上溢
                qn_err_set_overflow_upper_bound();
                return qn_false;
            } // if
            if (number >= 0.0L && number <= 0.0L && errno == ERANGE) {
                // 下溢
                qn_err_set_overflow_lower_bound();
                return qn_false;
            } // if
            if (! (new_child = qn_json_create_number(number)) ) {
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_TRUE:
            if (! (new_child = qn_json_create_boolean(qn_true)) ) {
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_FALSE:
            if (! (new_child = qn_json_create_boolean(qn_false)) ) {
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_NULL:
            if (! (new_child = qn_json_create_null()) ) {
                return qn_false;
            } // if
            break;

        case QN_JSON_TKNERR_NEED_MORE_TEXT:
            qn_err_set_try_again();
            return qn_false;

        case QN_JSON_TKNERR_NO_ENOUGH_MEMORY:
            qn_err_set_no_enough_memory();
            return qn_false;

        default:
            qn_err_set_bad_text_input();
            return qn_false;
    } // switch

    if (parent->class == QN_JSON_OBJECT) {
        new_child->key = parent->key;
        parent->key = NULL;
        qn_json_set(parent, qn_str_cstr(new_child->key), new_child);
    } else {
        qn_json_push(parent, new_child);
    } // if
    return qn_true;
} // qn_json_put_in

static
qn_bool qn_json_parse_object(qn_json_parser_ptr prs)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    qn_string_ptr txt = NULL;
    qn_json_ptr parsing_obj = qn_dqueue_last(prs->queue);

PARSING_NEXT_ELEMENT_IN_THE_OBJECT:
    switch (parsing_obj->status) {
        case QN_JSON_PARSING_KEY:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                // The current object element has been parsed.
                parsing_obj->status = QN_JSON_PARSING_DONE;
                return qn_true;
            } // if

            if (tkn != QN_JSON_TKN_STRING) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            parsing_obj->key = txt;
            parsing_obj->status = QN_JSON_PARSING_COLON;

        case QN_JSON_PARSING_COLON:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn != QN_JSON_TKN_COLON) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            parsing_obj->status = QN_JSON_PARSING_VALUE;

        case QN_JSON_PARSING_VALUE:
            tkn = qn_json_scan(&prs->scanner, &txt);

            // Put the parsed element into the container.
            if (!qn_json_put_in(prs, tkn, txt, parsing_obj)) {
                qn_str_destroy(txt);
                txt = NULL;
                return qn_false;
            } // if

            qn_str_destroy(txt);
            txt = NULL;
            parsing_obj->status = QN_JSON_PARSING_COMMA;

            // Go to parse the new element on the top of the stack.
            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) {
                return qn_true;
            } // if

        case QN_JSON_PARSING_COMMA:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                // The current object element has been parsed.
                parsing_obj->status = QN_JSON_PARSING_DONE;
                return qn_true;
            } // if
            if (tkn != QN_JSON_TKN_COMMA) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            parsing_obj->status = QN_JSON_PARSING_KEY;
            break;

        default:
            assert(parsing_obj->status != QN_JSON_PARSING_DONE);
            return qn_false;
    } // switch

    goto PARSING_NEXT_ELEMENT_IN_THE_OBJECT;
    return qn_false;
} // qn_json_parse_object

static
qn_bool qn_json_parse_array(qn_json_parser_ptr prs)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    qn_string_ptr txt = NULL;
    qn_json_ptr parsing_arr = qn_dqueue_last(prs->queue);

PARSING_NEXT_ELEMENT_IN_THE_ARRAY:
    switch (parsing_arr->status) {
        case QN_JSON_PARSING_VALUE:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                // The current array element has been parsed.
                parsing_arr->status = QN_JSON_PARSING_DONE;
                return qn_true;
            } // if

            // Put the parsed element into the container.
            if (!qn_json_put_in(prs, tkn, txt, parsing_arr)) {
                qn_str_destroy(txt);
                txt = NULL;
                return qn_false;
            } // if

            qn_str_destroy(txt);
            txt = NULL;
            parsing_arr->status = QN_JSON_PARSING_COMMA;

            // Go to parse the new element on the top of the stack.
            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) {
                return qn_true;
            } // if

        case QN_JSON_PARSING_COMMA:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                // The current array element has been parsed.
                parsing_arr->status = QN_JSON_PARSING_DONE;
                return qn_true;
            } // if
            if (tkn != QN_JSON_TKN_COMMA) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            parsing_arr->status = QN_JSON_PARSING_VALUE;
            break;

        default:
            assert(
                parsing_arr->status != QN_JSON_PARSING_DONE
                && parsing_arr->status != QN_JSON_PARSING_VALUE
                && parsing_arr->status != QN_JSON_PARSING_COMMA
            );
            return qn_false;
    } // switch

    goto PARSING_NEXT_ELEMENT_IN_THE_ARRAY;
    return qn_false;
} // qn_json_parse_array

qn_bool qn_json_prs_parse(
    qn_json_parser_ptr prs,
    const char * restrict buf,
    qn_size * restrict buf_size,
    qn_json_ptr * root)
{
#define qn_json_prs_stack_is_empty(prs) (qn_dqueue_is_empty(prs->queue))

    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    qn_string_ptr txt = NULL;
    qn_json_ptr child = NULL;
    qn_json_ptr parent = NULL;

    prs->scanner.buf = buf;
    prs->scanner.buf_size = *buf_size;
    prs->scanner.pos = 0;

    if (qn_json_prs_stack_is_empty(prs)) {
        tkn = qn_json_scan(&prs->scanner, &txt);
        if (tkn == QN_JSON_TKN_OPEN_BRACE) {
            child = qn_json_create_object();
            child->status = QN_JSON_PARSING_KEY;
        } else if (tkn == QN_JSON_TKN_OPEN_BRACKET) {
            child = qn_json_create_array();
            child->status = QN_JSON_PARSING_VALUE;
        } else {
            // Not a valid piece of JSON text.
            qn_err_set_bad_text_input();
            return qn_false;
        } // if

        if (!child) {
            return qn_false;
        } // if

        // Push the first new element into the stack as the root.
        qn_dqueue_push(prs->queue, child);
    } // if

    while (1) {
        child = qn_dqueue_last(prs->queue);
        if (child->class == QN_JSON_OBJECT) {
            if (!qn_json_parse_object(prs)) {
                // Failed to parse the current object element.
                *buf_size = prs->scanner.pos;
                return qn_false;
            } // if
        } else {
            if (!qn_json_parse_array(prs)) {
                // Failed to parse the current array element.
                *buf_size = prs->scanner.pos;
                return qn_false;
            } // if
        } // if

        // Pop the parsed element out of the stack.
        qn_dqueue_pop(prs->queue);
        if (qn_json_prs_stack_is_empty(prs)) {
            // And it is the root element.
            parent = child;
            break;
        } // if

        // Put the parsed element into the container.
        parent = qn_dqueue_pop(prs->queue);
        if (parent->class == QN_JSON_OBJECT) {
            child->key = parent->key;
            parent->key = NULL;
            qn_json_set(parent, qn_str_cstr(parent->key), child);
        } else {
            qn_json_push(parent, child);
        } // if
    } // while

    *buf_size = prs->scanner.pos;
    *root = parent;
    return qn_true;

#undef qn_json_prs_stack_is_empty
} // qn_json_prs_parse

//-- Implementation of qn_json_formatter

typedef struct _QN_JSON_FORMATTER {
    char * buf;
    qn_size buf_size;
    qn_size buf_capacity;
    qn_size buf_pages;

    qn_json_iterator_ptr iterator;
} qn_json_formatter;

#define QN_JSON_FMT_PAGE_SIZE 4096

qn_json_formatter_ptr qn_json_fmt_create(void)
{
    qn_json_formatter_ptr new_fmt = NULL;

    new_fmt = calloc(1, sizeof(*new_fmt));
    if (!new_fmt) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_fmt->buf_pages = 2;
    new_fmt->buf_capacity = QN_JSON_FMT_PAGE_SIZE * new_fmt->buf_pages;
    new_fmt->buf = calloc(1, new_fmt->buf_capacity);
    if (!new_fmt->buf) {
        free(new_fmt);
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_fmt->iterator = qn_json_itr_create();
    if (!new_fmt->iterator) {
        free(new_fmt->buf);
        free(new_fmt);
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    return new_fmt;
} // qn_json_fmt_create

void qn_json_fmt_destroy(qn_json_formatter_ptr fmt)
{
    if (fmt) {
        qn_json_itr_destroy(fmt->iterator);
        free(fmt->buf);
        free(fmt);
    } // for
} // qn_json_fmt_destroy

static
qn_bool qn_json_fmt_augment_buffer(qn_json_formatter_ptr fmt)
{
    qn_size new_buf_pages = fmt->buf_pages + (fmt->buf_pages >> 1); // 1.5 time of the old pages.
    qn_size new_buf_capacity = QN_JSON_FMT_PAGE_SIZE * new_buf_pages;
    char * new_buf = malloc(new_buf_capacity);
    if (!new_buf) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_buf, fmt->buf, fmt->buf_size);
    free(fmt->buf);

    fmt->buf = new_buf;
    fmt->buf_capacity = new_buf_capacity;
    fmt->buf_pages = new_buf_pages;
    return qn_true;
} // qn_json_fmt_augment_buffer

static inline
qn_bool qn_json_fmt_putc(qn_json_formatter_ptr fmt, char ch)
{
    if (fmt->buf_size + 1 >= fmt->buf_capacity && !qn_json_fmt_augment_buffer(fmt)) {
        return qn_false;
    } // if

    fmt->buf[fmt->buf_size++] = ch;
    return qn_true;
} // qn_json_fmt_putc

static
qn_bool qn_json_fmt_format_string(qn_json_formatter_ptr fmt, qn_string_ptr str)
{
#define c1 (qn_str_cstr(str)[i])
#define c2 (qn_str_cstr(str)[i+1])
#define c3 (qn_str_cstr(str)[i+2])
#define c4 (qn_str_cstr(str)[i+3])
#define head_code (0xD800 + (((wch - 0x10000) & 0xFFC00) >> 10))
#define tail_code (0xDC00 + ((wch - 0x10000) & 0x003FF))

    int i = 0;
    int j = 0;
    int ret = 0;
    qn_uint32 wch = 0;

    if (!qn_json_fmt_putc(fmt, '"')) {
        return qn_false;
    } // if
    while (i < qn_str_size(str)) {
        if ((c1 & 0x80) == 0) {
            // ASCII range: 0zzzzzzz（00-7F）
            if (c1 == '&') {
                if ((fmt->buf_size + 6) >= fmt->buf_capacity && !qn_json_fmt_augment_buffer(fmt)) {
                    return qn_false;
                } // if

                ret = qn_str_snprintf(fmt->buf + fmt->buf_size, (fmt->buf_capacity - fmt->buf_size), "\\u%4X", c1);
                if (ret < 0) {
                    return qn_false;
                } // if
                i += 1;
                fmt->buf_size += ret;
                continue;
            } // if

            if (!qn_json_fmt_putc(fmt, c1)) {
                return qn_false;
            } // if
            i += 1;
            continue;
        } // if

        if ((c1 & 0xE0) == 0xC0) {
            // Check if the c2 is valid.
            if ((c2 & 0xC0) != 0x80) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            // From : 110yyyyy（C0-DF) 10zzzzzz(80-BF）
            // To   : 00000000 00000yyy yyzzzzzz
            wch = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
            j = 2;
        } else if ((c1 & 0xF0) == 0xE0) {
            // Check if the c2 and c3 are valid.
            if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80)) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            // From : 1110xxxx(E0-EF) 10yyyyyy 10zzzzzz
            // To   : 00000000 xxxxyyyy yyzzzzzz
            wch = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            j = 3;
        } else if ((c1 & 0xF8) == 0xF0) {
            // Check if the c2 and c3 and c4 are valid.
            if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80)) {
                qn_err_set_bad_text_input();
                return qn_false;
            } // if

            // From : 11110www(F0-F7) 10xxxxxx 10yyyyyy 10zzzzzz
            // To   : 000wwwxx xxxxyyyy yyzzzzzz
            wch = ((c1 & 0x1F) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
            j = 4;
        } // if

        if (0xD800 <= wch && wch <= 0xDFFF) {
            qn_err_set_bad_text_input();
            return qn_false;
        } // if

        if ((fmt->buf_size + 12) >= fmt->buf_capacity && !qn_json_fmt_augment_buffer(fmt)) {
            return qn_false;
        } // if

        ret = qn_str_snprintf(fmt->buf + fmt->buf_size, (fmt->buf_capacity - fmt->buf_size), "\\u%4X\\u%4X", head_code, tail_code);
        if (ret < 0) {
            return qn_false;
        } // if
        i += j;
        fmt->buf_size += ret;
    } // while

    if (!qn_json_fmt_putc(fmt, '"')) {
        return qn_false;
    } // if
    return qn_true;

#undef c1
#undef c2
#undef c3
#undef c4
#undef head_code
#undef tail_code
} // qn_json_fmt_format_string

static
qn_bool qn_json_fmt_format_ordinary(qn_json_formatter_ptr fmt, qn_json_ptr child)
{
    int ret = 0;
    const char * str = NULL;

    // Format string value.
    if (child->class == QN_JSON_STRING) {
        return qn_json_fmt_format_string(fmt, child->string);
    } // if

FORMAT_ORDINARY_ELEMENT:
    switch (child->class) {
        case QN_JSON_INTEGER:
            ret = qn_str_snprintf(fmt->buf + fmt->buf_size, (fmt->buf_capacity - fmt->buf_size), "%lld", child->integer);
            break;

        case QN_JSON_NUMBER:
            ret = qn_str_snprintf(fmt->buf + fmt->buf_size, (fmt->buf_capacity - fmt->buf_size), "%Lf", child->number);
            break;

        case QN_JSON_BOOLEAN:
            str = (child->boolean) ? "true" : "false";
            ret = qn_str_snprintf(fmt->buf + fmt->buf_size, (fmt->buf_capacity - fmt->buf_size), str);
            break;

        case QN_JSON_NULL:
            str = "null";
            ret = qn_str_snprintf(fmt->buf + fmt->buf_size, (fmt->buf_capacity - fmt->buf_size), str);
            break;

        default:
            qn_err_set_invalid_argument();
            return qn_false;
    } // switch

    if (ret < 0) {
        return qn_false;
    } else if ((ret + 1) < (fmt->buf_capacity - fmt->buf_size)) {
        // All the output, include the final nul character, has been written into the buffer.
        // And this does not consume all remainder buffer space.
        fmt->buf_size += ret;
        return qn_true;
    } // if

    if (!qn_json_fmt_augment_buffer(fmt)) {
        return qn_false;
    } // if

    goto FORMAT_ORDINARY_ELEMENT;
    // TODO: Specify a reasonable errno value.
    return qn_false;
} // qn_json_fmt_format_ordinary

qn_bool qn_json_fmt_format(
    qn_json_formatter_ptr fmt,
    qn_json_ptr root,
    const char ** restrict buf,
    qn_size * restrict buf_size)
{
    qn_json_ptr parent = NULL;
    qn_json_ptr child = NULL;

    assert(fmt);
    qn_json_assert_complex_element(root);

    // Reset all fields.
    fmt->buf_size = 0;
    qn_json_itr_reset(fmt->iterator);

    // Push the root element to format.
    qn_json_itr_push(fmt->iterator, root);
    if (root->class == QN_JSON_OBJECT) {
        fmt->buf[fmt->buf_size++] = '{';
    } else {
        fmt->buf[fmt->buf_size++] = '[';
    } // if

    while ((parent = qn_json_itr_top(fmt->iterator))) {
NEXT_FORMATTING_LEVEL:
        while ((child = qn_json_itr_next(fmt->iterator))) {
            // Output the comma between each element.
            if (qn_json_itr_count(fmt->iterator) > 1 && !qn_json_fmt_putc(fmt, ',')) {
                return qn_false;
            } // if

            if (child->key) {
                // Output the key.
                if (!qn_json_fmt_format_string(fmt, child->key)) {
                    return qn_false;
                } // if

                // Output the comma between the key and the value.
                if (!qn_json_fmt_putc(fmt, ':')) {
                    return qn_false;
                } // if
            } // if

            if (child->class == QN_JSON_OBJECT) {
                if (!qn_json_fmt_putc(fmt, '{') || !qn_json_itr_push(fmt->iterator, child)) {
                    return qn_false;
                } // if
                goto NEXT_FORMATTING_LEVEL;
            } // if

            if (child->class == QN_JSON_ARRAY) {
                if (!qn_json_fmt_putc(fmt, '[') || !qn_json_itr_push(fmt->iterator, child)) {
                    return qn_false;
                } // if
                goto NEXT_FORMATTING_LEVEL;
            } // if

            // Output the ordinary element itself.
            if (!qn_json_fmt_format_ordinary(fmt, child)) {
                return qn_false;
            } // if
        } // while

        if (parent->class == QN_JSON_OBJECT) {
            if (!qn_json_fmt_putc(fmt, '}')) {
                return qn_false;
            } // if
        } else {
            if (!qn_json_fmt_putc(fmt, ']')) {
                return qn_false;
            } // if
        } // if

        qn_json_itr_pop(fmt->iterator);
    } // while

    *buf = (const char *)&fmt->buf;
    *buf_size = fmt->buf_size;
    return qn_true;
} // qn_json_fmt_format

#ifdef __cplusplus
}
#endif
