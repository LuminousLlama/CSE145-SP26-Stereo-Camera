#!/bin/bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export FASTRTPS_DEFAULT_PROFILES_FILE=$(ros2 pkg prefix camera_node)/share/camera_node/fastdds_profile.xml
