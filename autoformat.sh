#!/bin/bash

code_dirs=( src )

if [[ $# -gt 0 ]]; then
    cla=( "$@" )
    code_dirs+=( ${cla[@]} )
fi

for d in ${code_dirs[@]}; do
    find $d -name "*.hpp" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" | xargs clang-format -style=file -i
done
