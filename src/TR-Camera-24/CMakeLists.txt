cmake_minimum_required(VERSION 3.8)
project(camera_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-O3 -Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)

add_executable(camera_node src/camera_node.cpp src/Camera.cpp)
ament_target_dependencies(camera_node rclcpp std_msgs cv_bridge sensor_msgs image_transport)
target_include_directories(camera_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(camera_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17

# opencv
find_package(OpenCV)
target_include_directories(camera_node PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(camera_node ${OpenCV_LIBS})

#huaray
set(HUARAY_LIB_DIR /opt/HuarayTech/MVviewer/lib)

# GenICam libs are in an arch-specific subdirectory on x86_64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(GENICAM_LIB_DIR ${HUARAY_LIB_DIR}/GenICam/bin/Linux64_x64)
else()
  set(GENICAM_LIB_DIR ${HUARAY_LIB_DIR}/GenICam/bin)
endif()

find_library(CAMERA_LIB
  NAMES MVSDK
  HINTS ${HUARAY_LIB_DIR}
  REQUIRED
)
target_link_directories(camera_node PUBLIC ${GENICAM_LIB_DIR})
target_link_libraries(camera_node ${CAMERA_LIB})

# our stuff

install(TARGETS camera_node
  DESTINATION lib/${PROJECT_NAME})

# Make trigger_node.py executable as a ROS2 node
find_package(ament_cmake_python REQUIRED)
install(PROGRAMS
  src/trigger_node.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME trigger_node
)

# Install FastDDS profile
install(FILES
  fastdds_profile.xml
  DESTINATION share/${PROJECT_NAME}
)

# Set env vars automatically when package is sourced
ament_environment_hooks(
  "${CMAKE_CURRENT_SOURCE_DIR}/env_hooks/camera_node.sh"
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
