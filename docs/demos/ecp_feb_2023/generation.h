#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>

extern "C"
{
#else
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#endif // __cplusplus

// Number of values per file
#define NUM_VALS 10

// Simple macros to get the size of the value buffer
// (in bytes) and to perform allocation of the value buffer
#define VAL_BUF_SIZE NUM_VALS*sizeof(int32_t)
#define ALLOC_VAL_BUF() (int32_t*)malloc(VAL_BUF_SIZE)

// Generate a deterministic set of values
// given a seed value
int32_t* generate_vals(int32_t seed)
{
    int32_t start_val = seed * NUM_VALS;
    int32_t* val_buf = ALLOC_VAL_BUF();
    for (int32_t v = start_val; v < start_val + NUM_VALS; v++)
    {
        val_buf[v - start_val] = v;
    }
    return val_buf;
}

// Verify a set of values that should have been originally
// produced by generate_vals
bool vals_are_valid(int32_t seed, int32_t* val_buf)
{
    int32_t start_val = seed * NUM_VALS;
    bool is_valid = true;
    for (int32_t v = start_val; v < start_val + NUM_VALS; v++)
    {
        is_valid = is_valid & (val_buf[v - start_val] == v);
    }
    return is_valid;
}

#ifdef __cplusplus
}
#endif
