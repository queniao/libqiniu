#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "base/ds/eslink.h"

#include "base/string.h"
#include "base/json.h"

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
    QN_JSON_NULL,
    QN_JSON_BOOLEAN,
    QN_JSON_INTEGER,
    QN_JSON_NUMBER,
    QN_JSON_STRING,
    QN_JSON_ARRAY,
    QN_JSON_OBJECT,
} qn_json_class;

typedef struct _QN_JSON
{
    qn_eslink node;
    qn_json_hash hash;
    const char * key;
    qn_json_class class;
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

//-- Implementation of qn_json_object

qn_json_ptr qn_json_new_object(void)
{
    qn_json_ptr new_obj = NULL;
    qn_json_object * obj_data = NULL;
    int i = 0;

    new_obj = calloc(1, sizeof(*new_obj) + sizeof(new_obj->obj_data[0]));
    if (new_obj == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_obj->class = QN_JSON_OBJECT;
    new_obj->object = obj_data = &new_obj->obj_data[0];

    obj_data->size = 0;
    for (i = 0; i < QN_JSON_OBJECT_MAX_BUCKETS; i += 1) {
        qn_eslink_init(&obj_data->heads[i]);
        obj_data->tails[i] = &obj_data->heads[i];
    } // for

    return new_obj;
} // qn_json_new_object

static
void qn_json_object_delete(qn_json_object_ptr obj_data)
{
    int i = 0;
    qn_eslink * head = NULL;
    qn_eslink * pn = NULL;
    qn_eslink * cn = NULL;
    qn_json_ptr cv = NULL;

    assert(obj_data);

    if (obj_data->size == 0) {
        return;
    } // if

    for (i = 0; i < QN_JSON_OBJECT_MAX_BUCKETS; i += 1) {
        head = &obj_data->heads[i];
        for (pn = head, cn = qn_eslink_next(head); cn != head; pn = cn, cn = qn_eslink_next(cn)) {
            qn_eslink_remove_after(cn, pn);
            cv = qn_eslink_super(cn, qn_json_ptr, node);
            qn_json_delete(cv);
        } // for
    } // for
    obj_data->size = 0;
} // qn_json_object_delete

static
qn_json_hash qn_json_object_calculate_hash(const char * str)
{
    qn_json_hash hash = 5381;
    int c;

    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } // while
    return hash;
} // qn_json_object_calculate_hash

static
void qn_json_object_find(
    qn_eslink * head,
    qn_json_hash hash,
    const char * key,
    qn_eslink ** prev,
    qn_eslink ** curr
)
{
    qn_eslink * pn = NULL;
    qn_eslink * cn = NULL;
    qn_json_ptr cv = NULL;

    assert(head != qn_eslink_next(head));
    
    for (pn = head, cn = qn_eslink_next(head); cn != head; pn = cn, cn = qn_eslink_next(cn)) {
        cv = qn_eslink_super(cn, qn_json_ptr, node);
        if (cv->hash != hash || (strcmp(cv->key, key) != 0)) {
            continue;
        }
    } // for

    *prev = pn;
    *curr = cn;
} // qn_json_object_find

qn_bool qn_json_set(qn_json_ptr self, const char * key, qn_json_ptr new_value)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr curr_node = NULL;
    qn_json_ptr curr_value = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_json_hash hash = 0;
    int bucket = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key && new_value);

    obj_data = &self->obj_data[0];
    hash = qn_json_object_calculate_hash(key);
    bucket = hash % QN_JSON_OBJECT_MAX_BUCKETS;

    qn_json_object_find(&obj_data->heads[bucket], hash, key, &prev_node, &curr_node);
    if (curr_node != &obj_data->heads[bucket]) {
        // The value according to the given key has been existing.
        curr_value = qn_eslink_super(curr_node, qn_json_ptr, node);

        new_value->hash = curr_value->hash;
        new_value->key = curr_value->key;

        qn_eslink_remove_after(curr_node, prev_node);
        qn_eslink_insert_after(&new_value->node, prev_node);

        curr_value->key = NULL;
        qn_json_delete(curr_value);
        return qn_true;
    } // if

    new_value->hash = hash;
    new_value->key = strdup(key);
    if (!new_value->key) {
        errno = ENOMEM;
        return qn_false;
    } // if

    qn_eslink_insert_after(&new_value->node, obj_data->tails[bucket]);
    obj_data->tails[bucket] = &new_value->node;
    obj_data->size += 1;
    return qn_true;
} // qn_json_set

void qn_json_unset(qn_json_ptr self, const char * key)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr curr_node = NULL;
    qn_json_ptr curr_value = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_json_hash hash = 0;
    int bucket = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key);

    obj_data = &self->obj_data[0];
    hash = qn_json_object_calculate_hash(key);
    bucket = hash % QN_JSON_OBJECT_MAX_BUCKETS;

    qn_json_object_find(&obj_data->heads[bucket], hash, key, &prev_node, &curr_node);
    if (curr_node == &obj_data->heads[bucket]) {
        // The value according to the given key does not exist.
        return;
    } // if

    qn_eslink_remove_after(curr_node, prev_node);
    if (obj_data->tails[bucket] == curr_node) {
        obj_data->tails[bucket] = prev_node;
    } // if

    curr_value = qn_eslink_super(curr_node, qn_json_ptr, node);
    qn_json_delete(curr_value);
    obj_data->size -= 1;
} // qn_json_unset

//-- Implementation of qn_json_array

qn_json_ptr qn_json_new_array(void)
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
} // qn_json_new_array

static
void qn_json_array_delete(qn_json_array_ptr arr_data)
{
    qn_eslink * head = NULL;
    qn_eslink * pn = NULL;
    qn_eslink * cn = NULL;
    qn_json_ptr cv = NULL;

    assert(arr_data);

    if (arr_data->size == 0) {
        return;
    } // if

    head = &arr_data->head;
    for (pn = head, cn = qn_eslink_next(head); cn != head; pn = cn, cn = qn_eslink_next(cn)) {
        qn_eslink_remove_after(cn, pn);
        cv = qn_eslink_super(cn, qn_json_ptr, node);
        qn_json_delete(cv);
    } // for
    arr_data->size = 0;
} // qn_json_array_delete

static
void qn_json_array_find(
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
        }
        i += 1;
    } // for
    *prev = pn;
    *curr = cn;
} // qn_json_array_find

qn_bool qn_json_push(qn_json_ptr self, qn_json_ptr new_value)
{
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);
    assert(new_value);

    arr_data = &self->arr_data[0];
    qn_eslink_insert_after(&new_value->node, arr_data->tail);
    arr_data->tail = &new_value->node;
    arr_data->size += 1;
    return qn_true;
} // qn_json_push

void qn_json_pop(qn_json_ptr self)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr curr_node = NULL;
    qn_json_ptr curr_value = NULL;
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    arr_data = &self->arr_data[0];
    if (arr_data->size == 0) {
        return;
    } // if

    qn_json_array_find(&arr_data->head, arr_data->size - 1, &prev_node, &curr_node);

    curr_value = qn_eslink_super(curr_node, qn_json_ptr, node);
    qn_eslink_remove_after(curr_node, prev_node);
    arr_data->size -= 1;

    qn_json_delete(curr_value);
    return;
} // qn_json_pop

qn_bool qn_json_unshift(qn_json_ptr self, qn_json_ptr new_value)
{
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);
    assert(new_value);

    arr_data = &self->arr_data[0];
    qn_eslink_insert_after(&new_value->node, &arr_data->head);
    arr_data->size += 1;
    return qn_true;
} // qn_json_unshift

void qn_json_shift(qn_json_ptr self)
{
    qn_json_ptr curr_value = NULL;
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    arr_data = &self->arr_data[0];
    if (arr_data->size == 0) {
        return;
    } // if

    curr_value = qn_eslink_super(qn_eslink_next(&arr_data->head), qn_json_ptr, node);
    qn_eslink_remove_after(qn_eslink_next(&arr_data->head), &arr_data->head);
    arr_data->size -= 1;

    qn_json_delete(curr_value);
    return;
} // qn_json_shift


//-- Implementation of qn_json

void qn_json_delete(qn_json_ptr self) {
    if (self) {
        switch (self->class) {
            case QN_JSON_OBJECT:
                qn_json_object_delete(self->object);
                break;

            case QN_JSON_ARRAY:
                qn_json_array_delete(self->array);
                break;

            default:
                break;
        }
        free((void*)self->key);
        free(self);
    } // if
} // qn_json_delete

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

#ifdef __cplusplus
}
#endif
