# CSE145-SP26-Stereo-Camera


## setup

1. install [docker](https://docs.docker.com/engine/install/ubuntu/)

2. clone this repository
3. install git lfs 
    - `sudo apt install git-lfs`
    - `git lfs install`
4. (in the root of this repo) `git lfs pull`
5. install vscode extension `Dev Containers (Microsoft)`
6. `xhost +local:docker` (you need to do this to allow gui apps in the container to run on your host every time you reboot your machine)
6. command pallet: `Dev Containers: Reopen in Container`


## Building for prod with docker 
you only need to do this if you are building the arm64 image that runs on the PI 

```
docker run --privileged --rm tonistiigi/binfmt --install all`
docker buildx create --name multiarch --driver docker-container --use
docker buildx inspect --bootstrap
``` 

build arm64 and amd64 images 
```
docker buildx build --platform linux/amd64,linux/arm64 -t luminousllama/stereo-camera:latest --push .
```

## rubik pi setup 

!! before installing huaray tech sdk 
```
# needed for HuarayTech sdk
sudo apt update
sudo apt install linux-headers-$(uname -r)
sudo apt install build-essential linux-headers-$(uname -r)
# now install the HuarayTech sdk

# install ros2 jazzy https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html

```

## wifi 

TODO: add wifi.sh script in this repo

sudo nmcli --ask connection up UCSD-PROTECTED