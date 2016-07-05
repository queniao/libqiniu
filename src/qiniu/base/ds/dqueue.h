#ifndef __QN_DQUEUE_H__
#define __QN_DQUEUE_H__ 1

#include <assert.h>

#include "qiniu/base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void * qn_dqueue_element_ptr;

struct _QN_JSON_DQUEUE;
typedef struct _QN_JSON_DQUEUE * qn_dqueue_ptr;

extern qn_dqueue_ptr qn_dqueue_create(qn_size init_capacity);
extern void qn_dqueue_destroy(qn_dqueue_ptr queue);
extern void qn_dqueue_reset(qn_dqueue_ptr queue);

extern qn_size qn_dqueue_size(qn_dqueue_ptr queue);
#define qn_dqueue_is_empty(q) (qn_dqueue_size(q) == 0)

extern qn_bool qn_dqueue_push(qn_dqueue_ptr queue, qn_dqueue_element_ptr element);
extern qn_dqueue_element_ptr qn_dqueue_pop(qn_dqueue_ptr queue);
extern qn_bool qn_dqueue_unshift(qn_dqueue_ptr queue, qn_dqueue_element_ptr element);
extern qn_dqueue_element_ptr qn_dqueue_shift(qn_dqueue_ptr queue);
extern qn_dqueue_element_ptr qn_dqueue_get(qn_dqueue_ptr queue, qn_size n);
extern qn_dqueue_element_ptr qn_dqueue_last(qn_dqueue_ptr queue);
extern void qn_dqueue_replace(qn_dqueue_ptr queue, qn_size n, qn_dqueue_element_ptr element);
extern void qn_dqueue_remove(qn_dqueue_ptr queue, qn_size n);

#ifdef __cplusplus
}
#endif

#endif // __QN_DQUEUE_H__