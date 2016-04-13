#ifndef __QN_STRING_H__
#define __QN_STRING_H__

#include "base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_STRING
{
    qn_size size;
    const char * str;
    const char data[0];
} qn_string;

extern qn_string * qn_str_create(const char * str, qn_size size);
extern qn_string * qn_str_duplicate(qn_string * src);
extern void qn_str_destroy(qn_string * self);

extern qn_bool qn_str_compare(const qn_string * restrict s1, const qn_string * restrict s2);

static inline qn_size qn_str_size(const qn_string * self)
{
    return self->size;
} // qn_str_size

static inline const char * qn_str_string(const qn_string * self)
{
    return self->str;
} // qn_str_string

#ifdef __cplusplus
}
#endif

#endif // __QN_STRING_H__
