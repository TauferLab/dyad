#include "linux_mounts_ingester.h"
#include "storage_view.h"

#include <mntent.h>
#include <stdio.h>

static const size_t NUM_LOCAL_TYPES = 8;
static const char* LOCAL_TYPES[] = {
    "ext2", // 2nd Extended Filesystem
    "ext3", // 3rd Extended Filesystem
    "ext4", // 4th Extended Filesystem
    "xfs", // XFS File System
    "gfs2", // GFS2 File System
    "ramfs", // Virtual Memory FS that is dynamically resizable
             // (i.e., no max size besides amount of RAM)
    "tmpfs", // Virtual Memory FS that has a static size, but
             // can overflow into swap
    "dax", // Extension FS type that treats persistent storage
           // as byte-addressable memory
    
};
static const size_t NUM_REMOTE_TYPES = 4;
static const char* REMOTE_TYPES[] = {
    MNTTYPE_NFS, // Default NFS type from Linux kernel
    "nfs", // Basic NFS type
    "lustre", // Lustre Parallel File System
    "gpfs", // IBM Spectrum Scale Storage
};

static inline bool string_in_list (const char** list, const size_t list_size,
    const char* string)
{
    size_t i = 0;
    for (i = 0; i < list_size; i++) {
        if (strcmp (string, list[i]) == 0)
            return true;
    }
    return false;
}

// Return 1 on sucess but not EOF
// Return 0 on EOF
// Return -1 on error
static inline int parse_and_populate_proc_mounts_line (storage_view_t* view,
    FILE* mounts_fp)
{
    int rc = 0;
    struct mntent* curr_entry = NULL;
    storage_entry_t* storage_graph_entry = NULL;
    bool is_local = true;
    curr_entry = getmntent (mounts_fp);
    if (curr_entry == NULL) {
        if (feof (mounts_fp) != 0)
            return 0;
        return -1;
    }
    if (string_in_list (LOCAL_TYPES, NUM_LOCAL_TYPES, curr_entry->mnt_type))
        is_local = true;
    else if (string_in_list (REMOTE_TYPES, NUM_REMOTE_TYPES, curr_entry->mnt_type))
        is_local = false;
    else
        return 1;
    storage_graph_entry = create_storage_entry (
        curr_entry->mnt_dir,
        curr_entry->mnt_fsname,
        is_local
    );
    if (storage_graph_entry == NULL)
        return -1;
    rc = add_storage_entry (view, storage_graph_entry);
    if (rc != 0) {
        free_storage_entry (storage_graph_entry);
        return -1;
    }
    return 1;
}

int populate_from_linux_proc_mounts (storage_view_t* view)
{
    int rc = 0;
    FILE* mounts_fp = NULL;
    mounts_fp = setmntent("/proc/self/mounts", "r");
    if (mounts_fp == NULL)
        return -1;
    do {
        rc = parse_and_populate_proc_mounts_line (view, mounts_fp);
    } while (rc == 1);
    endmntent (mounts_fp);
    return rc;
}