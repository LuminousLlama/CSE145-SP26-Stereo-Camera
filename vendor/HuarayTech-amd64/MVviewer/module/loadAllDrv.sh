#!/bin/bash
# ***************************************************************************************
#     Copyright (c) 2015-2019
# ***************************************************************************************

START_DIR=`dirname $0`
START_DIR=`cd $START_DIR/..; pwd`

#load GigEDriver
if [ -f $START_DIR/module/GigEDriver/*.ko ]; then
	pushd $START_DIR/module/GigEDriver >>/dev/null
	echo ""
	bash loadDrv.sh
	popd >>/dev/null 
fi
