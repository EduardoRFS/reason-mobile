ARCH=$1
API=$2

sudo rm -f /system
sudo ln -s /opt/android/$ARCH-$API/system /system
