# https://www.ros.org/reps/rep-2001.html#id35
FROM ros:jazzy-perception-noble AS base
WORKDIR /ros2_ws

ARG TARGETARCH

WORKDIR /ros2_ws
COPY vendor/HuarayTech-${TARGETARCH} /opt/HuarayTech
RUN printf "/opt/HuarayTech/MVviewer/lib\n/opt/HuarayTech/MVviewer/lib/GenICam/bin\n" > /etc/ld.so.conf.d/huaray.conf && ldconfig


FROM base AS builder
RUN echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/10no-check-valid-until \
    && apt-get update && rosdep install --from-paths src --ignore-src -y && rm -rf /var/lib/apt/lists/*

COPY src/ src/
RUN . /opt/ros/jazzy/setup.sh && colcon build


# FROM base AS final
# COPY --from=builder /ros2_ws/install install/

# CMD ["bash"]