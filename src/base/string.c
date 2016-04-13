#include <stdlib.h>
#include <string.h>

#include "base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

qn_string * qn_str_create(const char * str, qn_size size)
{
    qn_string src;

    src.str = str;
    src.size = size;

    return qn_str_duplicate(&src);
} // qn_str_create

qn_string * qn_str_duplicate(qn_string * src)
{
    qn_string * new_str = NULL;

    if (!src) {
        return NULL;
    }

    new_str = calloc(1, sizeof(*new_str) + src->size + 1);
    if (!new_str) {
        return NULL;
    }

    new_str->size = src->size;
    new_str->str = &new_str->data[0];

    if (new_str->size > 0) {
        memcpy((void*)new_str->str, src->str, new_str->size);
    }
    return new_str;
} // qn_str_duplicate

void qn_str_destroy(qn_string * self)
{
    if (self) {
        free(self);
    }
} // qn_str_destroy

qn_bool qn_str_compare(const qn_string * restrict s1, const qn_string * restrict s2)
{
    if (s1->size < s2->size) {
        // case   | 1      | 2       | 3
        // s1     | AABB   | BBBB    | BBCC
        // s2     | BBBBBB | BBBBBB  | BBBBBB
        // return | -1     | 0 => -1 | 1
        return (memcmp(s1->str, s2->str, s1->size) <= 0) ? -1 : 1;
    } // if

    if (s1->size > s2->size) {
        // case   | 1      | 2      | 3
        // s1     | AABBBB | BBBBBB | BBBBBB
        // s2     | BBBB   | BBBB   | AABB
        // return | -1     | 0 => 1 | 1
        return (memcmp(s1->str, s2->str, s2->size) >= 0) ? 1 : -1;
    } // if

    // case   | 1      | 2       | 3
    // s1     | AABB   | BBBB    | BBCC
    // s2     | BBBB   | BBBB    | BBBB
    // return | -1     | 0       | 1
    return memcmp(s1->str, s2->str, s1->size);
} // qn_Str_compare

#ifdef __cplusplus
}
#endif
