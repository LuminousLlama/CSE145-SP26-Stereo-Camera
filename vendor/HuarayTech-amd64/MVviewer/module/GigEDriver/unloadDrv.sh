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
elif [ $HOST_ARCH = "i686" ]; then
  platform=m32x86
else
  platform=arm 
fi

DRIVER_NAME=mvUniversalForEthernet
DriverList=`cat /proc/modules | grep universalEthernet_$platform`
if [ -n "$DriverList" ]; then
	DRIVER_NAME=universalEthernet_$platform
fi

################################################################################
#
# unloadDrv.sh
#
################################################################################

# Display the help for this script
DisplayHelp()
{
    echo ""
    echo "NAME"
    echo "    unloadDrv.sh - Unload the Universal Ethernet driver module "
    echo ""
    echo "SYNOPSIS"
    echo "    bash unloadDrv.sh [--help]"
    echo ""
    echo "DESCRIPTION"
    echo "    Unload the Universal Ethernet module and remove the configure"
	echo "    from the system to be ready to use"
    echo "    This script can only used by root or sudoer"
    echo "    --help             Display this help"
    echo ""
    echo ""
}

# Print out the error and exit 
#  $1 Error message
#  $2 Exit code
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

# Unload the module
echo "Unloading Universal Ethernet..."
rmmod $DRIVER_NAME.ko $* || exit 1

# Remove existing node if any
echo "Delete device node..."
rm -f /dev/universalEthernet 

echo "Unloading Universal Ethernet drivers successfully!"
