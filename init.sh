#!/bin/bash

set -Eeuo pipefail

if [ ! -d "./uWebSockets/" ]; then
  git clone --recurse-submodules git@github.com:uNetworking/uWebSockets.git
  chmod 777 ./uWebSockets/uSockets
fi

mkdir -p /usr/local/include/uWebSockets/
mkdir -p /usr/local/include/uSockets/

cp -r ./uWebSockets/src/* /usr/local/include/uWebSockets/ &
cp -r ./uWebSockets/uSockets/src/* /usr/local/include/uSockets/

if [ ! -d "/usr/local/include/glaze/" ]; then
  mkdir -p /usr/local/include/glaze/
  git clone git@github.com:stephenberry/glaze.git
  cp -r ./glaze/include/* /usr/local/include/glaze/glaze/
  rm -rf ./glaze/
fi
