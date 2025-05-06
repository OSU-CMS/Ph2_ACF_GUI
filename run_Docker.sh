#!/bin/bash
SOCK=/tmp/.X11-unix; XAUTH=/tmp/.docker.xauth; xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -; chmod 777 $XAUTH;

mode=$1
## Finding the USB ports to use with the GUI#############
mydevices=""
target="tty"
mydevicelist=()
mydevicelist+='/dev/bus/usb/'
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
		-v ${PWD}/Ph2_ACF/settings/RD53Files:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/settings/RD53Files\
		-v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data\
		-v ${PWD}/Gui/QtGUIutils/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/QtGUIutils/\
		-v ${PWD}/Gui/GUIutils/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/GUIutils/\
		-v ${PWD}/Gui/python/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/python/\
		-v ${PWD}/InnerTrackerTests/:/home/cmsTkUser/Ph2_ACF_GUI/InnerTrackerTests/\
		-v ${PWD}/Configuration:/home/cmsTkUser/Ph2_ACF_GUI/Configuration\
		-v ${PWD}/F4T_Monitoring:/home/cmsTkUser/Ph2_ACF_GUI/F4T_Monitoring\
		-v ${PWD}/felis:/home/cmsTkUser/Ph2_ACF_GUI/felis/\
		-v ${PWD}/FirmwareImages:/home/cmsTkUser/Ph2_ACF_GUI/FirmwareImages/\
		-v ${PWD}/symlinks.sh:/home/cmsTkUser/Ph2_ACF_GUI/symlinks.sh/\
		-v ${PWD}/Gui/jsonFiles/:/home/cmsTkUser/Ph2_ACF_GUI/Gui/jsonFiles/\
		-w /home/cmsTkUser/Ph2_ACF_GUI -e DISPLAY=$DISPLAY\
		--volume="$HOME/.Xauthority:/root/.Xauthority:rw" --net host majoyce2/ph2_acf_gui_dev:latest 
else
    echo "running as user"
	IMAGE_NAME="majoyce2/ph2_acf_gui_user:latest"

        # Check if skopleo is installed, if not prompt user to install
        if ! which skopeo > /dev/null 2>&1; then
            echo -e "\e[31mPlease install skopeo. This allows us to check whether you are
using the latest version of the docker image for the Ph2_ACF_GUI.
To install on Alma Linux please run:\e[0m
\e[1;0msudo dnf -y install skopeo\e[0m"
            echo "We cannot ensure you are running the latest docker image. Continue at your own risk.
                  Would you like to continue? (y/n)" 
            read -r response
            if [[ "$response" =~ ^[Yy]$ ]]; then
                echo "Continuing with the script..."
            else
                echo "Exiting the script."
                exit 1
            fi
        fi
        
	LOCAL_DIGEST=$(docker inspect --format='{{index .RepoDigests 0}}' "$IMAGE_NAME" | cut -d'@' -f2)
	REMOTE_DIGEST=$(skopeo inspect docker://$IMAGE_NAME | jq -r '.Digest')

	if [[ "$LOCAL_DIGEST" == "$REMOTE_DIGEST" ]]; then
  		echo "The image is already up to date."
	else
  		echo "A newer image is available (this may be untrue if skopeo is not installed). Do you want to pull it? (y/n)"
  		read -r response
  		if [[ $response == "y" ]]; then
    		docker pull $IMAGE_NAME
    		echo "Image pulled successfully."
  		else
    		echo "Image pull canceled."
  		fi
	fi
    docker run --detach-keys='ctrl-e,e' --rm -ti $mydevices -v ${PWD}:${PWD}\
		-v ${PWD}/Gui/siteConfig.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/siteSettings.py\
		-v ${PWD}/Ph2_ACF/test:/home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test\
		-v ${PWD}/data:/home/cmsTkUser/Ph2_ACF_GUI/data\
        -v ${PWD}/Gui/jsonFiles:/home/cmsTkUser/Ph2_ACF_GUI/Gui/jsonFiles\
		-v ${PWD}/Gui/QtGUIutils/PeltierCoolingApp.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/QtGUIutils/PeltierCoolingApp.py\
        -v ${PWD}/Gui/python/Peltier.py:/home/cmsTkUser/Ph2_ACF_GUI/Gui/python/Peltier.py\
		-w $PWD  -e DISPLAY=$DISPLAY\
		--volume="$HOME/.Xauthority:/root/.Xauthority:rw" --net host majoyce2/ph2_acf_gui_user:latest #local/testimagejuly30user
		#Before, the docker run command had the options -v $XSOCK:$XSOCK -v $XAUTH:$XAUTH -e XAUTHORITY=$XAUTH. We were having trouble
		#running the GUI through SSH connections, so we removed those options and added --volume="$HOME/.Xauthority:/root/.Xauthority:rw"
		#which seemed to fix the issue of running the GUI from SSH connections. At the time of this commit, we have no idea why this fixed it
		#or what these lines did or do.
fi
