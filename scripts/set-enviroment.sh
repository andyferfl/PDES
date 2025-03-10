#!/bin/bash

cd `dirname $0`
cd ..
export BASE_DIR=`pwd`
export SCRIPTS_DIR=$BASE_DIR/scripts

export LOGS_DIR=$BASE_DIR/logs
export INSTALL_LOGS_DIR=$LOGS_DIR/install-logs.txt
export TEST_LOGS_DIR=$LOGS_DIR/test-logs.txt

export SOFT_DIR=$BASE_DIR/soft
export INSTALL_DIR=$SOFT_DIR
#export OPENSSL_DIR=/etc/ssl
export PYTHON_INSTALL_DIR=$INSTALL_DIR/python
export PYTHON_DIR=$PYTHON_INSTALL_DIR/bin/python3
export PIP_DIR=$PYTHON_INSTALL_DIR/bin/pip3
export CMAKE_INSTALL_DIR=$INSTALL_DIR/cmake
export CMAKE_DIR=$CMAKE_INSTALL_DIR/bin/cmake
export OPENMPI_INSTALL_DIR=$INSTALL_DIR/openmpi
export MPICC=$OPENMPI_INSTALL_DIR/bin/mpicc
export MPICCPP_DIR=$OPENMPI_INSTALL_DIR/bin/mpic++
export MPIEXEC_DIR=$OPENMPI_INSTALL_DIR/bin/mpiexec
export MPIRUN_DIR=$OPENMPI_INSTALL_DIR/bin/mpirun
#export TIMESTRETCH_INSTALL_DIR=$INSTALL_DIR/libtimestretch

export PLATFORMS_DIR=$BASE_DIR/platforms
export ROOTSIM_DIR=$PLATFORMS_DIR/rootsim
export ROOTSIMCC=$ROOTSIM_DIR/bin/rootsim-cc
export WINDOW_RACER_DIR=$PLATFORMS_DIR/window-racer

export TESTS_DIR=$BASE_DIR/tests
export SCHEDULE_DISTANCE_DIR=$TESTS_DIR/schedule-distance
export SCHEDULE_DISTANCE_RESULTS_DIR=$SCHEDULE_DISTANCE_DIR/results
export SCHEDULE_DISTANCE_ROOTSIM_DIR=$SCHEDULE_DISTANCE_DIR/rootsim
export SCHEDULE_DISTANCE_WR_DIR=$SCHEDULE_DISTANCE_DIR/windowracer


