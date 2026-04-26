#!/bin/bash

LOG_NAME=systemInfo.txt
LOG_PATH=/var/log/MVSDK
SYSTEM_INFO=$LOG_PATH/$LOG_NAME

if [ -f $SYSTEM_INFO ]; then
	rm -rf $SYSTEM_INFO
fi

USER_ID=`id -u`
if [ "$USER_ID" != "0" ]; then
	echo "ERROR: You need to run the installer as superuser (root account)."
	exit 1
fi

#cpu info
echo ">>cat /proc/cpuinfo | grep model\ name" >> $SYSTEM_INFO
echo `cat /proc/cpuinfo | grep model\ name` >> $SYSTEM_INFO
echo "" >> $SYSTEM_INFO

#ethernet
echo ">>lspci | grep -i ethernet" >> $SYSTEM_INFO
echo `lspci | grep -i ethernet` >> $SYSTEM_INFO
echo "" >> $SYSTEM_INFO

# GigE/U3v Driver
Lsmod_List=`lsmod | grep mvUniversalProForU3V`

if [[ $Lsmod_List =~ "mvUniversalProForU3V" ]]; then
	echo "U3v Driver is loaded." >> $SYSTEM_INFO
	echo "" >> $SYSTEM_INFO
else
	echo "U3v Driver is not loaded." >> $SYSTEM_INFO
	echo "" >> $SYSTEM_INFO
	if [ -d "../module/USBDriver" ]; then
		echo ">>ls ../module/USBDriver" >> $SYSTEM_INFO
		echo `ls ../module/USBDriver` >> $SYSTEM_INFO
		echo "" >> $SYSTEM_INFO

		if [ -f "../module/USBDriver/mvUniversalProForU3V.ko" ]; then
			echo ">>modinfo ../module/USBDriver/mvUniversalProForU3V.ko" >> $SYSTEM_INFO
			echo `modinfo ../module/USBDriver/mvUniversalProForU3V.ko` >> $SYSTEM_INFO
			echo "" >> $SYSTEM_INFO
		fi
	fi
fi

Lsmod_List=`lsmod | grep mvUniversalForEthernet`
if [[ $Lsmod_List =~ "mvUniversalForEthernet" ]]; then
	echo "GigE Driver is loaded." >> $SYSTEM_INFO
	echo "" >> $SYSTEM_INFO
else
	echo "GigE Driver is not loaded." >> $SYSTEM_INFO
	echo "" >> $SYSTEM_INFO
	if [ -d "../module/GigEDriver" ]; then
		echo ">>ls ../module/GigEDriver" >> $SYSTEM_INFO
		echo `ls ../module/GigEDriver` >> $SYSTEM_INFO
		echo "" >> $SYSTEM_INFO

		if [ -f "../module/USBDriver/mvUniversalForEthernet.ko" ]; then
			echo ">>modinfo ../module/GigEDriver/mvUniversalForEthernet.ko" >> $SYSTEM_INFO
			echo `modinfo ../module/GigEDriver/mvUniversalForEthernet.ko` >> $SYSTEM_INFO
			echo "" >> $SYSTEM_INFO
		fi
	fi
fi

#Kernel
echo ">>uname -a" >> $SYSTEM_INFO
echo `uname -a` >> $SYSTEM_INFO
echo "" >> $SYSTEM_INFO

#Linux OS
if [ ! -f "/etc/os-release" ]; then

	echo ">>lsb_release -a" >> $SYSTEM_INFO
	echo `lsb_release -a` >> $SYSTEM_INFO

	LINUX_OS=`lsb_release -d | awk '{print $2}'`
else
	echo ">>cat /etc/os-release" >> $SYSTEM_INFO
	echo `cat /etc/os-release` >> $SYSTEM_INFO

	LINUX_OS_TMP=`cat /etc/os-release | grep PRETTY_NAME | awk '{print $1}'`
	if [[ $LINUX_OS_TMP =~ "Ubuntu" ]]; then	
		LINUX_OS=`lsb_release -d | awk '{print $2}'`
	elif [[ $LINUX_OS_TMP =~ "CentOS" ]]; then
		LINUX_OS=CentOS	
	fi
fi
echo "" >> $SYSTEM_INFO

#rc.local
if [ $LINUX_OS = "CentOS" ]; then
	echo ">>cat /etc/rc.d/rc.local" >> $SYSTEM_INFO
	echo `cat /etc/rc.d/rc.local` >> $SYSTEM_INFO	
elif [ $LINUX_OS = "Ubuntu" ]; then
	echo ">>cat /etc/rc.local" >> $SYSTEM_INFO
	echo `cat /etc/rc.local` >> $SYSTEM_INFO
fi
echo "" >> $SYSTEM_INFO

#Kernel file
if [ $LINUX_OS = "CentOS" ]; then
	echo ">>ls /usr/src/kernels" >> $SYSTEM_INFO
	echo `ls /usr/src/kernels` >> $SYSTEM_INFO
elif [ $LINUX_OS = "Ubuntu" ]; then
	echo ">>ls /usr/src" >> $SYSTEM_INFO
	echo `ls /usr/src` >> $SYSTEM_INFO
fi
echo "" >> $SYSTEM_INFO

echo "Successed to save the $LOG_NAME to $LOG_PATH"


