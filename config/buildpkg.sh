#!/bin/bash
#sudo rm -Rf ${HOME}/cadabra2
#git clone https://github.com/kpeeters/cadabra2
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
		 source /opt/rh/rh-python36/enable
		 source /opt/rh/devtoolset-7/enable
      cmake3 -DPACKAGING_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..
   else
      cmake -DPACKAGING_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..
   fi
else
   cmake -DPACKAGING_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..
fi
make
if [ -n "${centos}" ]; then
    sudo cpack3
else
	 sudo cpack
fi
exit
