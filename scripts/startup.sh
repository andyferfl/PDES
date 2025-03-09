#!/bin/bash

install_software=false
clean_directories=false
build_platforms=false
run_tests=false

echo "1) Intall software needed"
echo "2) Clean directories"
echo "3) Build Platforms"
echo "4) Install software and build platforms"
echo "5) Run tests"
echo "press q to quit"

read -p "Please select the option you need: " choice

case $choice in
  1) install_software=true ;;
  2) clean_directories=true ;;
  3) build_platforms=true ;;
  4) clean_directories=true install_software=true build_platforms=true ;;
  5) run_tests=true ;;
  q) break ;;
  *) echo "Unrecognized selection: $choice" ;;
esac

echo =======================Setting Enviroment Variables=========================================
. set-enviroment.sh
echo ============================================================================================

if $clean_directories ; then
    echo =======================Cleaning Working Directory===========================================
    $SCRIPTS_DIR/clean-workdir.sh
    echo ============================================================================================
fi

if $install_software ; then
    echo =======================Installing necessary software========================================
    $SCRIPTS_DIR/install-soft.sh
    echo ============================================================================================
fi

if $build_platforms ; then
    echo ============================Building platforms==============================================
    $SCRIPTS_DIR/build-platforms.sh
    echo ============================================================================================
fi




