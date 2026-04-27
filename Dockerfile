# https://www.ros.org/reps/rep-2001.html#id35
FROM ros:jazzy-perception-noble AS base
WORKDIR /ros2_ws

ARG TARGETARCH

WORKDIR /ros2_ws
COPY vendor/HuarayTech-${TARGETARCH} /opt/HuarayTech
RUN GENICAM_BIN=/opt/HuarayTech/MVviewer/lib/GenICam/bin; \
    [ "$TARGETARCH" = "amd64" ] && GENICAM_BIN="${GENICAM_BIN}/Linux64_x64"; \
    printf "/opt/HuarayTech/MVviewer/lib\n${GENICAM_BIN}\n" > /etc/ld.so.conf.d/huaray.conf && ldconfig


FROM base AS builder
RUN echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/10no-check-valid-until \
    && apt-get update && rm -rf /var/lib/apt/lists/*

COPY src/ src/
RUN rosdep install --from-paths src --ignore-src -y
RUN . /opt/ros/jazzy/setup.sh && colcon build


# FROM base AS final
# COPY --from=builder /ros2_ws/install install/

# CMD ["bash"]