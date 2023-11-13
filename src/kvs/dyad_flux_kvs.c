#include "dyad_flux_kvs.h"

dyad_rc_t dyad_kvs_flux_init (dyad_kvs_t** kvs_handle,
                              dyad_kvs_mode_t mode,
                              const char* restrict kvs_namespace,
                              flux_t* h,
                              bool debug)
{
    (*kvs_handle)->h = h;
    (*kvs_handle)->debug = debug;
    (*kvs_handle)->kvs_namespace = (char*)malloc (strlen (kvs_namespace) + 1);
    if ((*kvs_handle)->kvs_namespace == NULL)
        return DYAD_RC_SYSFAIL;
    memset ((*kvs_handle)->kvs_namespace, '\0', strlen (kvs_namespace) + 1);
    strncpy ((*kvs_handle)->kvs_namespace, kvs_namespace, strlen (kvs_namespace) + 1);
    (*kvs_handle)->kvs_read = dyad_kvs_flux_read;
    (*kvs_handle)->kvs_write = dyad_kvs_flux_write;
    return DYAD_RC_OK;
}

dyad_rc_t dyad_kvs_flux_read (dyad_kvs_t* restrict self,
                              const char* restrict key,
                              bool should_wait,
                              dyad_metadata_t** restrict mdata)
{
    dyad_rc_t rc = DYAD_RC_OK;
    dyad_kvs_flux_t* kvs_handle = self->private.flux_kvs_handle;
    int kvs_lookup_flags = 0;
    flux_future_t* f = NULL;
    if (mdata == NULL) {
        FLUX_LOG_ERR (kvs_handle->h,
                      "Metadata double pointer is NULL. Cannot correctly create metadata "
                      "object");
        rc = DYAD_RC_NOTFOUND;
        goto kvs_read_end;
    }
    // Lookup information about the desired file (represented by kvs_topic)
    // from the Flux KVS. If there is no information, wait for it to be
    // made available
    if (should_wait)
        kvs_lookup_flags = FLUX_KVS_WAITCREATE;
    DYAD_LOG_INFO (ctx, "Retrieving information from KVS under the key %s\n", topic);
    f = flux_kvs_lookup (kvs_handle->h, ctx->kvs_namespace, kvs_lookup_flags, topic);
    // If the KVS lookup failed, log an error and return DYAD_BADLOOKUP
    if (f == NULL) {
        DYAD_LOG_ERR (ctx, "KVS lookup failed!\n");
        rc = DYAD_RC_NOTFOUND;
        goto kvs_read_end;
    }
    // Extract the rank of the producer from the KVS response
    DYAD_LOG_INFO (ctx, "Building metadata object from KVS entry\n");
    if (*mdata != NULL) {
        DYAD_LOG_INFO (ctx, "Metadata object is already allocated. Skipping allocation");
    } else {
        *mdata = (dyad_metadata_t*)malloc (sizeof (struct dyad_metadata));
        if (*mdata == NULL) {
            DYAD_LOG_ERR (ctx, "Cannot allocate memory for metadata object");
            rc = DYAD_RC_SYSFAIL;
            goto kvs_read_end;
        }
    }
    size_t topic_len = strlen (topic);
    (*mdata)->fpath = (char*)malloc (topic_len + 1);
    if ((*mdata)->fpath == NULL) {
        DYAD_LOG_ERR (ctx, "Cannot allocate memory for fpath in metadata object");
        rc = DYAD_RC_SYSFAIL;
        goto kvs_read_end;
    }
    memset ((*mdata)->fpath, '\0', topic_len + 1);
    strncpy ((*mdata)->fpath, topic, topic_len);
    rc = flux_kvs_lookup_get_unpack (f, "i", &((*mdata)->owner_rank));
    // If the extraction did not work, log an error and return DYAD_BADFETCH
    if (rc < 0) {
        DYAD_LOG_ERR (ctx, "Could not unpack owner's rank from KVS response\n");
        rc = DYAD_RC_BADMETADATA;
        goto kvs_read_end;
    }
    rc = DYAD_RC_OK;

kvs_read_end:
    if (DYAD_IS_ERROR (rc) && mdata != NULL && *mdata != NULL) {
        dyad_free_metadata (mdata);
    }
    if (f != NULL) {
        flux_future_destroy (f);
        f = NULL;
    }
    return rc;
}

dyad_rc_t dyad_kvs_flux_write (dyad_kvs_t* restrict self,
                               const char* restrict key,
                               const dyad_metadata_t* restrict mdata)
{
}

dyad_rc_t dyad_kvs_flux_finalize (dyad_kvs_t** self)
{
    if ((*self)->private.flux_kvs_handle->kvs_namespace != NULL)
        free ((*self)->private.flux_kvs_handle->kvs_namespace);
    return DYAD_RC_OK;
}