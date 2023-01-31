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
        std::cerr << "Usage: ./cpp_cons <# of Files Transferred> <Consumer Directory>\n";
        return -1;
    }
    // Ensure a valid number of file transfers were provided
    int32_t num_transfers = (int32_t) atol(argv[1]);
    if (num_transfers <= 0)
    {
        std::cerr << "Either an invalid number of transfers was provided, \
                or an error occured in parsing the argument!\n";
        return -1;
    }
    // Get the Consumer Directory from command line and
    // make sure the directory exists
    char* fpath = argv[2];
    struct stat finfo;
    if (stat(fpath, &finfo) != 0 || !S_ISDIR(finfo.st_mode))
    {
        std::cerr << "The provided directory (" << fpath << ") does not exist!\n";
        return -1;
    }
    std::string cpp_fpath = fpath;
    std::string full_path;
    int32_t* val_buf = ALLOC_VAL_BUF();
    int num_chars;
    size_t items_read;
    for (int32_t seed = 0; seed < num_transfers; seed++)
    {
        memset(val_buf, 0, VAL_BUF_SIZE);
        full_path = cpp_fpath + "/data" + std::to_string(seed) + ".txt";
        dyad::ifstream_dyad ifs_dyad;
        try
        {
            ifs_dyad.open(full_path, std::ifstream::in | std::ios::binary);
        }
        catch (std::ios_base::failure)
        {
            std::cerr << "Could not open file: " << full_path << "\n";
            std::cout << "File " << seed << ": Cannot open\n";
            continue;
        }
        std::ifstream& ifs = ifs_dyad.get_stream();
        try
        {
            ifs.read((char*) val_buf, VAL_BUF_SIZE);
        }
        catch (std::ios_base::failure)
        {
            ifs_dyad.close();
            std::cerr << "Could not read the full file (" << full_path << ")\n";
            std::cout << "File " << seed << ": Cannot read\n";
            continue;
        }
        ifs_dyad.close();
        if (vals_are_valid(seed, val_buf))
        {
            std::cout << "File " << seed << ": OK\n";
        }
        else
        {
            std::cout << "File " << seed << ": BAD\n";
        }
    }
    free(val_buf);
}
