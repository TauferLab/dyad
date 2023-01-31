#include "generation.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
    // Print an error and abort if an invalid number of arguments
    // were provided
    if (argc != 3)
    {
        fprintf(stderr, "Usage: ./c_prod <# of Files Transferred> <Producer Directory>\n");
        return -1;
    }
    // Ensure a valid number of file transfers were provided
    int32_t num_transfers = (int32_t) atol(argv[1]);
    if (num_transfers <= 0)
    {
        fprintf(stderr, "Either an invalid number of transfers was provided, \
                or an error occured in parsing the argument!\n");
        return -1;
    }
    // Get the Consumer Directory from command line and
    // make sure the directory exists
    char* fpath = argv[2];
    struct stat finfo;
    if (stat(fpath, &finfo) != 0 || !S_ISDIR(finfo.st_mode))
    {
        fprintf(stderr, "The provided directory (%s) does not exist!\n", fpath);
        return -1;
   }
    // Largest number of digits for a int32_t when converted to string
    const size_t max_digits = 10;
    // First 4 is for "data"
    // Second 4 is for ".txt"
    size_t path_len = strlen(fpath) + 4 + max_digits + 4;
    // Allocate a buffer for the file path
    char* full_path = malloc(path_len + 1);
    int num_chars;
    size_t items_read;
    for (int32_t seed = 0; seed < num_transfers; seed++)
    {
        // Clear full_path to be safe
        memset(full_path, '\0', path_len+1);
        // Generate the file's data
        int32_t* val_buf = generate_vals(seed);
        // Generate the file name
        num_chars = snprintf(full_path, path_len+1, "%s/data%ld.txt", fpath, (long) seed);
        // If the file name could not be generated, print an error
        // and abort
        if (num_chars < 0)
        {
            fprintf(stderr, "Could not generate file name for transfer %ld\n", (long) seed);
            printf("File %ld: Bad Name\n", (long) seed);
            free(val_buf);
            continue;
        }
        // Open the file or abort if not possible
        FILE* fp = fopen(full_path, "w");
        if (fp == NULL)
        {
            fprintf(stderr, "Cannot open file: %s\n", full_path);
            printf("File %ld: Cannot open\n", (long) seed);
            free(val_buf);
            continue;
        }
        // Read the file or abort if not possible
        items_read = fwrite(val_buf, sizeof(int32_t), VAL_BUF_SIZE, fp);
        fclose(fp);
        free(val_buf);
        if (items_read != NUM_VALS)
        {
            fprintf(stderr, "Could not write the full file (%s)\n", full_path);
            printf("File %ld: Cannot write\n", (long) seed);
            continue;
        }
    }
    free(full_path);
}
