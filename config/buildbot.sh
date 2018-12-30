#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function runbuild {
	 # Start the VM and wait for it to come up.
	 echo "Starting build bot for $1, ssh port $3"
	 ssh buildbothost "nohup VBoxHeadless -s '$1' > /tmp/buildbot.out 2> /tmp/buildbot.err < /dev/null &"
	 echo "Waiting 10 seconds for VM to start up..."
	 sleep 10

	 # Setup the tunnel to the VM; a local port $3 gets mapped
	 # to a port $3 on the build host, which is forwarded by 
	 # virtualbox to the ssh port of the build bot.

	 echo 'Going to start build process...'
	 ssh -M -S my-ctrl-socket -fnNT -L $3:localhost:$3 buildbothost
	 
	 # Execute build commands on the VM, using the build script
	 # on the local machine (as the build bot may not have an
	 # up-to-date build script yet).

	 ssh -tt -p $3 buildbot "bash -s" -- < ${DIR}/buildpkg.sh
	 
	 # Copy the generated package to the web server.

	 echo 'Going to copy the package to the web server'
	 scp -P $3 'buildbot:cadabra2/build/cadabra*' .
	 scp cadabra*$2 "cadabra_web:/var/www/cadabra2/packages/$4"
	 rm -f cadabra*$2
	 
	 # Take down the VM gracefully.

	 echo 'Shutting down build bot...'
	 ssh -tt -p $3 buildbot 'sudo shutdown now'
	 
	 # Close the tunnel to the VM.

	 ssh -S my-ctrl-socket -O exit buildbot
}

# To setup a new build bot, install a basic system, install
# openssh-server, and setup a tunnel from a local port (to be given
# below) to the ssh port on the build bot. Put
#
#   kasper ALL=(ALL) NOPASSWD: ALL
#
# at the bottom of the file edited with 'sudo visudo' (to avoid
# scripts asking for passwords). Copy the ~/.ssh/buildbots_rsa.pub to
# ~/.ssh/authorized_keys on the build VM.
#
# For RPM-based systems, install rpm-build.
#
# Then install 'git', install all cadabra2 dependencies, and clone the
# github repo into ~/cadabra2.

# Parameters: VM name, package type, local ssh port, folder name on web server.

runbuild "Ubuntu_16.04_build" ".deb" 7000 ubuntu1604
# ## runbuild "Fedora_24_build" ".rpm" 7001 fedora24
# runbuild "Fedora_26" ".rpm" 7011 fedora26
runbuild "Fedora_27" ".rpm" 7015 fedora27
runbuild "Fedora_28" ".rpm" 7020 fedora28
runbuild "Mint_18" ".deb" 7002 mint18
runbuild "OpenSUSE_Leap" ".rpm" 7003 opensuse421
runbuild "CentOS_7" ".rpm" 7004 centos7
# ## runbuild "Ubuntu_14.04_build" ".deb" 7005 ubuntu1404
# runbuild "Ubuntu_17.10" ".deb" 7012 ubuntu1710
# ## runbuild "Debian86" ".deb" 7006 debian86
# ## runbuild "Scientific_Linux_74" ".rpm" 7013 scientific74
runbuild "Debian_921" ".deb" 7014 debian9
runbuild "Debian_Buster" ".deb" 7021 debian10
runbuild "Ubuntu_18.04" ".deb" 7017 ubuntu1804
