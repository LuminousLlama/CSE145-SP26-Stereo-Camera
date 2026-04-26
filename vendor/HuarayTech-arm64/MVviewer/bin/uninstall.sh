#!/bin/bash
# ***************************************************************************************
#     Copyright (c) 2015-2019
# ***************************************************************************************

# Default variables
USR_LIB=/usr/lib

START_DIR=`dirname $0`
START_DIR=`cd $START_DIR/..; pwd`

RC_LOCAL=rc.local
RC_LOCAL_LOCATION=/etc/$RC_LOCAL

FLEXIBLE_NAME=rc-local.service
FLEXIBLE_TO=/etc/systemd/system/

INSTALL_ROOT=$START_DIR
LINUX_OS=`lsb_release -d | awk '{print $2}'`
LINUX_OS_VERSION=`lsb_release -d | awk '{print $3}'`
HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`

DisplayHelp()
{
    echo ""
    echo "NAME"
    echo "    uninstall.sh - Uninstalls the MV SDK."
    echo ""
    echo "SYNOPSIS"
    echo "    bash uninstall.sh [ --help ]"
    echo ""
    echo "DESCRIPTION"
    echo "   Uninstalls the MV SDK from the PC."
    echo "   This script can only used by the root or sudoer account."
    echo "    --help             Displays this help."
    echo ""
    echo "COPYRIGHT"
    echo "    Copyright (c) 2015-2019"
    echo ""
    echo "VERSION"
    echo "    4.0.8.3423"
    echo ""
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

# Initial screen
clear
echo "  ( $LINUX_OS-$LINUX_OS_VERSION-$HOST_ARCH )"
echo "========================================"
echo ""

# Ensure permission...
if [ ! -w $INSTALL_ROOT ]; then
	echo "You do not have write access to uninstall '$INSTALL_ROOT'."
	echo "Run the script as superuser (root account)."
	exit 1
fi

# Ensure the client want to uninstall the SDK
echo -n "Remove MV_SDK installed in the '$INSTALL_ROOT' directory"
echo ""

# We only remove the driver if this one is in memory
if [ -f "$INSTALL_ROOT/module/GigEDriver/unloadDrv.sh" ]; then
	bash "$INSTALL_ROOT/module/GigEDriver/unloadDrv.sh"
fi

if [ -f "$INSTALL_ROOT/module/USBDriver/unloadDrv.sh" ]; then
	bash "$INSTALL_ROOT/module/USBDriver/unloadDrv.sh"
fi

# We remove the symbolic link
if [ -f "$INSTALL_ROOT/bin/install.sh" ]; then
	bash "$INSTALL_ROOT/bin/install.sh" --uninstall
fi

# Delete auto loadDrviver and set usb stack into the start script
if [ -f "$FLEXIBLE_TO/$FLEXIBLE_NAME" ]; then
	echo "Canceling soft connections $FLEXIBLE_NAME of the system $LINUX_OS $LINUX_OS_VERSION"
	`rm -rf $FLEXIBLE_TO/$FLEXIBLE_NAME`

	echo "Delete the $RC_LOCAL_LOCATION in the $LINUX_OS $LINUX_OS_VERSION system"
	`rm -rf $RC_LOCAL_LOCATION`
else
	echo "Delete the driver loading content form the $RC_LOCAL_LOCATION file of the $LINUX_OS $LINUX_OS_VERSION system"
	`sed -i '/module/d' $RC_LOCAL_LOCATION`
	`sed -i '/MVviewer/d' $RC_LOCAL_LOCATION`
fi

if [ -d $INSTALL_ROOT/module/USBDriver ]; then 
	UDEV_RULES_NAME=60-u3v-drv.rules
	UDEV_RULES_PATH=/etc/udev/rules.d
	UDEV_RULES_U3V_PATH=$UDEV_RULES_PATH/$UDEV_RULES_NAME

	if [ -f "$UDEV_RULES_U3V_PATH" ]; then
		echo "Removing $UDEV_RULES_U3V_PATH"
		rm -f $UDEV_RULES_U3V_PATH
	fi
fi

echo ""
echo "Removing software on $INSTALL_ROOT."
if [ -d $INSTALL_ROOT ]; then
	EXCEPTION_FOLDER=""
	if [ -d "$INSTALL_ROOT/licenses" ]; then
		if [ -n "$( ls $INSTALL_ROOT/licenses/*.lic 2> /dev/null )" ]; then
			EXCEPTION_FOLDER="licenses"
		fi
	fi
	if [ -z "$EXCEPTION_FOLDER" ]; then
		rm -rf $INSTALL_ROOT
		rmdir --ignore-fail-on-non-empty $(readlink -m $INSTALL_ROOT/..) 
		rmdir --ignore-fail-on-non-empty $(readlink -m $INSTALL_ROOT/../..) 
	else
		ls -d $INSTALL_ROOT/* -1 | grep -v -E $EXCEPTION_FOLDER | xargs rm -rf
	fi
fi

echo ""
echo "MV SDK has been uninstalled."
echo ""
echo ""
