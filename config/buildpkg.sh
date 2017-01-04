#!/bin/bash
sudo rm -Rf ${HOME}/cadabra2
git clone https://github.com/kpeeters/cadabra2
cd ${HOME}/cadabra2
sudo ls 
#git pull origin
sudo rm -Rf build/*
mkdir build
cd build
if [ -f /etc/redhat-release ]; then 
   centos="`cat /etc/redhat-release | grep CentOS`"
   echo ${centos}
   if [ -n "${centos}" ]; then
      cmake .. -DUSE_PYTHON_3=OFF -DCMAKE_INSTALL_PREFIX=/usr
   else
      cmake .. -DCMAKE_INSTALL_PREFIX=/usr
   fi
else
   cmake ..
fi
make
sudo cpack
exit
