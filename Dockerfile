FROM ros:jazzy-perception-noble AS base
ARG TARGETARCH
WORKDIR /ros2_ws

# install rviz2 and rqt
RUN apt-get update && apt-get install -y --no-install-recommends \ 
    ros-jazzy-rviz2 \
    ros-jazzy-rqt \ 
    && rm -rf /var/lib/apt/lists/*


# HuarayTech stuff 
COPY vendor/HuarayTech-${TARGETARCH} /opt/HuarayTech
RUN GENICAM_BIN=/opt/HuarayTech/MVviewer/lib/GenICam/bin; \
    [ "$TARGETARCH" = "amd64" ] && GENICAM_BIN="${GENICAM_BIN}/Linux64_x64"; \
    printf "/opt/HuarayTech/MVviewer/lib\n${GENICAM_BIN}\n" > /etc/ld.so.conf.d/huaray.conf && ldconfig


# Dev used by vscode dev containers. Empty because code is mounted at runtime 
FROM base AS dev
# add git lfs to dev to be able to push
RUN apt-get update \ 
    && apt-get install -y --no-install-recommends git-lfs \ 
    && git lfs install \
    && rm -rf /var/lib/apt/lists/*


# Prod used for docker pull and run on the PI directly
FROM base AS prod
COPY src/ src/
RUN apt-get update \
    && rosdep install --from-paths src --ignore-src -y \
    && rm -rf /var/lib/apt/lists/*
RUN . /opt/ros/jazzy/setup.sh && colcon build
CMD ["bash"]
