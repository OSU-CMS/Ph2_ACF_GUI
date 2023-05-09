#!/bin/bash
SOCK=/tmp/.X11-unix; XAUTH=/tmp/.docker.xauth; xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -; chmod 777 $XAUTH;

output=$(python3 -m serial.tools.list_ports -q)
IFS=$'\n' ports=($output)
devices=""
for port in "${ports[@]}"; do
    if [ ! -z "$port" ]; then
        devices+="--device=$port "
    fi
done
devices=$(echo $devices | xargs)

docker run --rm -ti -v $PWD:$PWD --device=/dev/ttyUSBLV --device=/dev/ttyUSBHV -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  local/testimage
