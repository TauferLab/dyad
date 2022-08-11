#include "core.h"
#include "dyad_err.h"
#include "utils.h"

#include <string.h>

FILE *fopen_real(const char *path, const char *mode)
{
    typedef FILE *(*fopen_real_ptr_t) (const char *, const char *);
    fopen_real_ptr_t func_ptr = (fopen_real_ptr_t) dlsym (RTLD_NEXT, "fopen");
    char *error = NULL;

    if ((error = dlerror ())) {
        // TODO log error
        return NULL;
    }

    return (func_ptr (path, mode));
}

FILE *dyad_fopen(dyad_ctx_t *ctx, const char *path, const char *mode)
{
    if (ctx->intercept)
    {
        return fopen_real(path, mode);
    }
    return fopen(path, mode);
}

int fclose_real(FILE *fp)                 
{                                         
    typedef int (*fclose_real_ptr_t) (FILE *);
    fclose_real_ptr_t func_ptr = (fclose_real_ptr_t) dlsym (RTLD_NEXT, "fclose");
    char *error = NULL;                   
                                          
    if ((error = dlerror ())) {           
        DPRINTF ("DYAD: fclose() error in dlsym: %s\n", error);
        return EOF; // return the failure code
    }

    return func_ptr (fp);
}

int dyad_fclose(dyad_ctx_t *ctx, FILE *fp)
{
    if (ctx->intercept)
    {
        return fclose_real(fp);
    }
    return fclose(fp);
}
                                          
int dyad_init(bool debug, bool check, bool shared_storage,
        unsigned int key_depth, unsigned int key_bins, 
        const char *kvs_namespace, const char *managed_path,
        dyad_ctx_t **ctx)
{
    if (*ctx != NULL)
    {
        if ((*ctx)->initialized)
        {
            // TODO Indicate already initialized
        }                                 
        else                              
        {                                 
            **ctx = dyad_ctx_default;     
        }                                 
        return DYAD_OK;                   
    }                                     
    *ctx = (dyad_ctx_t*) malloc(sizeof(dyad_ctx_t));
    if (*ctx == NULL)
    {
        // TODO Indicate error       
        return DYAD_NOCTX;
    }                                
    **ctx = dyad_ctx_default;        
    (*ctx)->debug = debug;           
    (*ctx)->check = check;           
    (*ctx)->shared_storage = shared_storage;
    (*ctx)->key_depth = key_depth;   
    (*ctx)->key_bins = key_bins;
    (*ctx)->kvs_namespace = (char*) malloc(strlen(kvs_namespace)+1);
    strncpy(
        (*ctx)->kvs_namespace,
        kvs_namespace,
        strlen(kvs_namespace)+1
    );
    (*ctx)->managed_path = (char*) malloc(strlen(managed_path)+1);
    strncpy(
        (*ctx)->managed_path,
        managed_path,
        strlen(managed_path)+1
    );
    (*ctx)->reenter = true;
    (*ctx)->h = flux_open(NULL, 0);
    if ((*ctx)->h == NULL)
    {
        // TODO Indicate error
        return DYAD_FLUXFAIL;
    }
    if (flux_get_rank((*ctx)->h, &((*ctx)->rank)) < 0)
    {
        // TODO Indicate error
        return DYAD_FLUXFAIL;
    }
    (*ctx)->initialized = true;
    // TODO Print logging info
    return DYAD_OK;
}

int dyad_produce(dyad_ctx_t *ctx, const char *fname)
{
    return dyad_commit(ctx, fname);
}

int dyad_consume(dyad_ctx_t *ctx, const char *fname)
{
    dyad_kvs_response_t *resp = NULL;
    int rc;
    rc = dyad_fetch(ctx, fname, &resp);
    if (rc != 0)
    {
        // TODO Indicate error
        if (resp != NULL)
        {
            dyad_free_kvs_response(resp);
        }
        return rc;
    }
    rc = dyad_pull(ctx, fname, resp);
    dyad_free_kvs_response(resp);
    if (rc != 0)
    {
        // TODO Indicate error
        return rc;
    };
    return DYAD_OK;
}

int dyad_kvs_commit(dyad_ctx_t *ctx, flux_kvs_txn_t *txn,
        const char *upath, flux_future_t **f)
{
    *f = flux_kvs_commit(ctx->h, ctx->kvs_namespace,
            0, txn);
    if (*f == NULL)
    {
        // TODO FLUX_LOG_ERR
        return DYAD_BADCOMMIT;
    }
    flux_future_wait_for(*f, -1.0);
    flux_future_destroy(*f);
    *f = NULL;
    flux_kvs_txn_destroy(txn);
    return DYAD_OK;
}

int publish_via_flux(dyad_ctx_t* ctx, const char *upath)
{
    const char *managed_path = ctx->managed_path;
    flux_kvs_txn_t *txn = NULL;
    flux_future_t *f = NULL;
    const size_t topic_len = PATH_MAX;
    char topic[topic_len+1];
    memset(topic, '\0', topic_len+1);
    gen_path_key(
        upath,
        topic,
        topic_len,
        ctx->key_depth,
        ctx->key_bins
    );
    if (ctx->h == NULL)
    {
        // TODO Set correct ret val
        return DYAD_NOCTX;
    }
    // TODO FLUX_LOG_INFO
    txn = flux_kvs_txn_create();
    if (txn == NULL)
    {
        // TODO Log err
        return DYAD_FLUXFAIL;
    }
    if (flux_kvs_txn_pack(txn, 0, topic, "i", ctx->rank) < 0)
    {
        // TODO Log err
        return DYAD_FLUXFAIL;
    }
    int ret = dyad_kvs_commit(ctx, txn, upath, &f);
    if (ret < 0)
    {
        // TODO log error
        return ret;
    }
    return DYAD_OK;
}
                                     
int dyad_commit(dyad_ctx_t *ctx, const char *fname)
{ 
    if (ctx == NULL)
    {
        // TODO log
        return DYAD_NOCTX;
    }
    int rc = -1;
    char upath[PATH_MAX] = {'\0'};
    if (!cmp_canonical_path_prefix (ctx->managed_path, fname, upath, PATH_MAX)) 
    {
        // TODO log error
        rc = 0;
        goto commit_done;
    }
    ctx->reenter = false;
    rc = publish_via_flux(ctx, upath);
    ctx->reenter = true;

commit_done:
    if (rc == DYAD_OK && (ctx && ctx->check))
    {
        setenv(DYAD_CHECK_ENV, "ok", 1);
    }
    return rc;
}

int dyad_kvs_lookup(dyad_ctx_t *ctx, const char* kvs_topic, uint32_t *owner_rank)
{
    int rc = -1;
    flux_future_t *f = NULL;
    f = flux_kvs_lookup(ctx->h,
            ctx->kvs_namespace,
            FLUX_KVS_WAITCREATE,          
            kvs_topic                     
        );                                
    if (f == NULL)                        
    {                                     
        // TODO log                       
        return DYAD_BADLOOKUP;                      
    }
    rc = flux_kvs_lookup_get_unpack(f, "i", owner_rank);
    if (f != NULL)
    {
        flux_future_destroy(f);
        f = NULL;
    }
    if (rc < 0)
    {
        // TODO log
        return DYAD_BADFETCH;
    }
    return DYAD_OK;
}                                         
                                          
int dyad_fetch(dyad_ctx_t *ctx, const char* fname, 
        dyad_kvs_response_t **resp)       
{
    if (ctx == NULL)
    {
        // TODO log
        return DYAD_NOCTX;
    }
    int rc = -1;
    char upath[PATH_MAX] = {'\0'};
    if (!cmp_canonical_path_prefix(ctx->managed_path, fname, upath, PATH_MAX))
    {
        // TODO log error                 
        return 0;
    }                                     
    ctx->reenter = false;                 
    uint32_t owner_rank = 0;              
    const size_t topic_len = PATH_MAX;    
    char topic[topic_len+1];
    memset(topic, '\0', topic_len+1);
    gen_path_key(
        upath,
        topic,                            
        topic_len,                        
        ctx->key_depth,                   
        ctx->key_bins                     
    );                                    
    if (ctx->h == NULL)                   
    {                                     
        // TODO log                       
        return DYAD_NOCTX;                
    }                                     
    rc = dyad_kvs_lookup(ctx, topic, &owner_rank);
    if (rc != DYAD_OK)                    
    {
        // TODO log error
        return rc;
    }
    if (ctx->shared_storage || (owner_rank == ctx->rank))
    {
        return 0;
    }                                     
    *resp = malloc(sizeof(struct dyad_kvs_response));
    if (*resp == NULL)                    
    {                                     
        // TODO log error                 
        return DYAD_BADRESPONSE;                  
    }                                     
    (*resp)->fpath = malloc(strlen(upath)+1);
    strncpy(
        (*resp)->fpath,
        upath,
        strlen(upath)+1
    );                                    
    (*resp)->owner_rank = owner_rank;     
    return DYAD_OK;                       
}                                         
                                          
int dyad_rpc_get(dyad_ctx_t *ctx, dyad_kvs_response_t *kvs_data,
        const char **file_data, int *file_len)
{                                         
    int rc = -1;                          
    flux_future_t *f = NULL;              
    f = flux_rpc_pack(ctx->h,             
            "dyad.fetch",                 
            kvs_data->owner_rank,         
            0,                            
            "{s:s}",                      
            "upath",
            kvs_data->fpath
        );
    if (f == NULL)
    {
        // TODO log
        return DYAD_BADRPC;
    }
    rc = flux_rpc_get_raw(f, (const void**) file_data, file_len);
    if (f != NULL)
    {
        flux_future_destroy(f);
        f = NULL;
    }
    if (rc < 0)
    {
        // TODO log
        return DYAD_BADRPC;
    }
    return DYAD_OK;
}                                         
                                          
int dyad_pull(dyad_ctx_t *ctx, const char* fname, 
        dyad_kvs_response_t *kvs_data)    
{                                         
    if (ctx == NULL)                      
    {                                     
        // TODO log
        return DYAD_NOCTX;
    }
    int rc = -1;
    const char *file_data = NULL;
    int file_len = 0;
    const char *odir = NULL;
    FILE *of = NULL;
    char file_path [PATH_MAX+1] = {'\0'}; 
    char file_path_copy [PATH_MAX+1] = {'\0'};

    rc = dyad_rpc_get(ctx, kvs_data, &file_data, &file_len);
    if (rc != DYAD_OK)                    
    {                                     
        goto pull_done;
    }                                     
                                          
    strncpy (file_path, consumer_path, PATH_MAX-1);
    concat_str (file_path, user_path, "/", PATH_MAX);
    strncpy (file_path_copy, file_path, PATH_MAX); // dirname modifies the arg

    // Create the directory as needed     
    // TODO: Need to be consistent with the mode at the source
    odir = dirname (file_path_copy);      
    m = (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_ISGID);
    if ((strncmp (odir, ".", strlen(".")) != 0) &&
        (mkdir_as_needed (odir, m) < 0)) {
        // TODO log that dirs could not be created
        rc = DYAD_BADFIO;                 
        goto pull_done;                   
    }                                     
                                          
    of = dyad_fopen(ctx, file_path, "w");           
    if (of == NULL)                       
    {                                     
        // TODO log that file could not be opened
        rc = DYAD_BADFIO;               
        goto pull_done;
    }                                     
    size_t written_len = fwrite(file_data, sizeof(char), (size_t) file_len, of);
    if (written_len != (size_t) file_len)
    {
        // TODO log that write did not work
        rc = DYAD_BADFIO;
        goto pull_done;
    }
    rc = dyad_fclose(ctx, of);
    ctx->reenter = true;
    if (rc != 0)
    {
        rc = DYAD_BADFIO;
        goto pull_done;
    }
    rc = DYAD_OK;

pull_done:
    if (rc == DYAD_OK && (ctx && ctx->check))
        setenv (DYAD_CHECK_ENV, "ok", 1);
    return DYAD_OK;
}

int dyad_free_kvs_response(dyad_kvs_response_t *resp)
{                                         
    if (resp == NULL)                     
    {                                     
        return 0;                         
    }                                     
    free(resp->fpath);                    
    free(resp);
    resp = NULL;
    return 0;
}

int dyad_finalize(dyad_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return 0;
    }
    flux_close(ctx->h);
    free(ctx->kvs_namespace);
    free(ctx->managed_path);
    free(ctx);
    ctx = NULL;
    return 0;
}
