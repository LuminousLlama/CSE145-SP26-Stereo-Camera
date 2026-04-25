FROM osrf/ros:jazzy-desktop AS builder 

WORKDIR /ros2_ws

COPY src/ src/
RUN rosdep install --from-paths src --ignore-src -y
RUN . /opt/ros/jazzy/setup.sh && colcon build


FROM osrf/ros:jazzy-desktop AS final 

WORKDIR /ros2_ws

COPY --from=builder /ros2_ws/install install/