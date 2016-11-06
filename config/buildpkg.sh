#!/bin/sh
cd ${HOME}/cadabra2
sudo ls 
git pull origin
sudo rm -Rf build/*
mkdir build
cd build
cmake ..
make
sudo cpack
exit
