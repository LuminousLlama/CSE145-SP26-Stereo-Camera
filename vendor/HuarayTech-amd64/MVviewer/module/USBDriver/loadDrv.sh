#!/bin/bash

#Print out the error and exit 
# $1 Error message
# $2 Exit code
ErrorOut()
{
	echo ""
	echo "Error: $1"
	echo ""
	exit $2
}

KERNEL_VER=`uname -r`
V4L2_MODULE_ROOT_PATH=/lib/modules/$KERNEL_VER/kernel/drivers/media
V4L2_MODULE_PATH_1=$V4L2_MODULE_ROOT_PATH/video
V4L2_MODULE_PATH_2=$V4L2_MODULE_ROOT_PATH/v4l2-core

if [ -f "$V4L2_MODULE_PATH_1/videodev.ko" ];then
	echo "loading $V4L2_MODULE_PATH_1/videodev.ko..."
	insmod $V4L2_MODULE_PATH_1/videodev.ko

	if [ -f "$V4L2_MODULE_PATH_1/v4l2-compat-ioctl32.ko" ];then
		echo "loading $V4L2_MODULE_PATH_1/v4l2-compat-ioctl32.ko..."
		insmod $V4L2_MODULE_PATH_1/v4l2-compat-ioctl32.ko
	fi
elif [ -d $V4L2_MODULE_PATH_2 ]; then
	if [ -f "$V4L2_MODULE_PATH_2/videodev.ko.xz" ]; then
		echo "decompression videodev.ko.xz file"
		cp -rf $V4L2_MODULE_PATH_2/videodev.ko.xz $V4L2_MODULE_PATH_2/videodev.ko.xz.bak
		xz -d $V4L2_MODULE_PATH_2/videodev.ko.xz
	fi

	if [ -f "$V4L2_MODULE_PATH_2/videodev.ko" ];then
		if [ -f "$V4L2_MODULE_ROOT_PATH/media.ko" ];then
			echo "loading $V4L2_MODULE_ROOT_PATH/media.ko..."
			insmod $V4L2_MODULE_ROOT_PATH/media.ko
		fi

		echo "loading $V4L2_MODULE_PATH_2/videodev.ko..."
		insmod $V4L2_MODULE_PATH_2/videodev.ko			
	fi	
else
	echo "no v4l2 module found!"
fi

#UDEV_RULES_CONTENT="usb" SUBSYSTEMS=="usb", GROUP="users", MODE="0666""

HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`
if [ $HOST_ARCH = "x86_64" ]; then
  traceLev=3
elif [ $HOST_ARCH = "i686" ]; then
  traceLev=3
elif [ $HOST_ARCH = "aarch64" ] || [ $HOST_ARCH = "arm" ]; then
  traceLev=3	
else
  traceLev=3
fi

DRIVER_NAME=u3v_drv

DRIVER_FILE=$DRIVER_NAME.ko
if [ ! -e $DRIVER_FILE ]; then
	ErrorOut "$DRIVER_NAME.ko file is not exist!" 1
fi

echo "Loading u3v_drv.ko..."
insmod ./$DRIVER_NAME.ko trace=$traceLev

if [ $? != 0 ]; then
	linuxVermagic=`uname -r`
	driverInfo=`modinfo $DRIVER_NAME.ko`
	driverInfo=`echo ${driverInfo#*vermagic:}`
	driverVermagic=`echo ${driverInfo%%SMP*}`
	if [ "$linuxVermagic" != "$driverVermagic" ];then
		echo ""
		echo "Linux Vermagic [$linuxVermagic]"
		echo "Driver Vermagic [$driverVermagic]"
		ErrorOut "Vermagic is not match!" 1
	else
		exit 1
	fi
fi

echo "Loading Universal U3V drivers successfully!"
