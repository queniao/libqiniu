#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

#include "base/ds/eslink.h"

#include "base/string.h"
#include "base/json.h"

#define qn_json_assert_complex_element(elem) assert(elem && (elem->class == QN_JSON_OBJECT || elem->class == QN_JSON_ARRAY))

#ifdef __cplusplus
extern "C"
{
#endif

typedef qn_bool qn_json_boolean;
typedef qn_string qn_json_string;
typedef qn_integer qn_json_integer;
typedef qn_number qn_json_number;
typedef qn_uint32 qn_json_hash;

#define QN_JSON_OBJECT_MAX_BUCKETS 5

typedef struct _QN_JSON_OBJECT
{
    qn_size size;
    qn_eslink heads[QN_JSON_OBJECT_MAX_BUCKETS];
    qn_eslink * tails[QN_JSON_OBJECT_MAX_BUCKETS];
} qn_json_object, *qn_json_object_ptr;

typedef struct _QN_JSON_ARRAY
{
    qn_size size;
    qn_eslink head;
    qn_eslink * tail;
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
    qn_eslink node;
    qn_json_hash hash;
    qn_string_ptr key;
    struct {
        qn_json_class class:3;
        qn_json_status status:3;
    };
    union {
        qn_json_object * object;
        qn_json_array * array;
        qn_json_string * string;
        qn_json_integer integer;
        qn_json_number number;
        qn_json_boolean boolean;
    };
    union {
        qn_json_object obj_data[0];
        qn_json_array arr_data[0];
        qn_json_string str_data[0];
    };
} qn_json; // _QN_JSON

typedef struct _QN_JSON_ELEM_ITERATOR
{
    qn_eslink_ptr head;
    qn_eslink_ptr end;
    qn_eslink_ptr node;
} qn_json_elem_iterator, *qn_json_elem_iterator_ptr;

static
qn_json_ptr qn_json_elem_itr_next(qn_json_elem_iterator_ptr self)
{
    qn_json_ptr element = NULL;

    while (self->node == self->head) {
        if ((self->head + 1) >= self->end) {
            return NULL;
        } // if

        // Move to the next bucket.
        self->node = qn_eslink_next(++self->head);
    } // while

    element = qn_eslink_super(self->node, qn_json_ptr, node);
    self->node = qn_eslink_next(self->node);
    return element;
} // qn_json_elem_itr_next

//-- Implementation of qn_json_object

static inline
void qn_json_obj_itr_init(qn_json_elem_iterator_ptr self, qn_json_ptr parent)
{
    self->head = &parent->object->heads[0];
    self->end = &parent->object->heads[QN_JSON_OBJECT_MAX_BUCKETS];
    self->node = qn_eslink_next(self->head);
} // qn_json_obj_itr_init

static
void qn_json_obj_destroy(qn_json_object_ptr obj_data)
{
    int i = 0;
    qn_eslink * head = NULL;
    qn_eslink * cn = NULL;
    qn_json_ptr ce = NULL;

    assert(obj_data);

    if (obj_data->size == 0) {
        return;
    } // if

    for (i = 0; i < QN_JSON_OBJECT_MAX_BUCKETS; i += 1) {
        head = &obj_data->heads[i];
        cn = qn_eslink_next(head);
        while (cn != head) {
            qn_eslink_remove_after(cn, head);
            ce = qn_eslink_super(cn, qn_json_ptr, node);
            cn = qn_eslink_next(cn);
            qn_json_destroy(ce);
        } // while
    } // for
    obj_data->size = 0;
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
void qn_json_obj_find(
    qn_eslink * head,
    qn_json_hash hash,
    qn_string_ptr key,
    qn_eslink ** prev,
    qn_eslink ** curr)
{
    qn_eslink * pn = NULL;
    qn_eslink * cn = NULL;
    qn_json_ptr ce = NULL;

    assert(head != qn_eslink_next(head));
    
    for (pn = head, cn = qn_eslink_next(head); cn != head; pn = cn, cn = qn_eslink_next(cn)) {
        ce = qn_eslink_super(cn, qn_json_ptr, node);
        if (ce->hash != hash || (qn_str_compare(key, ce->key)) != 0) {
            continue;
        } // if
    } // for

    *prev = pn;
    *curr = cn;
} // qn_json_obj_find

//-- Implementation of qn_json_array

static inline
void qn_json_arr_itr_init(qn_json_elem_iterator_ptr self, qn_json_ptr parent)
{
    self->head = &parent->array->head;
    self->end = &parent->array->head + 1;
    self->node = qn_eslink_next(self->head);
} // qn_json_arr_itr_init

static
void qn_json_arr_destroy(qn_json_array_ptr arr_data)
{
    qn_eslink * head = NULL;
    qn_eslink * cn = NULL;
    qn_json_ptr ce = NULL;

    assert(arr_data);

    if (arr_data->size == 0) {
        return;
    } // if

    head = &arr_data->head;
    cn = qn_eslink_next(head);
    while (cn != head) {
        qn_eslink_remove_after(cn, head);
        ce = qn_eslink_super(cn, qn_json_ptr, node);
        cn = qn_eslink_next(cn);
        qn_json_destroy(ce);
    } // while
    arr_data->size = 0;
} // qn_json_arr_destroy

static
void qn_json_arr_find(
    qn_eslink * head,
    qn_size n,
    qn_eslink ** prev,
    qn_eslink ** curr
)
{
    qn_size i = 0;
    qn_eslink * pn = NULL;
    qn_eslink * cn = NULL;

    assert(head != qn_eslink_next(head));
    
    for (pn = head, cn = qn_eslink_next(head); cn != head; pn = cn, cn = qn_eslink_next(cn)) {
        if (i == n) {
            break;
        } // if
        i += 1;
    } // for
    *prev = pn;
    *curr = cn;
} // qn_json_arr_find

//-- Implementation of qn_json

qn_json_ptr qn_json_create_string(const char * cstr, qn_size cstr_size)
{
    qn_json_ptr new_str = NULL;
    qn_string_ptr str_data = NULL;

    new_str = calloc(1, sizeof(*new_str) + sizeof(new_str->str_data[0]) + cstr_size);
    if (new_str == NULL) {
        errno = ENOMEM;
        return NULL;
    } // if

    new_str->class = QN_JSON_STRING;
    new_str->string = str_data = &new_str->str_data[0];
    qn_str_copy(str_data, cstr, cstr_size);
    return new_str;
} // qn_json_create_string

qn_json_ptr qn_json_create_integer(qn_integer val)
{
    qn_json_ptr new_int = NULL;

    new_int = calloc(1, sizeof(*new_int));
    if (new_int == NULL) {
        errno = ENOMEM;
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
        errno = ENOMEM;
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
        errno = ENOMEM;
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
        errno = ENOMEM;
        return NULL;
    } // if

    new_null->class = QN_JSON_NULL;
    return new_null;
} // qn_json_create_null

qn_json_ptr qn_json_create_object(void)
{
    qn_json_ptr new_obj = NULL;
    qn_json_object * obj_data = NULL;
    int i = 0;

    new_obj = calloc(1, sizeof(*new_obj) + sizeof(new_obj->obj_data[0]));
    if (new_obj == NULL) {
        errno = ENOMEM;
        return NULL;
    } // if

    new_obj->class = QN_JSON_OBJECT;
    new_obj->object = obj_data = &new_obj->obj_data[0];

    obj_data->size = 0;
    for (i = 0; i < QN_JSON_OBJECT_MAX_BUCKETS; i += 1) {
        qn_eslink_init(&obj_data->heads[i]);
        obj_data->tails[i] = &obj_data->heads[i];
    } // for

    return new_obj;
} // qn_json_create_object

static
qn_bool qn_json_set_impl(qn_json_ptr self, qn_string_ptr key, qn_json_ptr new_child)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr child_node = NULL;
    qn_json_ptr child = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_json_hash hash = 0;
    int bucket = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key && new_child);

    obj_data = &self->obj_data[0];
    hash = qn_json_obj_calculate_hash(qn_str_cstr(key));
    bucket = hash % QN_JSON_OBJECT_MAX_BUCKETS;

    qn_json_obj_find(&obj_data->heads[bucket], hash, key, &prev_node, &child_node);
    if (child_node != &obj_data->heads[bucket]) {
        // The element according to the given key has been existing.
        child = qn_eslink_super(child_node, qn_json_ptr, node);

        new_child->hash = child->hash;
        new_child->key = child->key;

        qn_eslink_remove_after(child_node, prev_node);
        qn_eslink_insert_after(&new_child->node, prev_node);

        child->key = NULL;
        qn_json_destroy(child);
        return qn_true;
    } // if

    new_child->hash = hash;
    new_child->key = qn_str_duplicate(key);
    if (!new_child->key) {
        return qn_false;
    } // if

    qn_eslink_insert_after(&new_child->node, obj_data->tails[bucket]);
    obj_data->tails[bucket] = &new_child->node;
    obj_data->size += 1;
    return qn_true;
} // qn_json_set_impl

qn_bool qn_json_set(qn_json_ptr self, const char * key, qn_json_ptr new_child)
{
    qn_string key2 = {key, strlen(key)};
    return qn_json_set_impl(self, &key2, new_child);
} // qn_json_set

void qn_json_unset(qn_json_ptr self, const char * key)
{
    qn_string key2;
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr child_node = NULL;
    qn_json_ptr child = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_json_hash hash = 0;
    int bucket = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key);

    obj_data = &self->obj_data[0];
    if (obj_data->size == 0) {
        return;
    } // if

    key2.cstr = key;
    key2.size = strlen(key);

    hash = qn_json_obj_calculate_hash(key);
    bucket = hash % QN_JSON_OBJECT_MAX_BUCKETS;

    qn_json_obj_find(&obj_data->heads[bucket], hash, &key2, &prev_node, &child_node);
    if (child_node == &obj_data->heads[bucket]) {
        // The element according to the given key does not exist.
        return;
    } // if

    qn_eslink_remove_after(child_node, prev_node);
    if (obj_data->tails[bucket] == child_node) {
        obj_data->tails[bucket] = prev_node;
    } // if

    child = qn_eslink_super(child_node, qn_json_ptr, node);
    qn_json_destroy(child);
    obj_data->size -= 1;
} // qn_json_unset

qn_json_ptr qn_json_create_array(void)
{
    qn_json_ptr new_arr = NULL;
    qn_json_array_ptr arr_data = NULL;

    new_arr = calloc(1, sizeof(*new_arr) + sizeof(new_arr->arr_data[0]));
    if (new_arr == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_arr->class = QN_JSON_ARRAY;
    new_arr->array = arr_data = &new_arr->arr_data[0];

    arr_data->size = 0;
    qn_eslink_init(&arr_data->head);
    arr_data->tail = &arr_data->head;
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
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);
    assert(new_child);

    arr_data = &self->arr_data[0];
    qn_eslink_insert_after(&new_child->node, arr_data->tail);
    arr_data->tail = &new_child->node;
    arr_data->size += 1;
    return qn_true;
} // qn_json_push

void qn_json_pop(qn_json_ptr self)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr child_node = NULL;
    qn_json_ptr child = NULL;
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    arr_data = &self->arr_data[0];
    if (arr_data->size == 0) {
        return;
    } // if

    qn_json_arr_find(&arr_data->head, arr_data->size - 1, &prev_node, &child_node);

    child = qn_eslink_super(child_node, qn_json_ptr, node);
    qn_eslink_remove_after(child_node, prev_node);
    arr_data->size -= 1;

    qn_json_destroy(child);
    return;
} // qn_json_pop

qn_bool qn_json_unshift(qn_json_ptr self, qn_json_ptr new_child)
{
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);
    assert(new_child);

    arr_data = &self->arr_data[0];
    qn_eslink_insert_after(&new_child->node, &arr_data->head);
    arr_data->size += 1;
    return qn_true;
} // qn_json_unshift

void qn_json_shift(qn_json_ptr self)
{
    qn_json_ptr child = NULL;
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    arr_data = &self->arr_data[0];
    if (arr_data->size == 0) {
        return;
    } // if

    child = qn_eslink_super(qn_eslink_next(&arr_data->head), qn_json_ptr, node);
    qn_eslink_remove_after(qn_eslink_next(&arr_data->head), &arr_data->head);
    arr_data->size -= 1;

    qn_json_destroy(child);
    return;
} // qn_json_shift

qn_json_ptr qn_json_get(qn_json_ptr self, const char * key)
{
    qn_string key2;
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr child_node = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_json_hash hash = 0;
    int bucket = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key);

    key2.cstr = key;
    key2.size = strlen(key);

    obj_data = &self->obj_data[0];
    if (obj_data->size == 0) {
        // The object is empty.
        return NULL;
    } // if

    hash = qn_json_obj_calculate_hash(key);
    bucket = hash % QN_JSON_OBJECT_MAX_BUCKETS;

    qn_json_obj_find(&obj_data->heads[bucket], hash, &key2, &prev_node, &child_node);
    if (child_node == &obj_data->heads[bucket]) {
        // The element according to the given key does not exist.
        return NULL;
    } // if
    return qn_eslink_super(child_node, qn_json_ptr, node);
} // qn_json_get

qn_json_ptr qn_json_get_at(qn_json_ptr self, qn_size n)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr child_node = NULL;
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    arr_data = &self->arr_data[0];
    if (arr_data->size == 0 || arr_data->size < n) {
        // The array is empty or n exceeds the size of the array.
        return NULL;
    } // if

    qn_json_arr_find(&arr_data->head, n, &prev_node, &child_node);

    return qn_eslink_super(child_node, qn_json_ptr, node);
} // qn_json_get_at

qn_string * qn_json_to_string(qn_json_ptr self)
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

qn_bool qn_json_is_empty(qn_json_ptr self)
{
    if (self->class == QN_JSON_OBJECT) {
        return self->obj_data[0].size == 0;
    }
    if (self->class == QN_JSON_ARRAY) {
        return self->arr_data[0].size == 0;
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
        errno = ENOMEM;
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
        errno = ENOMEM;
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
    new_str = qn_str_allocate(primitive_len);
    if (!new_str) {
        errno = ENOMEM;
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if
    qn_str_copy(new_str, s->buf + s->pos + 1, primitive_len);

    cstr = &new_str->data[0];
    if (m > 0) {
        // 处理转义码
        for (i = 0, m = 0; i < primitive_len; i += 1) {
            // TODO: \u0026 形式的转义码转换
            if (cstr[i] == '\\') {
                i += 1;
                switch (cstr[i]) {
                    case '"': cstr[m++] = '"'; break;
                    case '\\': cstr[m++] = '\\'; break;
                    case '/': cstr[m++] = '/'; break;
                    case 't': cstr[m++] = '\t'; break;
                    case 'n': cstr[m++] = '\n'; break;
                    case 'r': cstr[m++] = '\r'; break;
                    case 'f': cstr[m++] = '\f'; break;
                    case 'v': cstr[m++] = '\v'; break;
                    case 'b': cstr[m++] = '\b'; break;
                    default:  cstr[m++] = cstr[i];
                } // switch
            } else {
                cstr[m++] = cstr[i];
            } // if
        } // for
        cstr[m] = '\0';
        new_str->size = m;
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
    new_str = qn_str_allocate(primitive_len);
    if (!new_str) {
        errno = ENOMEM;
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if
    qn_str_copy(new_str, s->buf + s->pos, primitive_len);

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
    qn_eslink top;
    qn_json_scanner scanner;
} qn_json_parser;

qn_json_parser_ptr qn_json_prs_create(void)
{
    qn_json_parser_ptr new_prs = NULL;
    
    new_prs = calloc(1, sizeof(*new_prs));
    if (!new_prs) {
        errno = ENOMEM;
        return NULL;
    }
    qn_eslink_init(&new_prs->top);
    return new_prs;
} // qn_json_prs_create

void qn_json_prs_destroy(qn_json_parser_ptr prs)
{
    qn_eslink_ptr child_node = NULL;
    qn_json_ptr curr_json = NULL;

    if (prs) {
        for (child_node = qn_eslink_next(&prs->top); child_node != &prs->top; child_node = qn_eslink_next(child_node)) {
            curr_json = qn_eslink_super(child_node, qn_json_ptr, node);
            qn_json_destroy(curr_json);
        } // while
        free(prs);
    } // if
} // qn_json_prs_destroy

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
            qn_eslink_insert_after(&new_child->node, &prs->top);
            return qn_true;

        case QN_JSON_TKN_OPEN_BRACKET:
            if (! (new_child = qn_json_create_array()) ) {
                return qn_false;
            } // if
            qn_eslink_insert_after(&new_child->node, &prs->top);
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
                errno = EBADMSG;
                return qn_false;
            } // if
            if ((integer == LLONG_MAX || integer == LLONG_MIN) && errno == ERANGE) {
                // 溢出
                errno = EOVERFLOW;
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
                errno = EBADMSG;
                return qn_false;
            } // if
            if (number >= HUGE_VALL && number <= HUGE_VALL && errno == ERANGE) {
                // 上溢
                errno = EOVERFLOW;
                return qn_false;
            } // if
            if (number >= 0.0L && number <= 0.0L && errno == ERANGE) {
                // 下溢
                errno = EOVERFLOW;
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
            errno = EAGAIN;
            return qn_false;

        case QN_JSON_TKNERR_NO_ENOUGH_MEMORY:
            errno = ENOMEM;
            return qn_false;

        default:
            errno = EBADMSG;
            return qn_false;
    } // switch

    if (parent->class == QN_JSON_OBJECT) {
        new_child->key = parent->key;
        parent->key = NULL;
        qn_json_set_impl(parent, new_child->key, new_child);
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
    qn_json_ptr parsing_obj = qn_eslink_super(qn_eslink_next(&prs->top), qn_json_ptr, node);

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
                errno = EBADMSG;
                return qn_false;
            } // if

            parsing_obj->key = txt;
            parsing_obj->status = QN_JSON_PARSING_COLON;

        case QN_JSON_PARSING_COLON:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn != QN_JSON_TKN_COLON) {
                errno = EBADMSG;
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
                errno = EBADMSG;
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
    qn_json_ptr parsing_arr = qn_eslink_super(qn_eslink_next(&prs->top), qn_json_ptr, node);

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
                errno = EBADMSG;
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
#define qn_json_prs_stack_is_empty(prs) (!qn_eslink_is_linked(&prs->top))

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
        } else if (tkn == QN_JSON_TKN_OPEN_BRACKET) {
            child = qn_json_create_array();
        } else {
            // Not a valid piece of JSON text.
            errno = EBADMSG;
            return qn_false;
        } // if

        if (!child) {
            return qn_false;
        } // if

        // Push the first new element into the stack as the root.
        qn_eslink_insert_after(&child->node, &prs->top);
    } // if

    while (1) {
        child = qn_eslink_super(qn_eslink_next(&prs->top), qn_json_ptr, node);
        if (child->class == QN_JSON_OBJECT && !qn_json_parse_object(prs)) {
            // Failed to parse the current object element.
            *buf_size = prs->scanner.pos;
            return qn_false;
        } else if (!qn_json_parse_array(prs)) {
            // Failed to parse the current array element.
            *buf_size = prs->scanner.pos;
            return qn_false;
        } // if

        // Pop the parsed element out of the stack.
        qn_eslink_remove_after(qn_eslink_next(&prs->top), &prs->top);
        if (qn_json_prs_stack_is_empty(prs)) {
            // And it is the root element.
            parent = child;
            break;
        } // if

        // Put the parsed element into the container.
        parent = qn_eslink_super(qn_eslink_next(&prs->top), qn_json_ptr, node);
        if (parent->class == QN_JSON_OBJECT) {
            child->key = parent->key;
            parent->key = NULL;
            qn_json_set_impl(parent, parent->key, child);
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
        errno = ENOMEM;
        return NULL;
    } // if

    new_fmt->buf_pages = 2;
    new_fmt->buf_capacity = QN_JSON_FMT_PAGE_SIZE * new_fmt->buf_pages;
    new_fmt->buf = calloc(1, new_fmt->buf_capacity);
    if (!new_fmt->buf) {
        free(new_fmt);
        errno = ENOMEM;
        return NULL;
    } // if

    new_fmt->iterator = qn_json_itr_create();
    if (!new_fmt->iterator) {
        free(new_fmt->buf);
        free(new_fmt);
        errno = ENOMEM;
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
        errno = ENOMEM;
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
            // TODO: Check if the d2 is valid.
            // From : 110yyyyy（C0-DF) 10zzzzzz(80-BF）
            // To   : 00000000 00000yyy yyzzzzzz
            wch = ((c1 & 0x1C) >> 2) << 8;
            wch |= ((c1 & 0x03) << 6) | (c2 & 0x3F);
            j = 2;
        } else if ((c1 & 0xF0) == 0xE0) {
            // TODO: Check if the d2 and d3 are valid.
            // From : 1110xxxx(E0-EF) 10yyyyyy 10zzzzzz
            // To   : 00000000 xxxxyyyy yyzzzzzz
            wch = ((((c1 & 0x0F) << 4) | ((c2 & 0x3C) >> 2))) << 8;
            wch |= ((c2 & 0x03) << 6) | (c3 & 0x3F);
            j = 3;
        } else if ((c1 & 0xF8) == 0xF0) {
            // TODO: Check if the d2 and d3 and d4 are valid.
            // From : 11110www(F0-F7) 10xxxxxx 10yyyyyy 10zzzzzz
            // To   : 000wwwxx xxxxyyyy yyzzzzzz
            wch = (((c1 & 0x07) << 2) | ((c2 & 0x30) >> 4)) << 16;
            wch |= (((c2 & 0x0F) << 4) | ((c3 & 0x3C) >> 2)) << 8;
            wch |= ((c3 & 0x03) << 6) | (c4 & 0x3F);
            j = 4;
        } // if

        if (0xD800 <= wch && wch <= 0xDFFF) {
            errno = EBADMSG;
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
            errno = EINVAL;
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

        if (parent->class == QN_JSON_OBJECT && !qn_json_fmt_putc(fmt, '}')) {
            return qn_false;
        } else if (!qn_json_fmt_putc(fmt, ']')) {
            return qn_false;
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
