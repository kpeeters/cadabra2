#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function runbuild {
	 # Start the VM and wait for it to come up.
	 echo "Starting build bot for $1, ssh port $3"
	 ssh buildbothost "nohup VBoxHeadless -s '$1' > /tmp/buildbot.out 2> /tmp/buildbot.err < /dev/null &"
	 sleep 5

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
	 scp cadabra*.$2 cadabra_web:/var/www/cadabra2/packages/
	 
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
# scripts asking for passwords).
#
# Then install 'git', install all cadabra2 dependencies, and clone the
# github repo into ~/cadabra2.

#runbuild "Ubuntu_16.04_build" ".deb" 7000
#runbuild "Fedora_24_build" ".rpm" 7001
runbuild "Mint_18" ".deb" 7002
#runbuild "OpenSUSE_Leap" ".rpm" 7003
#runbuild "CentOS_7" ".rpm" 7004
