#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function runbuild {
	 # start the VM and wait for it to come up
	 echo "Starting build bot for $1, ssh port $3"
	 ssh buildbothost "nohup VBoxHeadless -s '$1' > /tmp/buildbot.out 2> /tmp/buildbot.err < /dev/null &"
	 sleep 5

	 # setup the tunnel to the VM
	 echo 'Going to start build process...'
	 ssh -M -S my-ctrl-socket -fnNT -L $3:localhost:$3 buildbothost
	 
	 # execute build commands on the VM
	 #ssh -tt buildbot cadabra2/config/buildpkg
	 ssh -tt -p $3 buildbot "bash -s" -- < ${DIR}/buildpkg.sh
	 
	 # copy the generated package to the web server
	 echo 'Going to copy the package to the web server'
	 scp -P $3 'buildbot:cadabra2/build/cadabra*' .
	 scp cadabra*.$2 cadabra_web:/var/www/cadabra2/packages/
	 
	 # take down the VM gracefully
	 echo 'Shutting down build bot...'
	 ssh -tt -p $3 buildbot 'sudo shutdown now'
	 
	 # close the tunnel to the VM
	 ssh -S my-ctrl-socket -O exit buildbot
}


#runbuild "Ubuntu_16.04_build" ".deb" 7000
runbuild "Fedora_24_build" ".rpm" 7001
