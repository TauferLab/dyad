#!/bin/bash

# Usage/help printout
function usage {
    cat << EOF
Usage: ./autoformat.sh [OPTIONS]

Options:
========
  * --exclude_fname | -en <FILE_PATTERN>    exclude files matching the provided pattern. 
                                            Multiple "--exclude_fname" flags will add 
                                            multiple patterns to exclude
  * --exclude_path | -ep <PATH_PATTERN>     exclude paths matching the provided pattern.
                                            Multiple "--exclude_path" flags will add
                                            multiple patterns to exclude
  * --dummy | -d                            prints the formatting commands instead of running them
  * --help | -h                             print this usage statement
EOF
}

# Path patterns to exclude from search
path_exc_patterns=(  )
# File patterns to exclude from search
name_exc_patterns=(  )
# Run in dummy mode?
dummy=0

# Parse cmd-line args
while test $# -gt 0; do
    case "$1" in
        --exclude_fname | -en)
            shift
            name_exc_patterns+=("$1")
            ;;
        --exclude_path | -ep)
            shift
            path_exc_patterns+=("$1")
            ;;
        --dummy | -d)
            dummy=1
            ;;
        --help | -h)
            usage
            exit 0
            ;;
        *)
            echo "Invalid argument ($1)!"
            usage
            exit -1
            ;;
    esac
    shift
done

# Get the directory of the script (which should always be <DYAD Repo Root>/scripts)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# Get the directory from which the user is running this script
curr_dir=$(pwd)

# Move to the root directory of the DYAD repo
cd $SCRIPT_DIR
cd ..

# The directories in the repo to consider for formatting
# Note: update as needed
code_dirs=( ./src )

for d in ${code_dirs[@]}; do
    # Generate a "find" command to locate all C/C++ code
    find_cmd="find $d -type f ( -name \"*.hpp\" -or -name \"*.cpp\" -or -name \"*.h\" -or -name \"*.c\" )"
    # If exclude patterns were provided, add them to the "find" command
    for name_exc in "${name_exc_patterns[@]}"; do
        find_cmd="$find_cmd -not -name $name_exc"
    done
    for path_exc in "${path_exc_patterns[@]}"; do
        find_cmd="$find_cmd -not -path $path_exc"
    done
    # If running in dummy mode, print out info instead of running clang-format
    if test $dummy -eq 1; then
        echo "Will Run:"
        echo "========="
        echo "$find_cmd | xargs clang-format -style=file -i"
        echo
        echo "Files to be formatted:"
        echo "======================"
        $find_cmd
    # If not running in dummy mode, run clang-format on the collected files
    else
        $find_cmd | xargs clang-format -style=file -i
    fi
done

# Return to the user's original directory
cd $curr_dir
