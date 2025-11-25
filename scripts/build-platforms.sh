#!/bin/bash

cd "$(dirname "$0")/.."   
. ./scripts/set-environment.sh   

echo "Decompressing rootsim..."
tar -xvzf $PLATFORMS_DIR/rootsim.tar.gz --directory=$PLATFORMS_DIR >> $INSTALL_LOGS_DIR
cd $ROOTSIM_DIR

echo "Installing rootsim..."
chmod +x autogen.sh
./autogen.sh >> $INSTALL_LOGS_DIR
./configure --prefix=$ROOTSIM_DIR --enable-mpi --enable-debug --quiet >> $INSTALL_LOGS_DIR

make >> $INSTALL_LOGS_DIR
make install >> $INSTALL_LOGS_DIR

echo "Decompressing window racer..."
tar -xvzf $PLATFORMS_DIR/window-racer.tar.gz --directory=$PLATFORMS_DIR >> $INSTALL_LOGS_DIR
cd $WINDOW_RACER_DIR

echo "Installing window racer..."
cmake . -DCMAKE_BUILD_TYPE=Release
make

# Устанавливаем SST
echo "Decompressing SST..."
tar -xvzf $PLATFORMS_DIR/sst-core.tar.gz --directory=$PLATFORMS_DIR >> $INSTALL_LOGS_DIR
cd $SST_DIR

echo "Building SST..."
mkdir -p $SST_BUILD_DIR
cd $SST_BUILD_DIR
# cd $SST_SRC_DIR
# make distclean >> $INSTALL_LOGS_DIR 2>/dev/null
$SST_SRC_DIR/configure --prefix=$SST_INSTALL_DIR >> $INSTALL_LOGS_DIR 2>&1
make all >> $INSTALL_LOGS_DIR 2>&1
make install >> $INSTALL_LOGS_DIR 2>&1

mkdir -p $SST_INC_DIR/sst/core
cp $SST_SRC_DIR/src/sst/core/simulation_impl.h $SST_INC_DIR/sst/core/

mkdir -p $SST_INSTALL_DIR/etc/sst
cp $SST_SRC_DIR/src/sst/sstsimulator.conf $SST_INSTALL_DIR/etc/sst/

cd $SST_SRC_DIR
make install-headers >> $INSTALL_LOGS_DIR 2>&1 || true


echo "SST PHOLD..."
cd $SCHEDULE_DISTANCE_SST_SRC_DIR

mkdir -p $SST_INSTALL_DIR/etc/sst
cp $SST_SRC_DIR/src/sst/sstsimulator.conf $SST_INSTALL_DIR/etc/sst/
export SST_CONFIG_FILE=$SST_INSTALL_DIR/etc/sst/sstsimulator.conf

# echo "Using SST include dir: $SST_INC_DIR"
CXXFLAGS="-I$SST_INC_DIR $CXXFLAGS"
# echo "CXXFLAGS = $CXXFLAGS"

make clean >> $INSTALL_LOGS_DIR 2>&1 || true
make CXXFLAGS="$CXXFLAGS" >> $INSTALL_LOGS_DIR 2>&1

make install >> $INSTALL_LOGS_DIR 2>&1
