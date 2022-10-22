#!/bin/sh

cd /sys/kernel/config/usb_gadget/

mkdir g1
cd g1

echo "0x1d6b" > idVendor
echo "0x0104" > idProduct

mkdir strings/0x409
echo "0123456789" > strings/0x409/serialnumber
echo "SudoMaker" > strings/0x409/manufacturer
echo "Notkia v2.0" > strings/0x409/product

mkdir functions/acm.GS0
mkdir functions/acm.GS1
mkdir functions/rndis.usb0

mkdir configs/c.1
mkdir configs/c.1/strings/0x409
echo "FumoFumo" > configs/c.1/strings/0x409/configuration
ln -s functions/acm.GS0 configs/c.1
ln -s functions/acm.GS1 configs/c.1
ln -s functions/rndis.usb0 configs/c.1

echo 13500000.usb > UDC

# USB Network
sleep 3
ifconfig usb0 up 192.168.77.2

exit 0
