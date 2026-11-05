# 安装与编译

本文根据本机终端历史、CMake 配置和 2026-11-05 的实际构建结果整理。
主流程适用于 Ubuntu 22.04、ROS 2 Humble。示例工作空间是
`~/longxiaoze_ws`，本机对应 `/home/techshare/longxiaoze_ws`。

安装完成后的日常操作见 [easyrun.md](easyrun.md)，更新步骤见 [update.md](update.md)。

## 1. 环境与仓库

| 项目 | 本机实际配置 |
| --- | --- |
| 系统 | Ubuntu 22.04.5 LTS |
| 架构 | aarch64，Jetson |
| ROS | Humble，安装于 `/opt/ros/humble` |
| 驱动 | `odin_ros_driver`，版本 `0.14.4` |
| 仓库 | [Longxiaoze/longxiaoze-robot-driver-odin_ros_driver](https://github.com/Longxiaoze/longxiaoze-robot-driver-odin_ros_driver) |
| 分支 | `main` |
| OpenCV | 驱动构建使用 4.8.0，CMake 目录 `/usr/lib/cmake/opencv4` |
| USB | `2207:0019`，实测连接速度 5000M |

先安装 ROS 2 Humble，并确保以下文件存在：

```bash
test -f /opt/ros/humble/setup.bash
source /opt/ros/humble/setup.bash
echo "$ROS_DISTRO"
```

ROS 本身的安装步骤参考 [ROS 2 Humble 官方安装文档](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debians.html)。
下面的命令以 ROS 软件源已经配置完成为前提。

## 2. 安装依赖

终端历史中先后补装了 OpenCV、PCL、`cv_bridge` 和 `pcl_conversions`。
以下将这些依赖与 `CMakeLists.txt` 中的依赖统一列出：

```bash
sudo apt update
sudo apt-get install --no-remove -y \
  build-essential cmake git pkg-config usbutils gdb \
  python3-colcon-common-extensions python3-yaml \
  libgtk2.0-dev libavcodec-dev libavformat-dev libswscale-dev \
  libyaml-cpp-dev libusb-1.0-0-dev libssl-dev libeigen3-dev \
  libopencv-dev libpcl-dev

sudo apt-get install --no-remove -y \
  ros-humble-ament-cmake ros-humble-rclcpp ros-humble-std-msgs ros-humble-sensor-msgs \
  ros-humble-nav-msgs ros-humble-geometry-msgs ros-humble-visualization-msgs \
  ros-humble-cv-bridge ros-humble-image-transport ros-humble-pcl-conversions \
  ros-humble-message-filters ros-humble-tf2 ros-humble-tf2-ros \
  ros-humble-tf2-geometry-msgs ros-humble-ament-index-cpp \
  ros-humble-rosidl-default-generators ros-humble-rosidl-default-runtime \
  ros-humble-launch-ros ros-humble-rviz2
```

检查关键依赖：

```bash
source /opt/ros/humble/setup.bash
ros2 pkg prefix cv_bridge
ros2 pkg prefix pcl_conversions
pkg-config --modversion opencv4
pkg-config --modversion libusb-1.0
dpkg -L libpcl-dev | grep 'PCLConfig.cmake$'
```

历史中的桌面软件安装、Pango/GNOME 降级尝试和其他机器人项目的依赖不属于本驱动安装流程。

## 3. 下载源码

首次安装执行以下命令；已有此目录时按 [update.md](update.md) 更新。
仓库目录名与 ROS 包名不同，编译和运行时始终使用 `odin_ros_driver`。

```bash
mkdir -p ~/longxiaoze_ws/src
cd ~/longxiaoze_ws/src
git clone https://github.com/Longxiaoze/longxiaoze-robot-driver-odin_ros_driver.git
cd longxiaoze-robot-driver-odin_ros_driver
git remote -v
```

目录结构应为：

```text
~/longxiaoze_ws/
└── src/
    └── longxiaoze-robot-driver-odin_ros_driver/
        ├── CMakeLists.txt
        ├── package.xml
        ├── config/
        ├── include/
        ├── lib/
        ├── src/
        └── test/
```

SDK 静态库已随仓库提供，CMake 根据 `uname -m` 自动选择：

| 主机架构 | 库文件 |
| --- | --- |
| `aarch64` / ARM | `lib/liblydHostApi_arm.a` |
| `x86_64` | `lib/liblydHostApi_amd.a` |

本机的 ARM SDK 完整路径为
`/home/techshare/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver/lib/liblydHostApi_arm.a`。
库由本仓库的 `lib/` 目录直接链接。

## 4. 配置 USB 权限

```bash
sudo tee /etc/udev/rules.d/99-odin-usb.rules >/dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="2207", ATTR{idProduct}=="0019", MODE="0666", GROUP="plugdev"
EOF
sudo udevadm control --reload
sudo udevadm trigger
```

重新插拔 Odin 的 USB 连接，然后检查：

```bash
lsusb -d 2207:0019
lsusb -t
```

本机实测设备位于 USB 3.0 链路，速度显示 `5000M`。使用 USB 3.0 数据线和端口。

## 5. 编译与测试

在新的终端中，从工作空间根目录构建：

```bash
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
colcon build --packages-select odin_ros_driver \
  --cmake-args \
  -DBUILD_SYSTEM=ROS2 \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

`-DBUILD_SYSTEM=ROS2` 显式选择 ROS 2 构建。当前仓库的 `package.xml`
已经是 ROS 2 清单。

终端历史曾使用 `script/build_ros2.sh`；当前脚本包含未定义的 `WS_DIR`
以及清理构建目录、覆盖 `package.xml` 的操作。本文统一使用上面的直接构建命令。

测试并加载结果：

```bash
source ~/longxiaoze_ws/install/setup.bash
cd ~/longxiaoze_ws
colcon test --packages-select odin_ros_driver
colcon test-result --verbose
ros2 pkg prefix odin_ros_driver
```

本机测试结果为 `1 test, 0 errors, 0 failures, 0 skipped`。
测试覆盖 SDK USB 上下文转发；实际数据接收还需要连接 Odin。

## 6. 本机 OpenCV 安装历史与排查

本机历史中，`/usr/lib/libopencv_core.so` 对应的运行库曾需要补装。
实际使用 NVIDIA Jetson 软件源中的
`libopencv=4.8.0-1-g6371ee1` 修复，然后执行了 `sudo ldconfig`。
这是这台机器的软件源版本，不是所有 Ubuntu 主机都需要安装的固定版本。

先检查本机的软件源、包版本和链接：

```bash
apt-cache policy libopencv libopencv-dev libopencv-core4.5d
dpkg-query -W -f='${binary:Package}\t${Version}\t${db:Status-Abbrev}\n' 'libopencv*'
find /usr/lib /usr/local/lib /opt \
  \( -name OpenCVConfig.cmake -o -name 'libopencv_core.so*' \) -print 2>/dev/null
ls -l /usr/lib/libopencv_core.so*
readlink -e /usr/lib/libopencv_core.so
```

仅当该 Jetson 软件源提供以下确切版本，且需要复现同一运行库修复时执行：

```bash
apt-get -s install libopencv=4.8.0-1-g6371ee1
sudo apt-get install --no-remove libopencv=4.8.0-1-g6371ee1
sudo ldconfig
```

当前机器的驱动使用 OpenCV 4.8.0，而 `/opt/ros/humble/lib/libcv_bridge.so`
还依赖 OpenCV 4.5.4。因此链接器会输出 OpenCV/TBB 版本冲突提示。
当前 `CMakeLists.txt` 使用 `find_package(OpenCV REQUIRED)`，没有自动读取
`cv_bridge-extras.cmake` 并锁定同一版本的逻辑。

检查实际链接结果：

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ldd ~/longxiaoze_ws/install/odin_ros_driver/lib/odin_ros_driver/host_sdk_sample \
  | grep -E 'cv_bridge|opencv|libtbb|not found'
```

本机的短时实测已收到 RGB 图像并正常退出；这不构成长时间稳定性验证。
若发生首帧图像崩溃，应进一步统一驱动与 `cv_bridge` 的 OpenCV 构建版本。

## 7. 首次启动

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ros2 launch odin_ros_driver odin1_ros2.launch.py
```

确认日志出现 `Device ready and streams activated`。
本仓库已包含 USB 启动和退出崩溃修复，详见
[USB 修复说明](docs/usb_startup_crash.md)。

运行、参数、地图保存及日志路径见 [easyrun.md](easyrun.md)。
