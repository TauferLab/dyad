#include "storage_view.h"

#include <stdint.h>
#include <string.h>

const size_t MAX_CAPACITY = SIZE_MAX / sizeof(const storage_entry_t*);

storage_entry_t* create_storage_entry (const char* mount_pt, const char* mount_name,
    bool is_local)
{
    storage_entry_t* entry = (storage_entry_t*) malloc (sizeof(struct storage_entry));
    if (entry == NULL) {
        goto failure;
    }
    entry->mnt_pt = (char*) malloc (strlen(mount_pt)+1);
    if (entry->mnt_pt == NULL) {
        goto failure;
    }
    entry->mnt_name = (char*) malloc (strlen(mount_name)+1);
    if (entry->mnt_name == NULL) {
        goto failure;
    }
    strcpy (entry->mnt_pt, mount_pt);
    strcpy (entry->mnt_name, mount_name);
    entry->is_local = is_local;
    return entry;
failure:;
    free_storage_entry (&entry);
    return NULL;
}

json_t* pack_storage_entry (const storage_entry_t* entry)
{
    json_t* packed_json = NULL;
    packed_json = json_pack (
        "[s, s, b]",
        entry->mnt_pt,
        entry->mnt_name,
        entry->is_local
    );
    return packed_json;
}

storage_entry_t* unpack_storage_entry (json_t* packed_entry)
{
    storage_entry_t* unpacked_entry = NULL;
    char* mount_pt = NULL;
    char* mount_name = NULL;
    bool is_local = false;
    int rc = 0;
    rc = json_unpack(
        packed_entry,
        "[s, s, b]",
        &mount_pt,
        &mount_name,
        &is_local
    );
    if (rc != 0)
        return NULL;
    unpacked_entry = create_storage_entry (
        mount_pt,
        mount_name,
        is_local
    );
    return unpacked_entry;
}

bool check_storage_entry_eq (const storage_entry_t* lhs,
    const storage_entry_t* rhs)
{
    size_t name_size = 0;
    name_size = strlen (lhs->mnt_name);
    if (name_size != strlen (rhs->mnt_name))
        return false;
    return (strncmp (lhs->mnt_name, rhs->mnt_name, name_size) == 0);
}

void free_storage_entry (storage_entry_t** entry)
{
    if (entry == NULL || *entry == NULL)
        return;
    if ((*entry)->mnt_name != NULL) {
        free ((*entry)->mnt_name);
        (*entry)->mnt_name = NULL;
    }
    if ((*entry)->mnt_pt != NULL) {
        free ((*entry)->mnt_pt);
        (*entry)->mnt_pt = NULL;
    }
    free (*entry);
    *entry = NULL;
    return;
}

storage_view_t* create_storage_view ()
{
    storage_view_t* view = (storage_view_t*) malloc (sizeof (struct local_storage_view));
    if (view == NULL) {
        goto failure;
    }
    view->storage_devs = NULL;
    view->num_storage_devs = 0;
    view->capacity = 0;
    return view;
failure:;
    free_storage_view (&view);
    return NULL;
}

int realloc_storage_view (storage_view_t* view)
{
    size_t new_capacity = 0;
    storage_entry_t** new_vec = NULL;
    if (view == NULL)
        return -1;
    if (view->capacity == 0) {
        // Arbitrary power-of-2 size that will
        // decrease the number of reallocations needed
        new_capacity = 8;
    } else {
        if (view->capacity == MAX_CAPACITY) {
            return -1;
        } else if (view->capacity > (MAX_CAPACITY / 2)) {
            new_capacity = MAX_CAPACITY;
        } else {
            new_capacity = view->capacity * 2;
        }
    }
    new_vec = realloc (
        view->storage_devs,
        new_capacity * sizeof (const storage_entry_t*)
    );
    if (new_vec == NULL) {
        return -1;
    }
    view->storage_devs = new_vec;
    view->capacity = new_capacity;
    // Initialize newly allocated slots
    for (size_t i = view->num_storage_devs; i < view->capacity; i++) {
        view->storage_devs[i] = NULL;
    }
    return 0;
}

int add_storage_entry (storage_view_t* view, storage_entry_t* dev)
{
    if (view == NULL)
        return -1;
    int rc = 0;
    if (view->num_storage_devs == view->capacity) {
        rc = realloc_storage_view (view);
        if (rc < 0) {
            return rc;
        }
    }
    view->storage_devs[view->num_storage_devs] = dev;
    view->num_storage_devs++;
    return 0;
}

int constrain_storage_view_memory (storage_view_t* view)
{
    storage_entry_t** new_vec;
    if (view == NULL || view->capacity < view->num_storage_devs)
        return -1;
    if (view->capacity == view->num_storage_devs)
        return 0;
    new_vec = realloc (
        view->storage_devs,
        view->num_storage_devs * sizeof (const storage_entry_t*)
    );
    if (new_vec == NULL) {
        return -1;
    }
    view->storage_devs = new_vec;
    view->capacity = view->num_storage_devs;
    return 0;
}

void free_storage_view (storage_view_t** view)
{
    if (view == NULL || *view == NULL)
        return;
    if ((*view)->storage_devs != NULL) {
        for (size_t i = 0; i < (*view)->num_storage_devs; i++) {
            if ((*view)->storage_devs[i] != NULL)
                free_storage_entry (&((*view)->storage_devs[i]));
        }
        free ((*view)->storage_devs);
        (*view)->storage_devs = NULL;
    }
    free (*view);
    *view = NULL;
    return;
}