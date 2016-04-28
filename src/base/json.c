#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
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
    QN_JSON_NULL = 1,
    QN_JSON_BOOLEAN = 2,
    QN_JSON_INTEGER = 3,
    QN_JSON_NUMBER = 4,
    QN_JSON_STRING = 5,
    QN_JSON_ARRAY = 6,
    QN_JSON_OBJECT = 7
} qn_json_class;

typedef enum _QN_JSON_STATUS
{
    QN_JSON_PARSING_KEY = 0,
    QN_JSON_PARSING_COLON = 1,
    QN_JSON_PARSING_VALUE = 2,
    QN_JSON_PARSING_COMMA = 3
} qn_json_status;

typedef struct _QN_JSON
{
    qn_eslink node;
    qn_json_hash hash;
    const char * key;
    struct {
        qn_json_class class:3;
        qn_json_status status:2;
    };
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
qn_json_hash qn_json_object_calculate_hash(const char * cstr)
{
    qn_json_hash hash = 5381;
    int c;

    while ((c = *cstr++) != '\0') {
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

//-- Implementation of qn_json_array

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

//-- Implementation of qn_json

qn_json_ptr qn_json_new_string(const char * cstr, qn_size cstr_size)
{
    qn_json_ptr new_str = NULL;
    qn_string_ptr str_data = NULL;

    new_str = calloc(1, sizeof(*new_str) + sizeof(new_str->str_data[0]) + cstr_size);
    if (new_str == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_str->class = QN_JSON_STRING;
    new_str->string = str_data = &new_str->str_data[0];
    qn_str_copy(str_data, cstr, cstr_size);
    return new_str;
} // qn_json_new_string

qn_json_ptr qn_json_new_integer(qn_integer val)
{
    qn_json_ptr new_int = NULL;

    new_int = calloc(1, sizeof(*new_int));
    if (new_int == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_int->class = QN_JSON_INTEGER;
    new_int->integer = val;
    return new_int;
} // qn_json_new_integer

qn_json_ptr qn_json_new_number(qn_number val)
{
    qn_json_ptr new_num = NULL;

    new_num = calloc(1, sizeof(*new_num));
    if (new_num == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_num->class = QN_JSON_NUMBER;
    new_num->number = val;
    return new_num;
} // qn_json_new_number

qn_json_ptr qn_json_new_boolean(qn_bool val)
{
    qn_json_ptr new_bool = NULL;

    new_bool = calloc(1, sizeof(*new_bool));
    if (new_bool == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_bool->class = QN_JSON_BOOLEAN;
    new_bool->boolean = val;
    return new_bool;
} // qn_json_new_boolean

qn_json_ptr qn_json_new_null(void)
{
    qn_json_ptr new_null = NULL;

    new_null = calloc(1, sizeof(*new_null));
    if (new_null == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    new_null->class = QN_JSON_NULL;
    return new_null;
} // qn_json_new_null

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

qn_json_ptr qn_json_get(qn_json_ptr self, const char * key)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr curr_node = NULL;
    qn_json_object_ptr obj_data = NULL;
    qn_json_hash hash = 0;
    int bucket = 0;

    assert(self && self->class == QN_JSON_OBJECT);
    assert(key);

    obj_data = &self->obj_data[0];
    if (obj_data->size == 0) {
        // The object is empty.
        return NULL;
    } // if

    hash = qn_json_object_calculate_hash(key);
    bucket = hash % QN_JSON_OBJECT_MAX_BUCKETS;

    qn_json_object_find(&obj_data->heads[bucket], hash, key, &prev_node, &curr_node);
    if (curr_node == &obj_data->heads[bucket]) {
        // The value according to the given key does not exist.
        return NULL;
    } // if
    return qn_eslink_super(curr_node, qn_json_ptr, node);
} // qn_json_get

qn_json_ptr qn_json_get_at(qn_json_ptr self, qn_size n)
{
    qn_eslink_ptr prev_node = NULL;
    qn_eslink_ptr curr_node = NULL;
    qn_json_array_ptr arr_data = NULL;

    assert(self && self->class == QN_JSON_ARRAY);

    arr_data = &self->arr_data[0];
    if (arr_data->size == 0 || arr_data->size < n) {
        // The array is empty or n exceeds the size of the array.
        return NULL;
    } // if

    qn_json_array_find(&arr_data->head, n, &prev_node, &curr_node);

    return qn_eslink_super(curr_node, qn_json_ptr, node);
} // qn_json_get_at

qn_string * qn_json_cast_to_string(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_STRING);
    return self->string;
} // qn_json_cast_to_string

qn_integer qn_json_cast_to_integer(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_INTEGER);
    return self->integer;
} // qn_json_cast_to_integer

qn_number qn_json_cast_to_number(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_NUMBER);
    return self->number;
} // qn_json_cast_to_number

qn_bool qn_json_cast_to_boolean(qn_json_ptr self)
{
    assert(self && self->class == QN_JSON_BOOLEAN);
    return self->boolean;
} // qn_json_cast_to_boolean

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

qn_bool qn_json_is_empty(qn_json_ptr self)
{
    if (self->class == QN_JSON_OBJECT) {
        return self->obj_data[0].size == 0;
    }
    if (self->class == QN_JSON_ARRAY) {
        return self->arr_data[0].size == 0;
    }
    return qn_false;
} // qn_json_is_empty

//-- Implementation of qn_json_scanner

typedef enum _QN_JSON_TOKEN
{
    QN_JSON_TKN_STRING = 0,
    QN_JSON_TKN_COLON,
    QN_JSON_TKN_COMMA,
    QN_JSON_TKN_INTEGER,
    QN_JSON_TKN_NUMBER,
    QN_JSON_TKN_TRUE,
    QN_JSON_TKN_FALSE,
    QN_JSON_TKN_NULL,
    QN_JSON_TKN_OPEN_BRACE,
    QN_JSON_TKN_CLOSE_BRACE,
    QN_JSON_TKN_OPEN_BRACKET,
    QN_JSON_TKN_CLOSE_BRACKET,
    QN_JSON_TKN_INPUT_END,
    QN_JSON_TKNERR_MALFORMED_TEXT,
    QN_JSON_TKNERR_NEED_MORE_TEXT,
    QN_JSON_TKNERR_NO_ENOUGH_MEMORY
} qn_json_token;

typedef struct _QN_JSON_SCANNER
{
    const char * buf;
    qn_size buf_size;
    qn_size pos;
} qn_json_scanner, *qn_json_scanner_ptr;

static
qn_json_token qn_json_scan_string(qn_json_scanner_ptr s, const char ** restrict txt, qn_size * restrict txt_size)
{
    char * cstr = NULL;
    qn_size primitive_len = 0;
    qn_size i = 0;
    qn_size m = 0;
    qn_size pos = s->pos + 1; // 跳过起始双引号

    for (; pos < s->buf_size; pos += 1) {
        if (s->buf[pos] == '\\') { m += 1; continue; }
        if (s->buf[pos] == '"' && s->buf[pos - 1] != '\\') { break; }
    } // for
    if (pos == s->buf_size) {
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    pos += 1; // 跳过终止双引号

    primitive_len = pos - s->pos - 2;
    cstr = malloc(primitive_len + 1);
    if (!cstr) {
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    memcpy(cstr, s->buf + s->pos + 1, primitive_len);
    cstr[primitive_len] = '\0';

    if (m > 0) {
        // 处理转义码
        for (i = 0, m = 0; i < primitive_len; i += 1) {
            // TODO: \u0026 形式的转义码转换
            if (cstr[i] == '\\') {
                i += 1;
                switch (cstr[i]) {
                    case 't': cstr[m++] = '\t'; break;
                    case 'n': cstr[m++] = '\n'; break;
                    case 'r': cstr[m++] = '\r'; break;
                    case 'f': cstr[m++] = '\f'; break;
                    case 'v': cstr[m++] = '\v'; break;
                    case 'b': cstr[m++] = '\b'; break;
                    default:  cstr[m++] = cstr[i];
                } // switch
            } else {
                cstr[m++] = cstr[i];
            } // if
        } // for
        cstr[m] = '\0';
        *txt_size = m;
    } else {
        *txt_size = primitive_len;
    } // if

    *txt = cstr;
    s->pos = pos;
    return QN_JSON_TKN_STRING;
} // qn_json_scan_string

static
qn_json_token qn_json_scan_number(qn_json_scanner_ptr s, const char ** restrict txt, qn_size * restrict txt_size)
{
    qn_json_token tkn = QN_JSON_TKN_INTEGER;
    char * cstr;
    qn_size primitive_len;
    qn_size pos = s->pos + 1; // 跳过已经被识别的首字符

    for (; pos < s->buf_size; pos += 1) {
        if (s->buf[pos] == '.') {
            if (tkn == QN_JSON_TKN_NUMBER) { break; }
            tkn = QN_JSON_TKN_NUMBER;
            continue;
        }
        if (s->buf[pos] < '0' || s->buf[pos] > '9') { break; }
    } // for
    if (pos == s->buf_size) {
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    primitive_len = pos - s->pos;
    cstr = malloc(primitive_len + 1);
    if (!cstr) {
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    memcpy(cstr, s->buf + s->pos, primitive_len);
    cstr[primitive_len] = '\0';

    *txt = cstr;
    *txt_size = primitive_len;
    s->pos = pos;
    return tkn;
} // qn_json_scan_number

static
qn_json_token qn_json_scan(qn_json_scanner_ptr s, const char ** restrict txt, qn_size * restrict txt_size)
{
    qn_size pos = s->pos;

    for (; pos < s->buf_size && isspace(s->buf[pos]); pos += 1) {
        // do nothing
    } // for
    if (pos == s->buf_size) {
        s->pos = pos;
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    }

    // 此处 s->pos 应指向第一个非空白符的字符
    switch (s->buf[pos]) {
        case ':': s->pos = pos + 1; return QN_JSON_TKN_COLON;
        case ',': s->pos = pos + 1; return QN_JSON_TKN_COMMA;
        case '{': s->pos = pos + 1; return QN_JSON_TKN_OPEN_BRACE;
        case '}': s->pos = pos + 1; return QN_JSON_TKN_CLOSE_BRACE;
        case '[': s->pos = pos + 1; return QN_JSON_TKN_OPEN_BRACKET;
        case ']': s->pos = pos + 1; return QN_JSON_TKN_CLOSE_BRACKET;

        case '"':
            s->pos = pos;
            return qn_json_scan_string(s, txt, txt_size);

        case '+': case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            s->pos = pos;
            return qn_json_scan_number(s, txt, txt_size);

        case 't': case 'T':
            if (pos + 4 <= s->buf_size) {
                if (strncmp(s->buf + pos, "true", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_TRUE;
                }
                if (strncmp(s->buf + pos, "TRUE", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_TRUE;
                }
            } // if
            break;

        case 'f': case 'F':
            if (pos + 5 <= s->buf_size) {
                if (strncmp(s->buf + pos, "false", 5) == 0) {
                    s->pos = pos + 5;
                    return QN_JSON_TKN_FALSE;
                }
                if (strncmp(s->buf + pos, "FALSE", 5) == 0) {
                    s->pos = pos + 5;
                    return QN_JSON_TKN_FALSE;
                }
            } // if
            break;

        case 'n': case 'N':
            if (pos + 4 <= s->buf_size) {
                if (strncmp(s->buf + pos, "null", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_NULL;
                }
                if (strncmp(s->buf + pos, "NULL", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_NULL;
                }
            } // if
            break;
    } // switch
    return QN_JSON_TKNERR_MALFORMED_TEXT;
} // qn_json_scan

typedef struct _QN_JSON_PARSER
{
    qn_eslink head;
    qn_json_scanner scanner;
} qn_json_parser;

qn_json_parser_ptr qn_json_new_parser(void)
{
    qn_json_parser_ptr new_prs = NULL;
    
    new_prs = calloc(1, sizeof(*new_prs));
    if (!new_prs) {
        errno = ENOMEM;
        return NULL;
    }
    qn_eslink_init(&new_prs->head);
    return new_prs;
} // qn_json_new_parser

void qn_json_delete_parser(qn_json_parser_ptr prs)
{
    qn_eslink_ptr curr_node = NULL;
    qn_json_ptr curr_json = NULL;

    if (prs) {
        for (curr_node = qn_eslink_next(&prs->head); curr_node != &prs->head; curr_node = qn_eslink_next(curr_node)) {
            curr_json = qn_eslink_super(curr_node, qn_json_ptr, node);
            qn_json_delete(curr_json);
        } // while
        free(prs);
    } // if
} // qn_json_delete_parser

static
qn_bool qn_json_put_in(
    qn_json_parser_ptr prs,
    const char * key,
    qn_json_token tkn,
    const char * txt,
    qn_size txt_size,
    qn_json_ptr owner_json)
{
    qn_json_integer integer = 0L;
    qn_json_number number = 0.0L;
    qn_json_ptr new_json = NULL;
    char * end_txt = NULL;

    switch (tkn) {
        case QN_JSON_TKN_OPEN_BRACE:
            new_json = qn_json_new_object();
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            qn_eslink_insert_after(&new_json->node, &prs->head);
            return qn_true;

        case QN_JSON_TKN_OPEN_BRACKET:
            new_json = qn_json_new_array();
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            qn_eslink_insert_after(&new_json->node, &prs->head);
            return qn_true;

        case QN_JSON_TKN_STRING:
            new_json = qn_json_new_string(txt, txt_size);
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_INTEGER:
            integer = strtoll(txt, &end_txt, 10);
            if (end_txt == txt) {
                // 未解析
                errno = EBADMSG;
                return qn_false;
            } // if
            if ((integer == LLONG_MAX || integer == LLONG_MIN) && errno == ERANGE) {
                // 溢出
                errno = EOVERFLOW;
                return qn_false;
            } // if
            new_json = qn_json_new_integer(integer);
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_NUMBER:
            number = strtold(txt, &end_txt);
            if (end_txt == txt) {
                // 未解析
                errno = EBADMSG;
                return qn_false;
            } // if
            if (number >= HUGE_VALL && number <= HUGE_VALL && errno == ERANGE) {
                // 上溢
                errno = EOVERFLOW;
                return qn_false;
            } // if
            if (number >= 0.0L && number <= 0.0L && errno == ERANGE) {
                // 下溢
                errno = EOVERFLOW;
                return qn_false;
            } // if
            new_json = qn_json_new_number(number);
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_TRUE:
            new_json = qn_json_new_boolean(qn_true);
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_FALSE:
            new_json = qn_json_new_boolean(qn_false);
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            break;

        case QN_JSON_TKN_NULL:
            new_json = qn_json_new_null();
            if (!new_json) {
                errno = ENOMEM;
                return qn_false;
            } // if
            break;

        case QN_JSON_TKNERR_NEED_MORE_TEXT:
            errno = EAGAIN;
            return qn_false;

        case QN_JSON_TKNERR_NO_ENOUGH_MEMORY:
            errno = ENOMEM;
            return qn_false;

        default:
            errno = EBADMSG;
            return qn_false;
    } // switch

    if (owner_json->class == QN_JSON_OBJECT) {
        qn_json_set(owner_json, owner_json->key, new_json);
    } // if
    if (owner_json->class == QN_JSON_ARRAY) {
        qn_json_push(owner_json, new_json);
    } // if
    return qn_true;
} // qn_json_put_in

static
qn_bool qn_json_parse_object(qn_json_parser_ptr prs)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    const char * txt = NULL;
    qn_size txt_size = 0;
    qn_json_ptr curr_obj = NULL;

    while (1) {
        curr_obj = qn_eslink_super(qn_eslink_next(&prs->head), qn_json_ptr, node);

        if (curr_obj->status == QN_JSON_PARSING_KEY) {
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                return qn_true;
            } // if
            if (tkn != QN_JSON_TKN_STRING) {
                errno = EBADMSG;
                return qn_false;
            } // if

            curr_obj->key = txt;
            curr_obj->status = QN_JSON_PARSING_COLON;
        } // if

        if (curr_obj->status == QN_JSON_PARSING_COLON) {
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (tkn != QN_JSON_TKN_COLON) {
                errno = EBADMSG;
                return qn_false;
            } // if

            curr_obj->status = QN_JSON_PARSING_VALUE;
        } // if

        if (curr_obj->status == QN_JSON_PARSING_VALUE) {
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (!qn_json_put_in(prs, curr_obj->key, tkn, txt, txt_size, curr_obj)) {
                return qn_false;
            } // if

            curr_obj->key = NULL;
            curr_obj->status = QN_JSON_PARSING_COMMA;

            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) {
                return qn_true;
            } // if
        } // if

        tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
        if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
            return qn_true;
        } // if
        if (tkn != QN_JSON_TKN_COMMA) {
            errno = EBADMSG;
            return qn_false;
        } // if
        curr_obj->status = QN_JSON_PARSING_KEY;
    } // while
    return qn_false;
} // qn_json_parse_object

static
qn_bool qn_json_parse_array(qn_json_parser_ptr prs)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    const char * txt = NULL;
    qn_size txt_size = 0;
    qn_json_ptr curr_arr = NULL;

    while (1) {
        curr_arr = qn_eslink_super(qn_eslink_next(&prs->head), qn_json_ptr, node);
        if (curr_arr->status == QN_JSON_PARSING_VALUE) {
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                return qn_true;
            } // if

            if (!qn_json_put_in(prs, NULL, tkn, txt, txt_size, curr_arr)) {
                return qn_false;
            } // if
            curr_arr->status = QN_JSON_PARSING_COMMA;

            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) {
                return qn_true;
            } // if
        } // if

        tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
        if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
            return qn_true;
        } // if
        if (tkn != QN_JSON_TKN_COMMA) {
            errno = EBADMSG;
            return qn_false;
        } // if

        curr_arr->status = QN_JSON_PARSING_VALUE;
    } // while
    return qn_false;
} // qn_json_parse_array

qn_bool qn_json_parse(qn_json_parser_ptr prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_ptr * ret_json)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    const char * txt = NULL;
    qn_size txt_size = 0;
    qn_json_ptr curr_json = NULL;
    qn_json_ptr owner_json = NULL;

    prs->scanner.buf = buf;
    prs->scanner.buf_size = *buf_size;
    prs->scanner.pos = 0;

    if (!qn_eslink_is_linked(&prs->head)) {
        tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
        if (tkn == QN_JSON_TKN_OPEN_BRACE) {
            curr_json = qn_json_new_object();
        } else if (tkn == QN_JSON_TKN_OPEN_BRACKET) {
            curr_json = qn_json_new_array();
        } else {
            errno = EBADMSG;
            return qn_false;
        } // if

        if (!curr_json) {
            errno = ENOMEM;
            return qn_false;
        } // if
        qn_eslink_insert_after(&curr_json->node, &prs->head);
    } // if

    while (1) {
        curr_json = qn_eslink_super(qn_eslink_next(&prs->head), qn_json_ptr, node);
        if (curr_json->class == QN_JSON_OBJECT && !qn_json_parse_object(prs)) {
            *buf_size = prs->scanner.pos;
            return qn_false;
        } else if (!qn_json_parse_array(prs)) {
            *buf_size = prs->scanner.pos;
            return qn_false;
        } // if

        qn_eslink_remove_after(qn_eslink_next(&prs->head), &prs->head);
        if (qn_eslink_is_linked(&prs->head)) {
            owner_json = curr_json;
            break;
        } // if

        owner_json = qn_eslink_super(qn_eslink_next(&prs->head), qn_json_ptr, node);
        if (owner_json->class == QN_JSON_OBJECT) {
            qn_json_set(owner_json, owner_json->key, curr_json);
        } else {
            qn_json_push(owner_json, curr_json);
        } // if
    } // while

    *buf_size = prs->scanner.pos;
    *ret_json = owner_json;
    return qn_true;
} // qn_json_parse

#ifdef __cplusplus
}
#endif
