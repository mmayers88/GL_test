# GL_test

This is my OGL tests on Raspberry pi. Trying to learn how to use various libraries.

---

## Dependecies


### check for updates

```
sudo apt-get update  
sudo apt-get upgrade 
```

### install dependencies

```
sudo apt install -y meson  
sudo apt-get install -y libxcb-randr0-dev libxrandr-dev  
sudo apt-get install -y libxcb-xinerama0-dev libxinerama-dev libxcursor-dev  
sudo apt-get install -y libxcb-cursor-dev libxkbcommon-dev xutils-dev  
sudo apt-get install -y xutils-dev libpthread-stubs0-dev libpciaccess-dev  
sudo apt-get install -y libffi-dev x11proto-xext-dev libxcb1-dev libxcb-*dev  
sudo apt-get install -y libssl-dev libgnutls28-dev x11proto-dri2-dev  
sudo apt-get install -y x11proto-dri3-dev libx11-dev libxcb-glx0-dev  
sudo apt-get install -y libx11-xcb-dev libxext-dev libxdamage-dev libxfixes-dev  
sudo apt-get install -y libva-dev x11proto-randr-dev x11proto-present-dev  
sudo apt-get install -y libclc-dev libelf-dev mesa-utils  
sudo apt-get install -y libvulkan-dev libvulkan1 libassimp-dev  
sudo apt-get install -y libdrm-dev libxshmfence-dev libxxf86vm-dev libunwind-dev  
sudo apt-get install -y libwayland-dev wayland-protocols  
sudo apt-get install -y libwayland-egl-backend-dev  
sudo apt-get install -y valgrind libzstd-dev vulkan-tools  
sudo apt-get install -y git build-essential bison flex ninja-build  
```
```
# Buster
sudo apt-get install -y python-mako vulkan-utils

# or Bullseye
sudo apt-get install -y python3-mako
```
```
sudo apt install -y libdrm-dev libgbm-dev libegl-dev libgl-dev  
sudo apt install -y llvm 
```

## Build
```
wget https://archive.mesa3d.org/mesa-23.1.9.tar.xz  
tar -xf mesa-23.1.9.tar.xz  
cd mesa-23.1.9/  
meson setup builddir/  
meson compile -C builddir/  
sudo meson install -C builddir/
```
