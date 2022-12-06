# This script will find devices connected to a USB port, so will only detect serial connections if you are using a usb to serial adapter.
# To find serial port connection that does not use a usb adapter:
# Follow the instructions found here : https://unix.stackexchange.com/questions/125183/how-to-find-which-serial-port-is-in-use
for sysdevpath in $(find /sys/bus/usb/devices/usb*/ -name dev); do
    (
        syspath="${sysdevpath%/dev}"
        devname="$(udevadm info -q name -p $syspath)"
        [[ "$devname" == "bus/"* ]] && exit
        eval "$(udevadm info -q property --export -p $syspath)"
        [[ -z "$ID_SERIAL" ]] && exit
        echo "/dev/$devname - $ID_SERIAL"
    )
done
