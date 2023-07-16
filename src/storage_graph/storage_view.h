#ifndef STORAGE_VIEW_H
#define STORAGE_VIEW_H

#include <jansson.h>

#ifdef __cplusplus
#include <cstdlib>

extern "C" {
#else
#include <stdlib.h>
#include <stdbool.h>
#endif

struct storage_entry {
    char* mnt_pt;
    char* mnt_name;
    bool is_local;
};
typedef struct storage_entry storage_entry_t;

storage_entry_t* create_storage_entry (const char* mount_pt, const char* mount_name,
    bool is_local);

json_t* pack_storage_entry (const storage_entry_t* entry);

storage_entry_t* unpack_storage_entry (json_t* packed_entry);

bool check_storage_entry_eq (const storage_entry_t* lhs, const storage_entry_t* rhs);

void free_storage_entry (storage_entry_t** entry);

struct local_storage_view {
    size_t num_storage_devs;
    size_t capacity;
    storage_entry_t** storage_devs;
};
typedef struct local_storage_view storage_view_t;

storage_view_t* create_storage_view ();

int add_storage_entry (storage_view_t* view, storage_entry_t* dev);

int constrain_storage_view_memory (storage_view_t* view);

void free_storage_view (storage_view_t** view);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_VIEW_H */