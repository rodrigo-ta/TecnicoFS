#! /bin/bash

files=($(ls -p $1| grep -v /))

mkdir -p "${2}"

for fl in "${files[@]}"
do
    for ((thrds = 1; thrds <= $3; thrds++))
    do
    echo InputFile="$fl" NumThreads=$thrds
    #./tecnicofs "$1/$fl" "$fl-$thrds.txt" $thrds | tee "$2/$fl-$thrds.txt" | grep 'TecnicoFS completed in'
    ./tecnicofs "$1/$fl" "$fl-$thrds.txt" $thrds | grep 'TecnicoFS completed in'
    mv "$fl-$thrds.txt" "$2"
    done
done