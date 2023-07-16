#include "ingester.h"

// Include headers for ingesters here
#include "linux_mounts_ingester.h"

ingester_t* create_ingester (ingester_methods_t mode)
{
    ingester_t* in = malloc (sizeof(struct ingester));
    if (mode == LINUX_PROC_MOUNTS) {
        in->populate_storage_list = populate_from_linux_proc_mounts;
        return in;
    }
    free_ingester (&in);
    return NULL;
}

void free_ingester (ingester_t** in)
{
    if (in == NULL || *in == NULL)
        return;
    free (*in);
    *in = NULL;
    return;
}