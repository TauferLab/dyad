#include "storage_check.h"
#include "ingester.h"
#include "utils.h"

// Note: 'struct local_storage_view' is replaced
//       with the 'storage_view_t' typedef in storage_view.h
storage_view_t* init_storage_view (ingester_methods_t method)
{
    storage_view_t* view = NULL;
    ingester_t* in = NULL;
    int rc = 0;
    view = create_storage_view ();
    if (view == NULL)
        return NULL;
    in = create_ingester (method);
    if (in == NULL) {
        free_storage_view (&view);
        return NULL;
    }
    rc = in->populate_storage_list (view);
    free_ingester (&in);
    if (rc != 0) {
        free_storage_view (&view);
        return NULL;
    }
    // Ignore the return value of this function
    // If it fails, all that it means is that the view
    // will take up more memory than needed because
    // realloc failed.
    constrain_storage_view_memory (view);
    return view;
}

storage_check_record_t* check_if_local_storage (storage_view_t* view, const char* fname)
{
    storage_check_record_t* record = NULL;
    json_t* packed_entry = NULL;
    bool is_found = false;
    storage_entry_t* found_entry = NULL;
    size_t i = 0;
    // Loop over all known storage devices
    for (i = 0; i < view->num_storage_devs; i++) {
        // Check if the provided file name is a subpath of
        // the mount point for the current device
        is_found = cmp_canonical_path_prefix (
            view->storage_devs[i]->mnt_pt,
            fname,
            NULL,
            0
        );
        // If the file name is a subpath of the mount point:
        if (is_found) {
            // Set the current entry if not yet set
            if (found_entry == NULL) {
                found_entry = view->storage_devs[i];
            } else {
                // Otherwise, only update the current entry
                // if the mount point is closer to the file name
                is_found = cmp_canonical_path_prefix (
                    found_entry->mnt_pt,
                    view->storage_devs[i]->mnt_pt,
                    NULL,
                    0
                );
                if (is_found)
                    found_entry = view->storage_devs[i];
            }
        }
    }
    // Allocate space for the record
    record = malloc (sizeof(struct storage_check_record));
    if (record == NULL)
        return NULL;
    record->tmp_entry = false;
    // If no entry was found in the for loop, create a temporary
    // "dummy" entry
    if (found_entry == NULL) {
        record->tmp_entry = true;
        found_entry = create_storage_entry ("", "", true);
        if (found_entry == NULL)
            free (record);
            return NULL;
    }
    // Pack the entry into a JSON
    // Should be "freed" with json_decref
    packed_entry = pack_storage_entry (found_entry);
    if (packed_entry == NULL)
        free (record);
        return NULL;
    // Populate and return the record
    record->packed_record = packed_entry;
    record->storage_entry_ptr = found_entry;
    record->is_local = found_entry->is_local;
    return record;
}

void free_storage_check_record (storage_check_record_t** record)
{
    if (record == NULL || *record == NULL)
        return;
    if ((*record)->packed_record != NULL)
        json_decref ((*record)->packed_record);
    if ((*record)->tmp_entry)
        free_storage_entry (&((*record)->storage_entry_ptr));
    free (*record);
    *record = NULL;
    return;
}

void finalize_storage_view (storage_view_t** view)
{
    free_storage_view (view);
}