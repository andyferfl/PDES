#!/bin/bash

CONFIG_FILE=${1:-config/schedule-distance.conf}

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Config file $CONFIG_FILE not found!"
    exit 1
fi

source "$CONFIG_FILE"

rm -rf $SCHEDULE_DISTANCE_RESULTS_DIR/*

echo "=======================Rootsim========================================"
cd $SCHEDULE_DISTANCE_ROOTSIM_DIR
C_FILE="application.h"

sed -i "s/const simtime_t phold_gvt = [0-9.]\+f;/const simtime_t phold_gvt = ${gvt}f;/" "$C_FILE"
if [ $? -ne 0 ]; then
    echo "Error modifying GVT $gvt"
fi

for lookahead_val in $lookahead; do
    sed -i "s/const double phold_lookahead = [0-9.]\+;/const double phold_lookahead = $lookahead_val;/" "$C_FILE"
    if [ $? -ne 0 ]; then
        echo "Error modifying file for value $lookahead_val"
        continue
    fi

    $ROOTSIMCC *.c -o model >> logs.o
    if [ $? -eq 0 ]; then
        echo "Successfully compiled for value: $lookahead_val"
    else
        echo "Compilation failed for value $lookahead_val"
        continue
    fi

    for lp_val in $lp; do
        for ((i=1; i<=iterations; i++)); do
            output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead_val}_seq_${lp_val}_lp.o"
            echo -en "\r\033[K${lp_val} lp's sequential (${i} of ${iterations})"
            echo "${i} iteration ${lp_val} lp's sequential" >> $output_file
            ./model --sequential --lp $lp_val >> $output_file
            cat "./outputs/sequential_stats" >> $output_file
        done
        echo

        for threads_val in $threads; do
            if [ $threads_val -gt $lp_val ]; then continue; fi
            for ((i=1; i<=iterations; i++)); do
                output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead_val}_timewarp_${threads_val}_wt_${lp_val}_lp.o"
                echo -en "\r\033[K${lp_val} lp's ${threads_val} threads timewarp (${i} of ${iterations})"
                echo "${i} iteration ${lp_val} lp's ${threads_val} threads timewarp" >> $output_file
                ./model --wt $threads_val --lp $lp_val >> $output_file
                cat "./outputs/execution_stats" >> $output_file
            done
            echo
        done
    done
done

echo "=======================Windowracer===================================="
cd $SCHEDULE_DISTANCE_WR_DIR
C_FILE="window_racer_config.hpp"

for lookahead_val in $lookahead; do
    sed -i "s/const double phold_lookahead = [0-9.]\+;/const double phold_lookahead = $lookahead_val;/" "$C_FILE"

    for lp_val in $lp; do
        sed -i "s/const uint64_t num_entities = [0-9.]\+;/const uint64_t num_entities = $lp_val;/" "$C_FILE"

        for threads_val in $threads; do
            if [ $threads_val -gt $lp_val ]; then continue; fi
            sed -i "s/const uint64_t num_threads = [0-9.]\+;/const uint64_t num_threads = ${threads_val};/" "$C_FILE"

            cmake . -DCMAKE_BUILD_TYPE=Release >> logs.o
            make >> logs.o

            if [ $? -eq 0 ]; then
                echo "Successfully compiled for lookahead $lookahead_val, threads $threads_val, LP $lp_val"
            else
                echo "Compilation failed for lookahead $lookahead_val, threads $threads_val, LP $lp_val"
                continue
            fi

            for ((i=1; i<=iterations; i++)); do
                output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead_val}_windowracer_${threads_val}_wt_${lp_val}_lp.o"
                echo -en "\r\033[K${lp_val} lp's ${threads_val} threads windowracer (${i} of ${iterations})"
                echo "${i} iteration ${lp_val} lp's ${threads_val} threads windowracer" >> $output_file
                ./window_racer $gvt >> $output_file
                cat window_racer_stats.txt >> $output_file
            done
            echo
        done
    done
done

echo "=======================SST============================================"
cd $SCHEDULE_DISTANCE_SST_DIR/tests

for lookahead_val in $lookahead; do
    for lp_val in $lp; do
        for threads_val in $threads; do
            if [ $threads_val -gt $lp_val ]; then continue; fi
            for ((i=1; i<=iterations; i++)); do
                output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead_val}_sst_${threads_val}_wt_${lp_val}_lp.o"
                echo -en "\r\033[K${lp_val} lp's ${threads_val} threads SST (${i} of ${iterations})"
                echo "${i} iteration ${lp_val} lp's ${threads_val} threads SST" >> $output_file
                sst --lib-path=$SCHEDULE_DISTANCE_SST_LIB_DIR -n $threads_val phold.py --stop-at=${gvt}s -- --n=$lp_val --minimum=$lookahead_val >> $output_file
            done
            echo
        done
    done
done

echo "=======================Processing results============================="
cd $SCHEDULE_DISTANCE_DIR
$PIP_DIR install -r requirements.txt >> logs.o
$PYTHON_DIR process_results.py >> logs.o
echo "====================================================================="
