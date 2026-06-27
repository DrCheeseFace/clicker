#!/bin/bash

if [ -z "$1" ]; then
    echo "error: no executable provided."
    echo "usage: $0 <executable_name>"
    exit 1
fi

OUTPUT_FILE="valgrind.supp"
RAW_LOG="valgrind_raw.log"

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --gen-suppressions=all \
         --log-file=$RAW_LOG \
         ./$1

sed -n '/{/,/}/p' $RAW_LOG | sed 's/==[0-9]*== //g' > $OUTPUT_FILE
