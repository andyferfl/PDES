#!/bin/bash

echo =======================Setting Enviroment Variables=========================================
. set-enviroment.sh
echo ============================================================================================

chmod +x $SCRIPTS_DIR/*

while true
do

    build_platforms=false
    run_tests=false
    clean_directories=false

    echo "1) Install software and build platforms"
    echo "2) Run tests"
    echo "3) Delete all installed software"
    echo "press q to quit"

    read -p "Please select the option you need: " choice

    case $choice in
    1) build_platforms=true ;;
    2) run_tests=true ;;
    3) clean_directories=true ;;
    q) break ;;
    *) echo "Unrecognized selection: $choice" ;;
    esac

    if $clean_directories ; then
        echo =======================Cleaning Working Directory===========================================
        $SCRIPTS_DIR/clean-workdir.sh
        echo ============================================================================================
    fi

    if $build_platforms ; then
        echo =======================Installing necessary software========================================
        $SCRIPTS_DIR/install-soft.sh
        echo ============================================================================================
        echo ============================Building platforms==============================================
        $SCRIPTS_DIR/build-platforms.sh
        echo ============================================================================================
    fi

    if $run_tests ; then
        echo "1) Schedule distance test"
        echo "2) Event size test"
        echo "3) Phold test"

        read -p "Please select the option you need: " test_choice

        case $test_choice in
        1) $SCRIPTS_DIR/schedule-distance-test-script.sh ;;
        2) echo "event size" ;;
        3) echo "phold" ;;
        *) echo "Unrecognized selection: $choice" ;;
        esac
    fi

done


