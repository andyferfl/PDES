# PDES

This is a project set up in order to be able to test PDES against DES.
You can find some relevant scientific papers about PDES in the papers folder.
The platform with the implementation of the three main algorithms can be found in the fusion folder and in order to run it you can use the build-library script in that same folder.

The platforms used for initial tests are presented in a compressed way in the platforms folder.
In the soft folder you can find cmake, openmpi and python in a compressed way. \
All the software in the platforms and soft folders can be 
installed when running the startup script and then you can use it to make your own tests, or run the existing tests from the script.

## Requirements
- make
- gcc
- llvm
- clang
- autoconf
- libtool

## Usage
In order to start the testing enviroment just run the startup.sh. You can find it in the scripts folder 
