#!/bin/sh

# start the VM and wait for it to come up
echo 'Starting build bot...'
ssh buildbot "nohup VBoxHeadless -s 'Ubuntu_16.04_build' > /tmp/Ubuntu_16.04.out 2> /tmp/Ubuntu_16.04.err < /dev/null &"
sleep 5

# setup the tunnel to the VM
echo 'Going to start build process...'
ssh -M -S my-ctrl-socket -fnNT -L 7000:localhost:7000 buildbot

# execute build commands on the VM
ssh -tt -p 7000 localhost cadabra2/config/buildpkg

# take down the VM gracefully
echo 'Shutting down build bot...'
ssh -tt -p 7000 localhost 'sudo shutdown now'

# close the tunnel to the VM
ssh -S my-ctrl-socket -O exit buildbot

