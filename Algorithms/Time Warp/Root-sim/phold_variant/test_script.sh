#!/bin/bash

for i in {1..200}
do
    echo "${i} iteration 16 lp's sequential"
    echo "${i} iteration 16 lp's sequential" >> "lookahead_seq_test.o"
    ./model --sequential --lp 16 >> "lookahead_seq_test.o"
    cat "./outputs/sequential_stats" >> "lookahead_seq_test.o"
done

for ((lp=2; lp<=1024; lp=lp*2))
do
    if ((lp==4 || lp==8 || lp==32 || lp==256)); then
        continue
    fi
    for i in {1..200}
    do
        echo "${i} iteration ${lp} lp's 2 threads pdes"
        echo "${i} iteration ${lp} lp's 2 threads pdes" >> "lookahead_test_2_${lp}.o"
        ./model --wt 2 --lp $lp >> "lookahead_test_2_${lp}.o"
        cat "./outputs/execution_stats" >> "lookahead_test_2_${lp}.o"
    done
done



