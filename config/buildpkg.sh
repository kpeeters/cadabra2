#!/bin/bash
set -e

#sudo rm -Rf ${HOME}/cadabra2
#git clone https://github.com/kpeeters/cadabra2
cd ${HOME}/cadabra2
sudo ls 
git pull origin
sudo rm -Rf build/*
mkdir -p build
cd build
if [ -f /etc/redhat-release ]; then
    centos="`cat /etc/redhat-release | grep CentOS`"
    scilin="`cat /etc/redhat-release | grep Scientific`"
    if [ -n "${centos}" -o -n "${scilin}" ]; then
        source /opt/rh/rh-python36/enable
        source /opt/rh/devtoolset-7/enable
        sudo alternatives --install /usr/bin/python python /usr/bin/python3.6 60
        cmake3 -DPACKAGING_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..
    else
        cmake -DPACKAGING_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..
    fi
else
    cmake -DPACKAGING_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..
fi
make
if [ -n "${centos}" -o -n "${scilin}"  ]; then
    sudo cpack3
else
	 sudo cpack
fi
exit
