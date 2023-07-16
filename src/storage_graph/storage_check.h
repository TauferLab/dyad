#ifndef STORAGE_CHECK_H
#define STORAGE_CHECK_H

#include "ingester_methods.h"
#include "storage_view.h"

struct storage_check_record {
    json_t* packed_record;
    storage_entry_t* storage_entry_ptr;
    bool tmp_entry;
    bool is_local;
};
typedef struct storage_check_record storage_check_record_t;

storage_view_t* init_storage_view (ingester_methods_t method);

storage_check_record_t* check_if_local_storage (
    storage_view_t* view, const char* fname);

void free_storage_check_record (storage_check_record_t** record);

void finalize_storage_view (struct local_storage_view** view);

#endif /* STORAGE_CHECK_H */