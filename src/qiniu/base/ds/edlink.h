#ifndef __QN_EMBEDDED_DLINK_H__
#define __QN_EMBEDDED_DLINK_H__ 1

#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QUS_EMB_DLINK
{
    struct _QUS_EMB_DLINK * next;
    struct _QUS_EMB_DLINK * prev;
} qn_edlink, *qn_edlink_ptr;

static inline
void qn_edlink_init(qn_edlink_ptr node)
{
    node->next = node;
    node->prev = node;
} // qn_edlink_init

static inline
int qn_edlink_is_linked(qn_edlink_ptr node)
{
    return (node->next != node && node->prev != node);
} // qn_edlink_is_linked

static inline
void qn_edlink_insert_before(qn_edlink_ptr new_one, qn_edlink_ptr node)
{
    assert(new_one != node);
    new_one->next = node;
    new_one->prev = node->prev;
    node->prev->next = new_one;
    node->prev = new_one;
} // qn_edlink_insert_before

static inline
void qn_edlink_insert_after(qn_edlink_ptr new_one, qn_edlink_ptr node)
{
    assert(new_one != node);
    new_one->next = node->next;
    new_one->prev = node;
    new_one->next->prev = new_one;
    node->next = new_one;
} // qn_edlink_insert

static inline
void qn_edlink_remove(qn_edlink_ptr old_one)
{
    if (qn_edlink_is_linked(old_one)) {
        old_one->prev->next = old_one->next;
        old_one->next->prev = old_one->prev;
        qn_edlink_init(old_one);
    }
} // qn_edlink_remove

#define qn_edlink_offset(type, member) ((char *)(&((type)0)->member) - ((char *)0))
#define qn_edlink_object(ptr, type, member) ((type)((char *)ptr - qn_edlink_offset(type, member)))

#ifdef __cplusplus
}
#endif

#endif // __QN_EMBEDDED_DLINK_H__
