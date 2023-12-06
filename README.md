# ScreenRecorder 

基于[OBS](https://github.com/obsproject/obs-studio)开发的一款桌面录制软件，可以实现在**全屏**及**指定区域**录制。


# 平台移植：
|平台| 结果     | 备注                                                 |
|--|--------|----------------------------------------------------|
|Windows| 完美运行   | Qt version 5.15.2 ,msvc2019_64                     |
|Linux| 初步编译完成 | 测试平台ubuntu 23.04,Qt version 5.15.2，遗留问题：**声卡采集卡顿** |
|Mac| 未验证    | 没测试用的机器捏XD                                         |

---

## 下载

(目前仅提供windows版本下载)

[下载链接](https://github.com/Type3limit/ScreenRecorder/releases)

---
## 效果预览

![效果预览](./screenShot/preview.gif)

---
## 操作快捷键

基于[QHotKey](https://github.com/Skycoder42/QHotkey)项目，集成于此[文件夹](./helper)

>预留```Alt+S```作为debug快捷键，可以唤起预览窗口检查当前采集画面，预览如下：
![debugwindow](./screenShot/debugwindow.gif)


## 构建相关

[bin](./bin)目录中保存了预编译好的OBS-plugin链接库，基于[当前cmake](./CMakeLists.txt)的默认构建会生成于对应构建目录中。

### Windows build

安装Qt 5.15.2版本，使用cmake配置`CMAKE_PREFIX_PATH`为Qt库文件安装路径。

如（以下指定`ninja`为默认生成工具，Qt目录位于`C:/Qt/5.15.2/`中）：
```shell
cmake.exe -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=ninja.exe -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64;
```

如需要修改Qt版本，自行修改替换[cmake](CMakeLists.txt)指定的Qt版本即可。

### Linux build

大致步骤同[windows端](#Windows-build),但需要安装一些库依赖文件：

#### 安装必要的构建依赖：
**Debian-based (apt)**：

*构建工具：*
```bash
sudo apt install cmake ninja-build pkg-config clang clang-format build-essential curl ccache git zsh
```
*obs core libs:*
```bash
sudo apt install libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev libx264-dev libcurl4-openssl-dev libmbedtls-dev libgl1-mesa-dev libjansson-dev libluajit-5.1-dev python3-dev libx11-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-xinerama0-dev libxcb-composite0-dev libxcomposite-dev libxinerama-dev libxcb1-dev libx11-xcb-dev libxcb-xfixes0-dev swig libcmocka-dev libxss-dev libglvnd-dev libgles2-mesa libgles2-mesa-dev libwayland-dev librist-dev libsrt-openssl-dev libpci-dev libpipewire-0.3-dev libqrcodegencpp-dev
```
*obs plugins:*
```bash
sudo apt install libasound2-dev libfdk-aac-dev libfontconfig-dev libfreetype6-dev libjack-jackd2-dev libpulse-dev libsndio-dev libspeexdsp-dev libudev-dev libv4l-dev libva-dev libvlc-dev libvpl-dev libdrm-dev nlohmann-json3-dev libwebsocketpp-dev libasio-dev
```

**Red Hat-based (yum)**
 ```bash
   sudo yum install \
          alsa-lib-devel \
          asio-devel \
          cmake \
          ffmpeg-free-devel \
          fontconfig-devel \
          freetype-devel \
          gcc \
          gcc-c++ \
          gcc-objc \
          git \
          glib2-devel \
          json-devel \
          libavcodec-free-devel \
          libavdevice-free-devel \
          libcurl-devel \
          libdrm-devel \
          libglvnd-devel \
          libjansson-devel \
          libuuid-devel \
          libva-devel \
          libv4l-devel \
          libX11-devel \
          libXcomposite-devel \
          libXinerama-devel \
          luajit-devel \
          make \
          mbedtls-devel \
          pciutils-devel \
          pipewire-devel \
          pulseaudio-libs-devel \
          python3-devel \
          qt5-qtbase-devel \
          qt5-qtbase-private-devel \
          qt5-qtsvg-devel \
          qt5-qtwayland-devel \
          qt5-qtx11extras-devel \
          speexdsp-devel \
          swig \
          systemd-devel \
          vlc-devel \
          wayland-devel \
          websocketpp-devel \
          x264-devel
   ```


**其余发行版本**

可参考[OBS build instructions for linux](https://github.com/obsproject/obs-studio/wiki/build-instructions-for-linux)