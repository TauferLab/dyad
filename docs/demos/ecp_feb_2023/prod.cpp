#include "generation.h"

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <string>

#include <dyad_stream_api.hpp>

int main(int argc, char** argv)
{
    // Print an error and abort if an invalid number of arguments
    // were provided
    if (argc != 3)
    {
        fprintf(stderr, "Usage: ./cpp_prod <# of Files Transferred> <Producer Directory>\n");
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
    std::string cpp_fpath = fpath;
    std::string full_path;
    int num_chars;
    size_t items_read;
    for (int32_t seed = 0; seed < num_transfers; seed++)
    {
        int32_t* val_buf = generate_vals(seed);
        full_path = cpp_fpath + "/data" + std::to_string(seed) + ".txt";
        dyad::ofstream_dyad ofs_dyad;
        try
        {
            ofs_dyad.open(full_path, std::ofstream::out | std::ios::binary);
        }
        catch (std::ios_base::failure)
        {
            std::cerr << "Cannot open file: " << full_path << "\n";
            std::cout << "File " << seed << ": Cannot open\n";
            continue;
        }
        std::ofstream& ofs = ofs_dyad.get_stream();
        try
        {
            ofs.write((char*) val_buf, VAL_BUF_SIZE);
        }
        catch (std::ios_base::failure)
        {
            ofs_dyad.close();
            free(val_buf);
            std::cerr << "Could not write the full file (" << full_path << ")\n";
            std::cout << "File " << seed << ": Cannot write\n";
            continue;
        }
        free(val_buf);
    }
}
