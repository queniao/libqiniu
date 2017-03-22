#ifndef __QN_DS_DQUEUE_H__
#define __QN_DS_DQUEUE_H__ 1

#include <assert.h>

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void * qn_dqueue_element_ptr;

struct _QN_JSON_DQUEUE;
typedef struct _QN_JSON_DQUEUE * qn_dqueue_ptr;

QN_SDK extern qn_dqueue_ptr qn_dqueue_create(int init_capacity);
QN_SDK extern void qn_dqueue_destroy(qn_dqueue_ptr restrict queue);
QN_SDK extern void qn_dqueue_reset(qn_dqueue_ptr restrict queue);

QN_SDK extern int qn_dqueue_size(qn_dqueue_ptr restrict queue);
#define qn_dqueue_is_empty(q) (qn_dqueue_size(q) == 0)

QN_SDK extern qn_bool qn_dqueue_push(qn_dqueue_ptr restrict queue, qn_dqueue_element_ptr restrict element);
QN_SDK extern qn_dqueue_element_ptr qn_dqueue_pop(qn_dqueue_ptr restrict queue);
QN_SDK extern qn_bool qn_dqueue_unshift(qn_dqueue_ptr restrict queue, qn_dqueue_element_ptr restrict element);
QN_SDK extern qn_dqueue_element_ptr qn_dqueue_shift(qn_dqueue_ptr restrict queue);
QN_SDK extern qn_dqueue_element_ptr qn_dqueue_get(qn_dqueue_ptr restrict queue, int n);
QN_SDK extern qn_dqueue_element_ptr qn_dqueue_last(qn_dqueue_ptr restrict queue);
QN_SDK extern void qn_dqueue_replace(qn_dqueue_ptr restrict queue, int n, qn_dqueue_element_ptr restrict element);
QN_SDK extern void qn_dqueue_remove(qn_dqueue_ptr restrict queue, int n);

#ifdef __cplusplus
}
#endif

#endif // __QN_DS_DQUEUE_H__
