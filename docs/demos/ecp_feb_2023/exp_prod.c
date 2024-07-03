#include "generation.h" /* Defines the following:
                         *  - parse_cmd_line
                         *  - generate_vals
                         *  - NUM_VALS
                         */
#include <dyad/c_api/dyad_c.h>
#include <string.h>
#include <unistd.h>

/**
 * Write the spcified file and record it in for transfer with DYAD
 *
 * @param[in] full_path Path to the file to read
 * @param[in] seed      The "seed value" (i.e., number from the for-loop in main)
 *                      for the file
 * @param[in] val_buf   Array in which the contents of the file are stored
 *
 * @return 0 if no errors occured. -1 otherwise.
 */
int produce_file (dyad_ctx_t* ctx, char* full_path, int32_t seed, int32_t* val_buf)
{
    // Open the file or abort if not possible
    // The open flags and mode are meant to mimic fopen based on the information from
    // this man page:
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/fopen.html
    int fd = dyad_c_open (ctx,
                          full_path,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd < 0) {
        fprintf (stderr, "Cannot open file: %s\n", full_path);
        printf ("File %ld: Cannot open\n", (long)seed);
        return -1;
    }
    // Write the file or abort if not possible
    ssize_t items_read = write (fd, val_buf, NUM_VALS * sizeof (int32_t));
    // Close the file
    dyad_c_close (ctx, fd);
    // If an error occured during writing, abort
    if (items_read != NUM_VALS * sizeof (int32_t)) {
        fprintf (stderr, "Could not write the full file (%s)\n", full_path);
        printf ("File %ld: Cannot write\n", (long)seed);
        return -1;
    }
    return 0;
}

int main (int argc, char** argv)
{
    // Parse command-line arguments
    int32_t num_transfers;
    char* fpath;
    int rc = parse_cmd_line (argc, argv, &num_transfers, &fpath);
    // If an error occured during command-line parsing,
    // abort the consumer
    if (rc != 0) {
        return rc;
    }
    dyad_ctx_t* ctx;
    rc = dyad_c_init (&ctx);
    if (rc < 0) {
        return rc;
    }
    // Largest number of digits for a int32_t when converted to string
    const size_t max_digits = 10;
    // First 4 is for "data"
    // Second 4 is for ".txt"
    size_t path_len = strlen (fpath) + 4 + max_digits + 4;
    // Allocate a buffer for the file path
    char* full_path = malloc (path_len + 1);
    int num_chars;
    for (int32_t seed = 0; seed < num_transfers; seed++) {
        // Clear full_path to be safe
        memset (full_path, '\0', path_len + 1);
        // Generate the file's data
        int32_t* val_buf = generate_vals (seed);
        // Generate the file name
        num_chars = snprintf (full_path, path_len + 1, "%s/data%ld.txt", fpath, (long)seed);
        // If the file name could not be generated, print an error
        // and abort
        if (num_chars < 0) {
            fprintf (stderr, "Could not generate file name for transfer %ld\n", (long)seed);
            printf ("File %ld: Bad Name\n", (long)seed);
            free (val_buf);
            continue;
        }
        /********************************************
         *     Perform DYAD Data Production!!!      *
         ********************************************/
        rc = produce_file (ctx, full_path, seed, val_buf);
        // Free the data that was saved to file
        free (val_buf);
        // If an error occured, continue
        if (rc != 0) {
            continue;
        }
    }
    free (full_path);
    dyad_c_finalize (&ctx);
}
