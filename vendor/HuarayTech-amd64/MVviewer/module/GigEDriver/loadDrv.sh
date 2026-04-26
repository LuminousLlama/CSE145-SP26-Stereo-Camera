#!/bin/bash

# Variables
CHMOD_BIN=chmod
CP_BIN=cp
ECHO_BIN=echo
MAKE_BIN=make
MKDIR_BIN=mkdir
MV_BIN=mv
POP_BIN=popd
PUSHD_BIN=pushd

HOST_VERSION=`uname -r`
OUT_FOLDER=$PWD
SRC_FOLDER=$PWD/drivers
KERNEL_FOLDER=
PREFIX=
PATH_TOOLS=

HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`
if [ $HOST_ARCH = "x86_64" ]; then
  platform=m64x86
  traceLev=3
elif [ $HOST_ARCH = "i686" ]; then
  platform=m32x86
  traceLev=3
else
  platform=arm
  traceLev=3
fi

DRIVER_NAME=mvUniversalForEthernet

################################################################################
#
# loadDrv.sh
#
################################################################################

#Display the help for this script
DisplayHelp()
{
    echo ""
    echo "NAME"
    echo "    loadDrv.sh - Load the Universal For Ethernet driver module "
    echo ""
    echo "SYNOPSIS"
    echo "    bash loadDrv.sh [--help]"
    echo ""
    echo "DESCRIPTION"
    echo "    Load the Universal For Ethernet module and configure the system to be ready to use"
    echo ""
    echo "    This script can only used by root or sudoer"
    echo "    --help             Display this help"
    echo ""
    echo ""
}

# check if we need to open firewall source port 3956
echo "open firewall source port 3956"
iptables -I INPUT -p udp --sport 3956 -j ACCEPT

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

# Parse the input arguments
for i in $*
do
    case $i in        
        --help)
            DisplayHelp
            exit 0
        ;;
        *)
        # unknown option
        DisplayHelp
        exit 1
        ;;
    esac
done

# Check required priviledge
if [ `whoami` != root ]; then
	ErrorOut "This script can only be run by root user or sudoer" 1
fi

DRIVER_FILE=$DRIVER_NAME.ko
if [ ! -e $DRIVER_FILE ]; then
	ErrorOut "$DRIVER_NAME.ko file is not exist!" 1
fi

# Load the module
echo "Loading Universal Ethernet..."
insmod ./$DRIVER_NAME.ko $* trace_param=$traceLev
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

# Recreate the nodes
echo "Create device node..."
major=`cat /proc/devices | grep universalEthernet`
major=${major%% universalEthernet*}
mknod /dev/universalEthernet c $major 0

# Change permission
chmod 777 /dev/universalEthernet

# Modify the socket configuration
echo "Adjust network settings..."
sysctl -w net.core.rmem_max=10485760 > /dev/null
sysctl -w net.core.wmem_max=10485760 > /dev/null

echo "Loading Universal Ethernet drivers successfully!"

