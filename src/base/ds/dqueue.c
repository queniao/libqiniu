#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "base/ds/dqueue.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_JSON_DQUEUE
{
    qn_size capacity;
    qn_size begin;
    qn_size end;
    qn_dqueue_element_ptr * elements;
    qn_dqueue_element_ptr elements_data[4];
} qn_dqueue;

static
qn_bool qn_dqueue_augment_head(qn_dqueue_ptr queue)
{
    qn_size delta = 0;
    qn_size new_capacity = queue->capacity * ((queue->capacity < 16) ? 2 : 1.5);
    qn_dqueue_element_ptr * new_data = NULL;

    new_data = calloc(1, sizeof(queue->elements[0]) * new_capacity);
    if (!new_data) {
        errno = ENOMEM;
        return qn_false;
    } // if

    delta = new_capacity - queue->capacity;
    memcpy(&new_data[delta], &queue->elements[0], sizeof(queue->elements[0]) * queue->capacity);
    if (queue->elements != queue->elements_data) {
        free(queue->elements);
    } // if

    queue->elements = new_data;
    queue->begin += delta;
    queue->end += delta;
    queue->capacity = new_capacity;
    return qn_true;
} // qn_dqueue_augment_head

static
qn_bool qn_dqueue_augment_tail(qn_dqueue_ptr queue)
{
    qn_size new_capacity = queue->capacity * ((queue->capacity < 16) ? 2 : 1.5);
    qn_dqueue_element_ptr * new_data = NULL;

    new_data = calloc(1, sizeof(queue->elements[0]) * new_capacity);
    if (!new_data) {
        errno = ENOMEM;
        return qn_false;
    } // if

    memcpy(&new_data[0], &queue->elements[0], sizeof(queue->elements[0]) * queue->capacity);
    if (queue->elements != queue->elements_data) {
        free(queue->elements);
    } // if

    queue->elements = new_data;
    queue->capacity = new_capacity;
    return qn_true;
} // qn_dqueue_augment_tail

qn_dqueue_ptr qn_dqueue_create(qn_size init_capacity)
{
    qn_dqueue_ptr new_queue = NULL;

    new_queue = calloc(1, sizeof(*new_queue));
    if (!new_queue) {
        errno = ENOMEM;
        return NULL;
    } // if

    if (init_capacity > 4) {
        new_queue->elements = calloc(1, sizeof(new_queue->elements[0]) * init_capacity);
        if (!new_queue->elements) {
            free(new_queue);
            errno = ENOMEM;
            return NULL;
        } // if

        new_queue->capacity = init_capacity;
    } else {
        new_queue->capacity = sizeof(new_queue->elements_data) / sizeof(new_queue->elements_data[0]);
        new_queue->elements = &new_queue->elements_data[0];
    } // if

    qn_dqueue_reset(new_queue);
    return new_queue;
} // qn_dqueue_create

void qn_dqueue_destroy(qn_dqueue_ptr queue)
{
    if (queue) {
        if (queue->elements != queue->elements_data) {
            free(queue->elements);
        } // if
        free(queue);
    } // if
} // qn_dqueue_destroy

void qn_dqueue_reset(qn_dqueue_ptr queue)
{
    if (queue) {
        if (queue->begin == 0) {
            queue->begin = queue->end = 0;
        } else if (queue->end == queue->capacity) {
            queue->begin = queue->end = queue->capacity;
        } else {
            queue->begin = queue->end = queue->capacity / 2;
        } // if
    } // if
} // qn_dqueue_reset

qn_bool qn_dqueue_push(qn_dqueue_ptr queue, qn_dqueue_element_ptr element)
{
    if (queue->end == queue->capacity) {
        if (!qn_dqueue_augment_tail(queue)) {
            return qn_false;
        } // if
    } // if
    queue->elements[queue->end++] = element;
    return qn_true;
} // qn_dqueue_push

qn_dqueue_element_ptr qn_dqueue_pop(qn_dqueue_ptr queue)
{
    if (qn_dqueue_is_empty(queue)) {
        return NULL;
    } // if
    return queue->elements[--queue->end];
} // qn_dqueue_pop

qn_bool qn_dqueue_unshift(qn_dqueue_ptr queue, qn_dqueue_element_ptr element)
{
    if (queue->end == 0) {
        if (!qn_dqueue_augment_head(queue)) {
            return qn_false;
        } // if
    } // if
    queue->elements[--queue->begin] = element;
    return qn_true;
} // qn_dqueue_unshift

qn_dqueue_element_ptr qn_dqueue_shift(qn_dqueue_ptr queue)
{
    if (qn_dqueue_is_empty(queue)) {
        return NULL;
    } // if
    return queue->elements[queue->begin++];
} // qn_dqueue_shift

qn_dqueue_element_ptr qn_dqueue_get(qn_dqueue_ptr queue, qn_size n)
{
    if (n > qn_dqueue_size(queue)) {
        return NULL;
    } // if
    return queue->elements[queue->begin + n];
} // qn_dqueue_get

qn_dqueue_element_ptr qn_dqueue_last(qn_dqueue_ptr queue)
{
    if (qn_dqueue_is_empty(queue)) {
        return NULL;
    } // if
    return queue->elements[queue->end - 1];
} // qn_dqueue_last

void qn_dqueue_replace(qn_dqueue_ptr queue, qn_size n, qn_dqueue_element_ptr element)
{
    queue->elements[queue->begin + n] = element;
} // qn_dqueue_replace

void qn_dqueue_remove(qn_dqueue_ptr queue, qn_size n)
{
    if (n == 0) {
        qn_dqueue_shift(queue);
    } else if (n == qn_dqueue_size(queue) - 1) {
        qn_dqueue_pop(queue);
    } // if
    memmove(
        &queue->elements[queue->begin + n],
        &queue->elements[queue->begin + n + 1],
        sizeof(queue->elements[0]) * (qn_dqueue_size(queue) - n - 1)
    );
    queue->end -= 1;
} // qn_dqueue_remove

qn_size qn_dqueue_size(qn_dqueue_ptr queue)
{
    return (queue->end - queue->begin);
} // qn_dqueue_size

#ifdef __cplusplus
}
#endif
