#!/bin/bash

rm -rf $SCHEDULE_DISTANCE_RESULTS_DIR/*

read -p "Please set the number of iterations you want for each configurations             : " ITERATIONS
read -p "Please introduce the approximated GVT when the simulation should end (eg. 10.0)  : " GVT
read -p "Please introduce the values for lookahead separated by spaces                    : " -a LOOKAHEAD_VALUES
read -p "Please introduce an array with the lp's numbers separated by spaces              : " -a LPS
read -p "Please introduce an array with the threads numbers separated by spaces. Max $(nproc)      : " -a THREADS_LIST

echo =======================Rootsim========================================
cd $SCHEDULE_DISTANCE_ROOTSIM_DIR
C_FILE="application.h"            
#ITERATIONS=100
#LOOKAHEAD_VALUES=(1)
#LPS=(1024 1200 1400 1600 1800 2000)
#THREADS_LIST=(3 4 5)

sed -i "s/const simtime_t phold_gvt = [0-9.]\+f;/const simtime_t phold_gvt = ${GVT}f;/" "$C_FILE"

if [ $? -ne 0 ]; then
    echo "Error modifying GVT $GVT"
fi

for lookahead in "${LOOKAHEAD_VALUES[@]}"; do
    
    sed -i "s/const double phold_lookahead = [0-9.]\+;/const double phold_lookahead = $lookahead;/" "$C_FILE"
    
    if [ $? -ne 0 ]; then
        echo "Error modifying file for value $lookahead"
        continue
    fi
    
    $ROOTSIMCC *.c -o model >> logs.o
    
    if [ $? -eq 0 ]; then
        echo "Successfully compiled for value: $lookahead"
    else
        echo "Compilation failed for value $lookahead"
        continue
    fi
    
    for lp in "${LPS[@]}";
    do

        for i in $(seq 1 "$ITERATIONS")
        do
            output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead}_seq_${lp}_lp.o"
            echo -en "\r\033[K${lp} lp's sequential (${i} of ${ITERATIONS})"
            echo "${i} iteration ${lp} lp's sequential" >> $output_file
            ./model --sequential --lp $lp >> $output_file
            cat "./outputs/sequential_stats" >> $output_file
        done

        echo

        for threads in "${THREADS_LIST[@]}";
        do
            if [ $threads -gt $lp ]; then
                continue
            fi
            for i in $(seq 1 "$ITERATIONS")
            do
                output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead}_timewarp_${threads}_wt_${lp}_lp.o"
                echo -en "\r\033[K${lp} lp's ${threads} threads timewarp (${i} of ${ITERATIONS})"
                echo "${i} iteration ${lp} lp's ${threads} threads pdes" >> $output_file
                ./model --wt $threads --lp $lp >> $output_file
                cat "./outputs/execution_stats" >> $output_file
            done

            echo
        done
     
    done

done
echo ======================================================================
echo =======================Windowracer====================================
cd $SCHEDULE_DISTANCE_WR_DIR
C_FILE="window_racer_config.hpp" 

for lookahead in "${LOOKAHEAD_VALUES[@]}"; do
    
    sed -i "s/const double phold_lookahead = [0-9.]\+;/const double phold_lookahead = $lookahead;/" "$C_FILE"
    
    if [ $? -ne 0 ]; then
        echo "Error modifying file for lookahead $lookahead"
        continue
    fi
    
    for lp in "${LPS[@]}";
    do

        sed -i "s/const uint64_t num_entities = [0-9.]\+;/const uint64_t num_entities = $lp;/" "$C_FILE"
        if [ $? -ne 0 ]; then
            echo "Error modifying file for $lp LP"
            continue
        fi

        for threads in "${THREADS_LIST[@]}";
        do
            if [ $threads -gt $lp ]; then
                continue
            fi
            sed -i "s/const uint64_t num_threads = [0-9.]\+;/const uint64_t num_threads = ${threads};/" "$C_FILE"
            if [ $? -ne 0 ]; then
                echo "Error modifying file for $threads wt"
                continue
            fi
            
            cmake . -DCMAKE_BUILD_TYPE=Release >> logs.o
            make >> logs.o
            
            if [ $? -eq 0 ]; then
                echo "Successfully compiled for $lookahead lookahead $threads wt $lp lp's"
            else
                echo "Compilation failed for $lookahead lookahead $threads wt $lp lp's"
                continue
            fi

            for i in $(seq 1 "$ITERATIONS")
            do
                output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead}_windowracer_${threads}_wt_${lp}_lp.o"
                echo -en "\r\033[K${lp} lp's ${threads} threads windowracer (${i} of ${ITERATIONS})"
                echo "${i} iteration ${lp} lp's ${threads} threads windowracer" >> $output_file
                ./window_racer $GVT >> $output_file
                cat window_racer_stats.txt >> $output_file
            done

            echo
        done
     
    done

done
echo ======================================================================
echo =======================SST============================================
cd $SCHEDULE_DISTANCE_SST_DIR/tests

for lookahead in "${LOOKAHEAD_VALUES[@]}"; do
    
    for lp in "${LPS[@]}"; do

        for threads in "${THREADS_LIST[@]}"; do
            if [ $threads -gt $lp ]; then
                continue
            fi

            for i in $(seq 1 "$ITERATIONS"); do
                output_file="${SCHEDULE_DISTANCE_RESULTS_DIR}/lookahead_${lookahead}_sst_${threads}_wt_${lp}_lp.o"
                echo -en "\r\033[K${lp} lp's ${threads} threads SST (${i} of ${ITERATIONS})"
                echo "${i} iteration ${lp} lp's ${threads} threads SST" >> $output_file
                sst --lib-path=$SCHEDULE_DISTANCE_SST_LIB_DIR -n $threads phold.py --stop-at=${GVT}s -- --n=$lp --minimum=$lookahead>> $output_file
            done

            echo
        done

    done

done
echo ======================================================================
echo =======================Processing results=============================
cd $SCHEDULE_DISTANCE_DIR
$PIP_DIR install -r requirements.txt >> logs.o
$PYTHON_DIR process_results.py >> logs.o
echo ======================================================================


