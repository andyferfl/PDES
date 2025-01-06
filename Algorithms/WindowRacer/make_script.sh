#!/bin/bash

cmake . -DCMAKE_BUILD_TYPE=Release

make

mkdir "./tests/for"

for i in {1..200}
do
    echo "${i}"
    echo "${i}" >> "test.o"
    ./window_racer 10000 >>"test.o"
    cat window_racer_stats.txt >> "test.o"
done