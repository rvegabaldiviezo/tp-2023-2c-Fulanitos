#!/usr/bin/sudo bash

declare -a arr=("memoria" "fileSystem" "cpu" "kernel")

## iteramos el array para compilar cada modulo de la lista
for i in "${arr[@]}"
do
    bash build.sh $i
    if [ $? -ne 0 ]; then
        exit 1
    fi
done