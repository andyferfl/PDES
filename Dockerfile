FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV BASE_DIR=/app \
    SCRIPTS_DIR=/app/scripts \
    LOGS_DIR=/app/logs \
    INSTALL_LOGS_DIR=/app/logs/install-logs.txt \
    TEST_LOGS_DIR=/app/logs/test-logs.txt \
    SOFT_DIR=/app/soft \
    INSTALL_DIR=/app/soft \
    PYTHON_INSTALL_DIR=/app/soft/python \
    PYTHON_DIR=/app/soft/python/bin/python3 \
    PIP_DIR=/app/soft/python/bin/pip3 \
    CMAKE_INSTALL_DIR=/app/soft/cmake \
    CMAKE_DIR=/app/soft/cmake/bin/cmake \
    OPENMPI_INSTALL_DIR=/app/soft/openmpi \
    MPICC=/app/soft/openmpi/bin/mpicc \
    MPICCPP_DIR=/app/soft/openmpi/bin/mpic++ \
    MPIEXEC_DIR=/app/soft/openmpi/bin/mpiexec \
    MPIRUN_DIR=/app/soft/openmpi/bin/mpirun \
    PLATFORMS_DIR=/app/platforms \
    ROOTSIM_DIR=/app/platforms/rootsim \
    ROOTSIMCC=/app/platforms/rootsim/bin/rootsim-cc \
    WINDOW_RACER_DIR=/app/platforms/window-racer \
    TESTS_DIR=/app/tests \
    SCHEDULE_DISTANCE_DIR=/app/tests/schedule-distance \
    SCHEDULE_DISTANCE_RESULTS_DIR=/app/tests/schedule-distance/results \
    SCHEDULE_DISTANCE_ROOTSIM_DIR=/app/tests/schedule-distance/rootsim \
    SCHEDULE_DISTANCE_WR_DIR=/app/tests/schedule-distance/windowracer \
    SCHEDULE_DISTANCE_SST_DIR=/app/tests/schedule-distance/nullmessages/phold \
    SCHEDULE_DISTANCE_SST_SRC_DIR=/app/tests/schedule-distance/nullmessages/phold/src \
    SCHEDULE_DISTANCE_SST_LIB_DIR=/app/tests/schedule-distance/nullmessages/phold/src/lib \
    SST_DIR=/app/platforms/sst-core \
    SST_SRC_DIR=/app/platforms/sst-core/sst-core-src \
    SST_BUILD_DIR=/app/platforms/sst-core/build \
    SST_INSTALL_DIR=/app/platforms/sst-core/sst-core-install \
    SST_INC_DIR=/app/platforms/sst-core/sst-core-install/include \
    SST_LIB_DIR=/app/platforms/sst-core/sst-core-install/lib \
    PATH=/app/platforms/sst-core/sst-core-install/bin:$PATH

RUN apt update && apt install -y \
    wget git build-essential automake autoconf libtool \
    libevent-dev hwloc libhwloc-dev libpmix-dev python3 python3-dev \
    valgrind gfortran tar cmake libssl-dev zlib1g-dev libbz2-dev \
    libreadline-dev libsqlite3-dev pkg-config libarchive-dev libexpat1-dev

WORKDIR /app

RUN mkdir -p logs soft platforms tests

COPY soft/*.tar.gz ./soft/
COPY soft/*.tgz ./soft/
COPY platforms/*.tar.gz ./platforms/
COPY scripts/ ./scripts/
COPY tests/ ./tests/

RUN chmod +x scripts/*.sh

RUN ./scripts/install-soft.sh
RUN mkdir -p /opt && ln -s /app/platforms/sst-core /opt/sst-core
RUN ./scripts/build-platforms.sh
# RUN ./scripts/set-environment.sh
