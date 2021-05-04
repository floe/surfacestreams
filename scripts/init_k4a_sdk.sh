#!/usr/bin/env bash

# initialize_azure_kinect_sensor_sdk.sh:
#   Initializes the Kinect's SDK project.
#
# author: Everett
# created: 2021-04-15 18:26
# Github: https://github.com/antiqueeverett/

PROJECT_DIR=$(dirname $(dirname $(readlink -f "$0")))
echo "-- initializing the Kinect SDK Project"

# -- SDK build directory
K4A_SDK="$PROJECT_DIR/external/Azure-Kinect-Sensor-SDK/build"

command git pull --recurse-submodules -j 12
command git submodule update --init --recursive -j 12


cd "$PROJECT_DIR" || return
./scripts/build_k4a_sdk.sh -j 12
