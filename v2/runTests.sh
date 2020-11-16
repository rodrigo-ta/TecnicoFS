#! /bin/bash

files=($(ls -p $1| grep -v /))

mkdir -p "${2}"

for fl in "${files[@]}"
do
    for ((thrds = 1; thrds <= $3; thrds++))
    do
    echo InputFile="$fl" NumThreads=$thrds
    ./tecnicofs "$1/$fl" "$2/${fl%.*}-$thrds.txt" $thrds | grep 'TecnicoFS completed in'
    done
done