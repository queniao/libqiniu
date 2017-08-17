#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/service.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_SERVICE
{
    qn_svc_entry_st * entries;
    unsigned int ent_cnt:8;
    unsigned int ent_cap:8;
    unsigned int type:8;
} qn_service_st;

QN_SDK qn_service_ptr qn_svc_create(qn_svc_type type)
{
    qn_service_ptr new_svc = calloc(1, sizeof(qn_service_st));
    if (! new_svc) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_svc->ent_cnt = 0;
    new_svc->ent_cap = 4;
    new_svc->entries = calloc(new_svc->ent_cap, sizeof(qn_svc_entry_st));
    if (! new_svc->entries) {
        free(new_svc);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_svc->type = type;
    return new_svc;
}

QN_SDK void qn_svc_destroy(qn_service_ptr restrict svc)
{
    if (svc) {
        free(svc->entries);
        free(svc);
    } // if
}

QN_SDK unsigned int qn_svc_entry_count(qn_service_ptr restrict svc)
{
    assert(svc);
    return svc->ent_cnt;
}

QN_SDK qn_svc_entry_ptr qn_svc_get_entry(qn_service_ptr restrict svc, unsigned int n)
{
    assert(svc);
    if (svc->ent_cnt < n) return NULL;
    return &svc->entries[n];
}

static qn_bool qn_svc_augment(qn_service_ptr restrict svc)
{
    unsigned int new_cap = svc->ent_cap + (svc->ent_cap >> 1); // 1.5 times
    qn_svc_entry_st * new_entries = calloc(new_cap < 255 ? new_cap : 255, sizeof(qn_svc_entry_st));

    if (! new_entries) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_entries, svc->entries, sizeof(qn_svc_entry_st) * svc->ent_cnt);
    free(svc->entries);

    svc->entries = new_entries;
    svc->ent_cap = new_cap & 0xFF;
    return qn_true;
}

QN_SDK qn_bool qn_svc_add_entry(qn_service_ptr restrict svc, qn_svc_entry_ptr restrict ent)
{
    assert(svc);
    assert(ent);

    if (svc->ent_cnt == svc->ent_cap) {
        if (svc->ent_cap == 255) {
            qn_err_set_out_of_capacity();
            return qn_false;
        } // if
        if (! qn_svc_augment(svc)) return qn_false;
    } // if

    svc->entries[svc->ent_cnt].base_url = qn_str_duplicate(ent->base_url);
    if (! svc->entries[svc->ent_cnt].base_url) return qn_false;

    if (ent->hostname) {
        svc->entries[svc->ent_cnt].hostname = qn_str_duplicate(ent->hostname);
        if (! svc->entries[svc->ent_cnt].hostname) {
            qn_str_destroy(svc->entries[svc->ent_cnt].base_url);
            return qn_false;
        } // if
    } // if

    svc->ent_cnt += 1;
    return qn_true;
}

#ifdef __cplusplus
}
#endif
