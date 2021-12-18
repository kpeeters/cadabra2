#!/bin/bash

#sudo rm -Rf ${HOME}/cadabra2
#git clone https://github.com/kpeeters/cadabra2
# if [ -f /etc/redhat-release ]; then
#     echo "Updating .rpm system..."
#     sudo yum update -y
# else
#     echo "Updating .deb system..."
#     sudo apt update
#     sudo apt upgrade -y
# fi

cd ${HOME}/cadabra2
sudo ls
rm -f config/install_script.iss config/pre_install.rtf
git pull origin
git checkout ${CDB_TAG}
sudo rm -Rf build/*
mkdir build
cd build
if [ -f /etc/redhat-release ]; then
	 sudo dnf -y install rpm-build
    centos="`cat /etc/redhat-release | grep CentOS`"
    scilin="`cat /etc/redhat-release | grep Scientific`"
    if [ -n "${centos}" -o -n "${scilin}" ]; then
        source /opt/rh/rh-python36/enable
        source /opt/rh/devtoolset-7/enable
        sudo alternatives --install /usr/bin/python python /usr/bin/python3.6 60
        cmake3 -DPACKAGING_MODE=ON -DENABLE_MATHEMATICA=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
    else
        cmake -DPACKAGING_MODE=ON -DENABLE_MATHEMATICA=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
    fi
else
    cmake -DPACKAGING_MODE=ON -DENABLE_MATHEMATICA=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
fi
make
if [ -n "${centos}" -o -n "${scilin}"  ]; then
    sudo cpack3
else
	 sudo cpack
fi
exit
