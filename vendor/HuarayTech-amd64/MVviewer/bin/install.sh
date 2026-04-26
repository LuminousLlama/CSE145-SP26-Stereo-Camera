#!/bin/bash
# ***************************************************************************************
#     Copyright (c) 2015-2019
# ***************************************************************************************

START_DIR=`dirname $0`
START_DIR=`cd $START_DIR/..; pwd`
INSTALL_ROOT=$START_DIR
LDCONFIG_BIN=/sbin/ldconfig
LDCONFIG_CFG_LOCATION=ld.so.conf.d
LDCONFIG_CFG_LOCATION_PATH=/etc/$LDCONFIG_CFG_LOCATION
LDCONFIG_CFG_FILE=/etc/ld.so.conf
VENUS_CFG_LOCATION=MV_SDK.conf
VENUS_CFG_LOCATION_PATH=$LDCONFIG_CFG_LOCATION_PATH/$VENUS_CFG_LOCATION

DRIVER_SCRIPT=loadAllDrv.sh
USB_STACK_SCRIPT=set_usb_stack.sh
RC_LOCAL=rc.local
RC_LOCAL_LOCATION=/etc/$RC_LOCAL
RC_D_LOCAL_LOCATION=/etc/rc.d/$RC_LOCAL

FLEXIBLE_NAME=rc-local.service
FLEXIBLE_FROM=/lib/systemd/system/
FLEXIBLE_TO=/etc/systemd/system/

#CentOS6.5 and Ubuntu use 'lsb_release' command to get $LINUX_OS and $LINUX_OS_VERSION
if [ ! -f "/etc/os-release" ]; then
	LINUX_OS=`lsb_release -d | awk '{print $2}'`
	if [ $LINUX_OS = "CentOS" ]; then
		LINUX_OS_VERSION=`lsb_release -d | awk '{print $4}'`
	elif [ $LINUX_OS = "Ubuntu" ]; then
		LINUX_OS_VERSION=`lsb_release -d | awk '{print $3}'`
	fi
else
	LINUX_OS_TMP=`cat /etc/os-release | grep PRETTY_NAME | awk '{print $1}'`
	if [[ $LINUX_OS_TMP =~ "Ubuntu" ]]; then	
		LINUX_OS=`lsb_release -d | awk '{print $2}'`
		LINUX_OS_VERSION=`lsb_release -d | awk '{print $3}'`
	elif [[ $LINUX_OS_TMP =~ "CentOS" ]]; then
		#CentOS7 use '/etc/os-release' to get $LINUX_OS_VERSION
		LINUX_OS=CentOS
		LINUX_OS_VERSION=`cat /etc/os-release | grep PRETTY_NAME | awk '{print $3}'`		
	fi
fi

HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`

DisplayHelp()
{
    echo ""
    echo "NAME"
    echo "    install_libraries.sh - Installs or removes the libraries from the path."
    echo ""
    echo "SYNOPSIS"
    echo "    install_libraries.sh [ --install ][ --uninstall ][ --help ]"
    echo ""
    echo "DESCRIPTION"
    echo "    This script is used to add the libraries to the ldconfig application."
    echo ""
    echo ""
    echo "COPYRIGHT"
    echo "    Copyright (c) 2015-2019"
    echo ""
    echo "VERSION"
    echo "    1.0.8.3423"
    echo ""
}

Install()
{
    echo ""

    if which $LDCONFIG_BIN >/dev/null 2>&1; then

		# Remove old MV SDK instructions if any present
		if [ -f "$VENUS_CFG_LOCATION_PATH" ]; then
			echo "Removing $VENUS_CFG_LOCATION_PATH"
			rm -f $VENUS_CFG_LOCATION_PATH
		fi

        echo "Creating $VENUS_CFG_LOCATION_PATH"

        # Ensure the proper configuration of ldconfig
        if [ ! -d "$LDCONFIG_CFG_LOCATION_PATH" ]; then
            
			# Ensure we have at least the configuration file
			if [ ! -f "$LDCONFIG_CFG_FILE" ]; then
				echo "Unable to find the file $LDCONFIG_CFG_FILE"
				echo "This problem must be fix before re-running this script or "
				echo " use LD_LIBRARY_PATH to work around this situation"
				exit 3
			fi

            # Add missing directive into the file
            echo "Add directive to find $LDCONFIG_CFG_LOCATION_PATH conf files from $LDCONFIG_BIN" 
            echo "include $LDCONFIG_CFG_LOCATION/*.conf" >> $LDCONFIG_CFG_FILE

            # Create the missing folder
            mkdir $LDCONFIG_CFG_LOCATION_PATH
        fi

        # Build the list of libraries location
        SCRIPT_DIR=`readlink -f $0`
        SDK_BIN_DIR=`dirname $SCRIPT_DIR`
        SDK_DIR=`cd $SDK_BIN_DIR/..; pwd`
		SDK_USB_DRIVER_DIR="$SDK_DIR/module/USBDriver"
		SDK_GIGE_DRIVER_DIR="$SDK_DIR/module/GigEDriver"
        SDK_LIB_DIR="$SDK_DIR/lib"
        GENICAM_ENUM_LIBRARIES=`find $SDK_LIB_DIR/GenICam/bin -type d`

        # Write the file into the library
        echo "$SDK_LIB_DIR" >> $VENUS_CFG_LOCATION_PATH
        echo "$GENICAM_ENUM_LIBRARIES" >> $VENUS_CFG_LOCATION_PATH
        echo "" >> $VENUS_CFG_LOCATION_PATH
        echo "" >> $VENUS_CFG_LOCATION_PATH

		echo ""
		# write auto loadDrviver into the start script
		if [ $LINUX_OS = "CentOS" ]; then
			 echo "Adding driver loading content to the $RC_D_LOCAL_LOCATION file of the system $LINUX_OS $LINUX_OS_VERSION"
			 
			 echo "" >> $RC_D_LOCAL_LOCATION
			 echo "cd $INSTALL_ROOT/module" >> $RC_D_LOCAL_LOCATION	
			 echo "$INSTALL_ROOT/module/$DRIVER_SCRIPT" >> $RC_D_LOCAL_LOCATION     
			 echo "" >> $RC_D_LOCAL_LOCATION
			 echo "cd $INSTALL_ROOT/bin" >> $RC_D_LOCAL_LOCATION
			 echo "$INSTALL_ROOT/bin/$USB_STACK_SCRIPT" >> $RC_D_LOCAL_LOCATION
			 echo "" >> $RC_D_LOCAL_LOCATION
			 
			 `chmod +x $RC_D_LOCAL_LOCATION`

		elif [ $LINUX_OS = "Ubuntu" ]; then
			if [ -f "$RC_LOCAL_LOCATION" ]; then
				echo "Adding driver loading content to the $RC_LOCAL_LOCATION file of the system $LINUX_OS $LINUX_OS_VERSION"

				`sed -i '/nothing/a\'$INSTALL_ROOT/module/$DRIVER_SCRIPT'' $RC_LOCAL_LOCATION`
				`sed -i '/nothing/a\cd '$INSTALL_ROOT/module'' $RC_LOCAL_LOCATION`
				`sed -i '/nothing/a\'$INSTALL_ROOT/bin/$USB_STACK_SCRIPT'' $RC_LOCAL_LOCATION`
				`sed -i '/nothing/a\cd '$INSTALL_ROOT/bin'' $RC_LOCAL_LOCATION`
			
				`chmod +x $RC_LOCAL_LOCATION`
			else
				echo "Adding $FLEXIBLE_NAME service for system $LINUX_OS $LINUX_OS_VERSION"
				`ln -s $FLEXIBLE_FROM/$FLEXIBLE_NAME $FLEXIBLE_TO/$FLEXIBLE_NAME`

				echo "Generating $RC_LOCAL_LOCATION file to load driver of the system $LINUX_OS $LINUX_OS_VERSION"
				`touch $RC_LOCAL_LOCATION`
				
				echo "#!/bin/bash" >> $RC_LOCAL_LOCATION
				echo "" >> $RC_LOCAL_LOCATION
				echo "pushd $INSTALL_ROOT/module >>/dev/null" >> $RC_LOCAL_LOCATION
				echo "$INSTALL_ROOT/module/$DRIVER_SCRIPT" >> $RC_LOCAL_LOCATION
				echo "popd >>/dev/null" >> $RC_LOCAL_LOCATION
				echo "" >> $RC_LOCAL_LOCATION
				echo "pushd $INSTALL_ROOT/bin >>/dev/null" >> $RC_LOCAL_LOCATION
				echo "$INSTALL_ROOT/bin/$USB_STACK_SCRIPT" >> $RC_LOCAL_LOCATION
				echo "popd >>/dev/null" >> $RC_LOCAL_LOCATION
				echo "" >> $RC_LOCAL_LOCATION
				echo "exit 0" >> $RC_LOCAL_LOCATION

				`chmod +x $RC_LOCAL_LOCATION`	    
			fi
	
		fi

		echo ""
        echo "Reloading libraries manually"
        # Do only reload the needed lib to not show errors if any from somewhere else
        $LDCONFIG_BIN -n $SDK_LIB_DIR $GENICAM_ENUM_LIBRARIES
		$LDCONFIG_BIN
    else
        echo "Cannot find the application /sbin/ldconfig to make the installation."
        echo "Your system may be setup manually by using LD_LIBRARY_PATH."
    fi 

UDEV_RULES_NAME=60-u3v-drv.rules
UDEV_RULES_PATH=/etc/udev/rules.d
UDEV_RULES_U3V_PATH=$UDEV_RULES_PATH/$UDEV_RULES_NAME

if [ -d $SDK_USB_DRIVER_DIR ]; then
	if [ -f "$UDEV_RULES_U3V_PATH" ]; then
		echo "Removing $UDEV_RULES_U3V_PATH"
		rm -f $UDEV_RULES_U3V_PATH
	fi

>$UDEV_RULES_U3V_PATH
cat > $UDEV_RULES_U3V_PATH <<__END__
SUBSYSTEMS=="usb", GROUP="users", MODE="0666"
__END__
fi
}

Uninstall()
{
    if [ -f "$VENUS_CFG_LOCATION_PATH" ]; then
        echo ""
        
        echo "Removing $VENUS_CFG_LOCATION_PATH"
        rm -f $VENUS_CFG_LOCATION_PATH
        
        echo "Reloading library path"
        $LDCONFIG_BIN

    fi
}

ACTION=""

# Parse the input arguments
for i in $*
do
    case $i in        
        --install)
            ACTION="INSTALL"
            ;; 
        --uninstall)
            ACTION="UNINSTALL"
            ;; 
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

# Default variables
USER_ID=`id -u`

# Check required priviledge
if [ "$USER_ID" != "0" ]; then
  echo "You need to run this script as superuser (root account)."
  exit 1
fi

if [ -z "$ACTION" ]; then
    echo ""
    echo "The following operations can be performed by this script:"
    echo ""
    echo "0 - Add libraries to the path."
    echo "1 - Remove libraries from the path."
    echo ""
    ANSWER="not set"
    until [ "$ANSWER" = "0" -o "$ANSWER" = "1" ]; do
        echo -n "Enter your selection [0|1]. Default is 0. "
        read ANSWER
        if [ -z "$ANSWER" ]; then
            ANSWER="0"
        fi
    done

    # Convert the selection into usable variables
    if [ "$ANSWER" = "0" ]; then
        ACTION="INSTALL"
    elif [ "$ANSWER" = "1" ]; then
        ACTION="UNINSTALL"
    fi
fi

# Now we can install if this was the action to perform
if [ "$ACTION" = "INSTALL" ]; then
    Install
elif [ "$ACTION" = "UNINSTALL" ]; then
    Uninstall
fi

echo ""



