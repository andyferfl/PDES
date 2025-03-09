#!/bin/bash

echo "Downloading python..."
wget --quiet --no-clobber --output-file=$INSTALL_LOGS_DIR \
    --directory-prefix=$SOFT_DIR --output-document=$SOFT_DIR/Python-3.12.8.tgz \
    https://www.python.org/ftp/python/3.12.8/Python-3.12.8.tgz

tar -xvzf $SOFT_DIR/Python-3.12.8.tgz --directory=$SOFT_DIR >> $INSTALL_LOGS_DIR

echo "Configurating python..."
cd $SOFT_DIR/Python-3.12.8
CFLAGS="-Wno-array-bounds -Wno-stringop-truncation" ./configure --with-strict-overflow --quiet --prefix=$PYTHON_INSTALL_DIR >> $INSTALL_LOGS_DIR

echo "Getting ready python..."
make --jobs=$(nproc) >> $INSTALL_LOGS_DIR
make --jobs=$(nproc) test >> $INSTALL_LOGS_DIR

echo "Installing python..."
make install >> $INSTALL_LOGS_DIR
$PYTHON_DIR --version
$PYTHON_DIR -m pip install --upgrade pip >> $INSTALL_LOGS_DIR

echo "Downloading cmake..."
wget --quiet --no-clobber --output-file=$INSTALL_LOGS_DIR \
    --directory-prefix=$SOFT_DIR --output-document=$SOFT_DIR/cmake-3.31.5.tar.gz \
    https://github.com/Kitware/CMake/releases/download/v3.31.5/cmake-3.31.5.tar.gz

tar -xvzf $SOFT_DIR/cmake-3.31.5.tar.gz --directory=$SOFT_DIR >> $INSTALL_LOGS_DIR
cd $SOFT_DIR/cmake-3.31.5

echo "Configurating cmake..."
./bootstrap --parallel=$(nproc) --prefix=$CMAKE_INSTALL_DIR >> $INSTALL_LOGS_DIR
make --jobs=$(nproc) >> $INSTALL_LOGS_DIR

echo "Installing cmake..."
make install >> $INSTALL_LOGS_DIR

echo "Downloading openmpi..."
wget --quiet --no-clobber --output-file=$INSTALL_LOGS_DIR \
    --directory-prefix=$SOFT_DIR --output-document=$SOFT_DIR/openmpi-5.0.7.tar.gz \
    https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.7.tar.gz

tar -xvzf $SOFT_DIR/openmpi-5.0.7.tar.gz --directory=$SOFT_DIR >> $INSTALL_LOGS_DIR
cd $SOFT_DIR/openmpi-5.0.7

echo "Configurating openmpi..."
CFLAGS="-Wno-stringop-overflow -Wno-format-truncation -Wno-unused-result -Wno-alloc-size-larger-than  -Wno-array-parameter -Wno-dangling-pointer" \
    ./configure --prefix=$OPENMPI_INSTALL_DIR --quiet --enable-silent-rules >> $INSTALL_LOGS_DIR
make --jobs=$(nproc) >> $INSTALL_LOGS_DIR

echo "Installing openmpi..."
make install >> $INSTALL_LOGS_DIR

#echo "Decompressing libtimestretch..."
#tar -xvzf $SOFT_DIR/libtimestretch-master.tar.gz --directory=$SOFT_DIR >> $INSTALL_LOGS_DIR
#cd $SOFT_DIR/libtimestretch-master

#chmod +x autogen.sh
#./autogen.sh
#./configure --prefix=$TIMESTRETCH_INSTALL_DIR



