#!/bin/sh
set -eu

BASE_DIR=pico-light-switch_source_distribution

BASE_DIR=`realpath "$BASE_DIR"`

mkdir -p "$BASE_DIR"

if [ -d "$BASE_DIR/pico-light-switch" ]
then
    echo "Updating pico-light-switch"
    cd "$BASE_DIR/pico-light-switch"
    git --git-dir=.git pull
else
    echo "Cloning pico-light-switch"
    git clone . "$BASE_DIR/pico-light-switch"
fi

if [ -d "$BASE_DIR/pico-sdk" ]
then
    echo "Updating pico-sdk"
    cd "$BASE_DIR/pico-sdk"
    git --git-dir=.git pull --recurse-submodules
else
    echo "Cloning pico-sdk"
    git clone --recurse-submodules "https://github.com/raspberrypi/pico-sdk.git" "$BASE_DIR/pico-sdk"
fi
