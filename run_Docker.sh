#!/bin/bash
SOCK=/tmp/.X11-unix; XAUTH=/tmp/.docker.xauth; xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -; chmod 777 $XAUTH;

output=$(python3 -m serial.tools.list_ports -q)
IFS=$'\n' ports=($output)
devices=""
devicelist=""
for port in "${ports[@]}"; do
    if [ ! -z "$port" ]; then
        devices+="--device=$port "
    fi
    
done
devices=$(echo $devices | xargs)



#docker run --rm -ti -v $PWD:$PWD -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data $devices -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  local/testimage
#docker run --rm -ti -v $PWD:$PWD -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data --device=/dev/ttyUSBHV --device=/dev/ttyUSBLV --device=/dev/ttyUSBPeltier --device=/dev/ttyACM0 -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  local/testimage
docker run --rm -ti -v $PWD:$PWD -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data --device=/dev/ttyUSBHV --device=/dev/ttyUSBLV --device=/dev/ttyUSBPeltier --device=/dev/ttyACM0 -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  majoyce2/ph2_acf_gui:version4.13