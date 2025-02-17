#!/bin/bash

for i in {1..10}
do
    ./window_racer "${i}" >> "test/2/${i}.txt"
    cat window_racer_stats.txt >> "tests/2/${i}.txt"
done