#include <assert.h>

#include "qiniu/ud/variable.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- User-Defined Variable Functions (abbreviation: ud_var) ----

QN_SDK qn_bool qn_ud_var_set_raw(qn_ud_variable_ptr restrict udv, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size)
{
    qn_bool ret = qn_false;
    qn_string tmp_key = NULL;

    assert(key);
    assert(0 < key_size);

    if (2 <= key_size) {
        // Case 1: x:name
        // Case 2: x:
        if (posix_strncmp("x:", key, 2) == 0) return qn_etbl_set_raw((qn_etable_ptr) udv, key, key_size, val, val_size);

        if (posix_strncmp("X:", key, 2) == 0) {
            // Case 3: X:age
            // Case 4: X:
            tmp_key = qn_cs_sprintf("x:%s", key + 2);
        } else {
            // Case 5: gender
            // Case 6: no
            tmp_key = qn_cs_sprintf("x:%s", key);
        } // if
    } else if (key_size == 1) {
        // Case 7: G
        tmp_key = qn_cs_sprintf("x:%s", key);
    } else {
        qn_err_set_invalid_argument();
        return qn_false;
    } // if
    if (! tmp_key) return qn_false;

    ret = qn_etbl_set_raw((qn_etable_ptr) udv, qn_str_cstr(tmp_key), qn_str_size(tmp_key), val, val_size);
    qn_str_destroy(tmp_key);
    return ret;
}

#ifdef __cplusplus
}
#endif

