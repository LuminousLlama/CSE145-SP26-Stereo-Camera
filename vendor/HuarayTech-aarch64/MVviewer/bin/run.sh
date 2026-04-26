#!/bin/bash

START_DIR=`dirname $0`
START_DIR=`cd $START_DIR/..; pwd`

export LD_LIBRARY_PATH=./:$START_DIR/lib/:$START_DIR/lib/Qt:$START_DIR/lib/GenICam/bin/:$LD_LIBRARY_PATH
$START_DIR/bin/MVviewer
