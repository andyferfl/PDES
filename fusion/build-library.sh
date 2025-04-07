#!/bin/bash
rm -rf build
rm -rf install
mkdir build
cd build

cmake .. -DCMAKE_INSTALL_PREFIX=../install
cmake --build . --config Release
cmake --install .

cd ../examples/phold/
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
./phold_simulation