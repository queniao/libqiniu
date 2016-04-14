#include "base/string.h"
#include "base/json.h"

#ifdef __cplusplus
extern "C"
{
#endif

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

typedef qn_bool qn_json_boolean;
typedef qn_string qn_json_string;
typedef qn_integer qn_json_integer;
typedef qn_number qn_json_number;

typedef struct _QN_JSON
{
    qn_json_class class;
    union {
        qn_json_object object;
        qn_json_array array;
        qn_json_string string;
        qn_json_integer integer;
        qn_json_number number;
        qn_json_boolean boolean;
    };
} qn_json; // _QN_JSON

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
