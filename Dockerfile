FROM osrf/ros:jazzy-desktop AS base
WORKDIR /ros2_ws

# SDK present in every downstream stage (builder needs headers, final needs runtime libs)
COPY vendor/HuarayTech /opt/HuarayTech

# Register loader paths once, here — inherited by both builder and final
RUN echo "/opt/HuarayTech/MVviewer/lib" > /etc/ld.so.conf.d/huaray.conf && \
    echo "/opt/HuarayTech/MVviewer/lib/GenICam/bin/Linux64_x64" >> /etc/ld.so.conf.d/huaray.conf && \
    ldconfig


FROM base AS builder
COPY src/ src/
RUN apt-get update && rosdep install --from-paths src --ignore-src -y && rm -rf /var/lib/apt/lists/*
RUN . /opt/ros/jazzy/setup.sh && colcon build


FROM base AS final
COPY --from=builder /ros2_ws/install install/

CMD ["bash"]