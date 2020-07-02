#!/bin/sh

machine=`uname -m`

case "$machine" in
    "arm") 
        cpu="arm"
        cpufamily="arm"
        archabi="armhf"
        syslibdir="arm-linux-gnueabihf"
    ;;
    "armv7l")
        cpu="armv7l"
        cpufamily="arm"
        archabi="armhf"
        syslibdir="arm-linux-gnueabihf"
    ;;
    "aarch64")
        cpu="aarch64"
        cpufamily="arm"
        archabi="arm64"
        syslibdir="aarch64-linux-gnu"
    ;;
    "x86_64")
        cpu="x86_64"
        cpufamily="x86"
        archabi="amd64"
        syslibdir="x86_64-linux-gnu"
    ;;
    *) 
        echo "Unsupported machine $machine"
        exit 1
    ;;
esac

echo machine $machine
echo cpu $cpu
echo cpufamily $cpufamily
echo archabi $archabi
echo syslibdir $syslibdir

