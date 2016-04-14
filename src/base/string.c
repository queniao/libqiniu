#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>
#include <errno.h>

#include "base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

static qn_string qn_str_empty_one = {"", 0};

qn_string * qn_str_create(const char * str, qn_size size)
{
    qn_string src;

    if (size == 0L) {
        return &qn_str_empty_one;
    }

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
    if (self && self != &qn_str_empty_one) {
        free(self);
    }
} // qn_str_destroy

qn_string * qn_str_join_strings(const char * delimiter, ...)
{
    va_list ap;
    qn_string * new_str = NULL;
    const char * first_str = NULL;
    const char * src_str = NULL;
    char * dst_pos = NULL;
    qn_size first_size = 0L;
    qn_size src_size = 0L;
    qn_size remainder_capacity = QN_STR_MAX_SIZE;
    qn_size delimiter_size = 0L;
    int i = 0;
    int arg_pair_count = 0;

    assert(delimiter != NULL);

    //== Count argument pairs
    delimiter_size = strlen(delimiter);

    va_start(ap, delimiter);
    // Check the first argument pair
    first_str = va_arg(ap, const char *);
    if (!first_str) {
        // No argument pairs are passed.
        va_end(ap);
        errno = EINVAL;
        return NULL;
    }

    first_size = va_arg(ap, qn_size);
    if (remainder_capacity < first_size) {
        va_end(ap);
        errno = EOVERFLOW;
        return NULL;
    }
    remainder_capacity -= first_size;
    arg_pair_count += 1;

    // Check other argument pairs
    src_str = va_arg(ap, const char *);
    while (src_str) {
        src_size = va_arg(ap, qn_size);
        if (remainder_capacity < delimiter_size + src_size) {
            va_end(ap);
            errno = EOVERFLOW;
            return NULL;
        }
        remainder_capacity -= (delimiter_size + src_size);
        arg_pair_count += 1;

        // Try to fetch next string.
        src_str = va_arg(ap, const char *);
    } // while
    va_end(ap);

    if (arg_pair_count == 1) {
        // Only one argument pair is passed.
        return qn_str_create(first_str, first_size);
    } // if

    //== Prepare a new string object
    new_str = calloc(1, sizeof(*new_str) + (QN_STR_MAX_SIZE - remainder_capacity) + 1);
    if (!new_str) {
        errno = ENOMEM;
        return NULL;
    }

    //== Copy all source strings and delimiters between them.
    va_start(ap, delimiter);
    dst_pos = &new_str->data[0];

    // Copy the first string.
    src_str = va_arg(ap, const char *);
    src_size = va_arg(ap, qn_size);
    if (src_size > 0L) {
        memcpy(dst_pos, src_str, src_size);
        dst_pos += src_size;
    }

    // Copy other strings and delimiters between them.
    for (i = 1; i < arg_pair_count; i += 1) {
        memcpy(dst_pos, delimiter, delimiter_size);
        dst_pos += delimiter_size;

        src_str = va_arg(ap, const char *);
        src_size = va_arg(ap, qn_size);
        if (src_size > 0L) {
            memcpy(dst_pos, src_str, src_size);
            dst_pos += src_size;
        }
    } // while
    va_end(ap);

    new_str->size = QN_STR_MAX_SIZE - remainder_capacity;
    new_str->str = &new_str->data[0];
    return new_str;
} // qn_str_join_strings

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
