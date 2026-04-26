#!/bin/bash

KERNEL_RELEASE=
LINUX_VERSION_CODE=

#the version code of kernel 4.20.0 ((4 << 16) + (20 << 8) + 0)
KERNEL_4_20_0_VERSION_CODE=267264

function GetKernelCode(){
	#例:4.18.0-10-generic
	KERNEL_RELEASE=`uname -r`
	#例:4.18.0 (VERSION.PATCHLEVEL.SUBLEVEL)
	KERNEL_VERSION=${KERNEL_RELEASE%%-*}
	#例:VERSION(4)
	VERSION=${KERNEL_VERSION%%.*}
	#例:PATCHLEVEL(18)
	PATCHLEVEL_TMP=${KERNEL_VERSION%.*}
	PATCHLEVEL=${PATCHLEVEL_TMP##*.}
	#例:SUBLEVEL(0)
	SUBLEVEL=${KERNEL_VERSION##*.}

	#(VERSION << 16) + (PATCHLEVEL << 8) + SUBLEVEL
	LINUX_VERSION_CODE=`expr $VERSION \* 65536 + 0$PATCHLEVEL \* 256 + 0$SUBLEVEL`
}

HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`
if [ $HOST_ARCH = "x86_64" ]; then
	driverName=m64x86
elif [ $HOST_ARCH = "i686" ]; then
	driverName=m32x86
else
	driverName=arm
fi

cp -rf ./libModule/*.a ./
make platform=$driverName clean

#获取KernelCode
GetKernelCode

if [ $? == 0 ]; then
	echo kernel release:$KERNEL_RELEASE    linux version code:$LINUX_VERSION_CODE
	if [ $LINUX_VERSION_CODE -ge $KERNEL_4_20_0_VERSION_CODE ]; then
		#kernel 4.20.0及以上的内核版本编译使用.o文件，否则用静态库
		cp -rf libModule/libuniversalEthernet-core.a libModule/libuniversalEthernet-core.o
		touch libModule/.libuniversalEthernet-core.o.cmd
		makeFile=./Makefile
		if [ -a $makeFile ]; then
			sed -i 's/libuniversalEthernet-core.a/libuniversalEthernet-core.o/g' $makeFile
		fi
	fi
fi

make platform=$driverName
chmod 777 *.ko
