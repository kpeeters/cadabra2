#!/bin/bash
cd ${HOME}/cadabra2
sudo ls 
git pull origin
sudo rm -Rf build/*
mkdir build
cd build
if [ -f /etc/redhat-release ]; then 
   centos="`cat /etc/redhat-release | grep CentOS`"
   echo ${centos}
   if [ -n "${centos}" ]; then
      cmake .. -DUSE_PYTHON_3=OFF
   else
      cmake ..
   fi
else
   cmake ..
fi
make
sudo cpack
exit
