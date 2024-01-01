#!/bin/bash

# usage: bash init.sh

if [ "$(whoami)" != "root" ]; then
    sudo bash $0
    exit
fi

function getReleasePath () {
    cd $(dirname $0)/.. && echo "$(pwd)/release"
}

function setPathEnv () {
    local path
    for path in $@; do
        [[ "$PATH" =~ "$path" ]] || PATH=$PATH:$path
    done
}

RELEASE_PATH=$(getReleasePath)
if [[ "$PATH" =~ "$RELEASE_PATH" ]]; then
    echo "Error: *** PATH is already set."
    echo "PATH=$PATH"
    exit 1
fi

setPathEnv $RELEASE_PATH /usr/local/sbin

export PATH
bash
