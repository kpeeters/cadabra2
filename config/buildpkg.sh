#!/bin/sh
cd ${HOME}/cadabra2
sudo ls 
git pull origin
mkdir build
sudo rm -Rf build/*
cd build
cmake ..
make
sudo cpack
exit
