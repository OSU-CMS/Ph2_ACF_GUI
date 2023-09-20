#!/bin/bash
SOCK=/tmp/.X11-unix; XAUTH=/tmp/.docker.xauth; xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -; chmod 777 $XAUTH;


## Change the ports in 'mydevicelist' to those which you wish to use with the GUI#############
mydevices=""
mydevicelist=("/dev/ttyUSBLV" "/dev/ttyUSBHV" "/dev/ttyUSBPeltier")

for mydevice in "${mydevicelist[@]}"; do
    mydevices+="--device=$mydevice "
    mydevices+=" "
    echo "device loaded $mydevices"
done
mydevices=$(echo $mydevices | xargs)
echo $mydevices | xargs
################################################################################################

#docker run --rm -ti -v $PWD:$PWD -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data $devices -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  local/testimage
#docker run --rm -ti -v $PWD:$PWD -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data --device=/dev/ttyUSBHV --device=/dev/ttyUSBLV --device=/dev/ttyUSBPeltier --device=/dev/ttyACM0 -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  local/testimage
#docker run --rm -ti $mydevices -v $PWD:$PWD -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/Gui/siteConfig.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/siteSettings.py:ro -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  majoyce2/ph2_acf_gui:latest
docker run --rm -ti $mydevices -v $PWD:$PWD -v ${PWD}/icicle/icicle:/home/cmsTrUser/Ph2_ACF_GUI/icicle/icicle:ro -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test -v ${PWD}/Gui/siteConfig.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/siteSettings.py:ro -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH   -e XAUTHORITY=$XAUTH --net host  local/testimage
