#!/bin/sh

# LCD coupe hack
/sbin/notkia-fbtft-unbind.sh
/sbin/blanker
/sbin/notkia-fbtft-bind.sh
echo 15 > /sys/class/leds/notkia::backlight/brightness
sleep 0.5
clear > /dev/tty1
echo -ne "\nWelcome to Notkia!\n\n" > /dev/tty1

# Audio AMP
gpioset 5 12=0
gpioset 5 11=0

# BQ25895 CE
gpioset 5 7=1

exit 0
