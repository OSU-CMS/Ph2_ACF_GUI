#!/bin/bash
SOCK=/tmp/.X11-unix; XAUTH=/tmp/.docker.xauth; xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -; chmod 777 $XAUTH;


docker run --rm -ti -v $PWD:$PWD -w $PWD -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host $devices ph2acf_gui
