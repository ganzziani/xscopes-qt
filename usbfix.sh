FILEPATH="/etc/udev/rules.d/libusb.rules"
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi
if [ "$1" = "" ]; then
 echo "Usage: usbfix.sh USERNAME"
 exit 1
fi
touch $FILEPATH
echo SUBSYSTEM==\"usb\", ACTION==\"add\", ATTR{idVendor}==\"16d0\", ATTR{idProduct}==\"06f9\", GROUP=\"$1\" > $FILEPATH
udevadm control --reload-rules
echo "Done! Now unplug your device and then plug it again" 
