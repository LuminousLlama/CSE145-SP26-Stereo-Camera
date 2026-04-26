# CSE145-SP26-Stereo-Camera


## setup 

sudo apt install git-lfs 
git lfs install 

## Docker setup 

1. install docker 

2. Enable QEMU for cross-platform builds (dev laptop only)

is this needed? 
xhost +local:docker


buildx cross platform build 

docker buildx create --name multiarch --driver docker-container --bootstrap --use
docker buildx inspect
docker run --privileged --rm tonistiigi/binfmt --install all

docker run --rm -it \
  --ipc=host \
  -e DISPLAY=$DISPLAY \
  -e QT_X11_NO_MITSHM=1 \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  -v $HOME/.Xauthority:/root/.Xauthority:rw \
  -v /opt/HuarayTech:/opt/HuarayTech:ro \
  --network host \
  stereo:test

  Huaray tech needs to be installed on host manually

  before running gui app need 
  `xhost +local:docker`
