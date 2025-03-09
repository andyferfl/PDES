#!/bin/bash

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


