#ifndef __QN_ESLINK_H__
#define __QN_ESLINK_H__ 1

#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_ESLINK
{
    struct _QN_ESLINK * next;
} qn_eslink, *qn_eslink_ptr;

static inline
void qn_eslink_init(qn_eslink_ptr node)
{
    node->next = node;
} // qn_eslink_init

static inline
int qn_eslink_is_linked(qn_eslink_ptr self)
{
    return (self->next != self);
} // qn_eslink_is_linked

static inline
void qn_eslink_insert_after(qn_eslink_ptr restrict self, qn_eslink_ptr restrict prev)
{
    assert(self != prev);
    self->next = prev->next;
    prev->next = self;
} // qn_eslink_insert_after

static inline
void qn_eslink_remove_from(qn_eslink_ptr restrict self, qn_eslink_ptr restrict prev)
{
    assert(self != prev);
    if (qn_eslink_is_linked(self)) {
        prev->next = self->next;
        qn_eslink_init(self);
    }
} // qn_eslink_remove_from

#define qn_eslink_offset(type, mem) ((char *)(&((type)0)->mem) - ((char *)0))
#define qn_eslink_super(ptr, type, mem) ((type)((char *)ptr - qn_eslink_offset(type, mem)))

#ifdef __cplusplus
}
#endif

#endif // __QN_ESLINK_H__
