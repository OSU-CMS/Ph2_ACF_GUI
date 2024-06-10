#!/bin/bash
SOCK=/tmp/.X11-unix; XAUTH=/tmp/.docker.xauth; xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -; chmod 777 $XAUTH;

mode=$1
echo "the mode is $mode"
## Finding the USB ports to use with the GUI#############
mydevices=""
target="tty"
mydevicelist=()
for sysdevpath in $(find /sys/bus/usb/devices/usb*/ -name dev); do
    syspath="${sysdevpath%/dev}"
    devname="$(udevadm info -q name -p $syspath)"
    device="/dev/$devname"
    if [[ $device =~ $target ]]; then
        mydevicelist+=("$device")
    fi 
done
#echo "${mydevicelist[@]}"

aliasout=$(ls -alF /dev/ttyUSB*)
aliasprint=$(echo "$aliasout" | awk '{print $9}' | grep "$target")
#echo $aliasprint
aliasarr=($aliasprint)
for usbalias in "${aliasarr[@]}"; do
    mydevicelist+=("$usbalias")
done

for mydevice in "${mydevicelist[@]}"; do
    mydevices+="--device=$mydevice "
    mydevices+=" "
    echo "device loaded $mydevice"
done
mydevices=$(echo $mydevices | xargs)
echo $mydevices | xargs
################################################################################################

if [[ $mode == "dev" ]] 
then
    echo "running as $mode"
    docker run --detach-keys='ctrl-e,e' --rm -ti $mydevices -v ${PWD}:${PWD}\
	    -v ${PWD}/icicle/icicle:/home/cmsTkUser/Ph2_ACF_GUI/icicle/icicle:ro\
	    -v ${PWD}/Gui/siteConfig.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/siteSettings.py\
	    -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test\
	    -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data\
	    -v ${PWD}/Gui/QtGUIutils/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/QtGUIutils/\
	    -v ${PWD}/Gui/GUIutils/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/GUIutils/\
	    -v ${PWD}/Gui/python/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/python/\
	    -v ${PWD}/InnerTrackerTests/:/home/cmsTkUser/Ph2_ACF_GUI/InnerTrackerTests/\
	    -v ${PWD}/Configuration:/home/cmsTkUser/Ph2_ACF_GUI/Configuration\
		-v ${PWD}/FirmwareImages:/home/cmsTkUser/Ph2_ACF_GUI/FirmwareImages/\
		-v ${PWD}/felis:/home/cmsTkUser/Ph2_ACF_GUI/felis/\
	    -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH\
	    -e XAUTHORITY=$XAUTH --net host majoyce2/ph2_acf_gui_dev:latest #local/testimage423newdev:latest
else
    echo "running as user"
    docker run --pull=always --detach-keys='ctrl-e,e' --rm -ti $mydevices -v ${PWD}:${PWD}\
	    -v ${PWD}/Gui/siteConfig.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/siteSettings.py\
	    -v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test\
	    -v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data\
	    -w $PWD  -e DISPLAY=$DISPLAY -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH\
	    -e XAUTHORITY=$XAUTH --net host majoyce2/ph2_acf_gui_user:latest
fi
