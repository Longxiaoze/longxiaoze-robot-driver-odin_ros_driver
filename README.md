# Odin ROS Driver

Odin1 传感器 ROS 驱动，提供图像、IMU、里程计、点云和 SLAM 数据接口。
本仓库基于 Manifold Tech 的 Odin ROS Driver，维护本机部署说明与 USB 崩溃修复。

- 当前仓库：[Longxiaoze/longxiaoze-robot-driver-odin_ros_driver](https://github.com/Longxiaoze/longxiaoze-robot-driver-odin_ros_driver)
- 默认分支：`main`
- ROS 包名：`odin_ros_driver`
- 厂商资料：[Odin1 Wiki](https://manifoldtechltd.github.io/wiki/Odin1/Cover.html)

| 文档 | 内容 |
| --- | --- |
| [install.md](install.md) | 参考终端历史整理的依赖安装、USB 权限、编译与 OpenCV 排查 |
| [easyrun.md](easyrun.md) | 日常启动、数据检查、配置、地图保存和输出路径 |
| [update.md](update.md) | 更新源码、备份配置、回归验证和 Git 推送 |
| [USB 崩溃修复](docs/usb_startup_crash.md) | 启动及退出崩溃的原因、实现与实机验证 |
| [运行时服务](docs/runtime_service_operations.md) | AE/AWB、保存地图、算法复位及日志服务 |
| [重定位指南](RELOCALIZATION_GUIDE.md) | 地图重定位的配置与使用 |
| [上游版本记录](CHANGELOG.md) | 厂商版本变更记录 |

## 1. 版本与验证环境

| 项目 | 当前状态 |
| --- | --- |
| 驱动版本 | `0.14.4` |
| 已验证系统 | Ubuntu 22.04.5 LTS，Jetson aarch64 |
| 已验证 ROS | ROS 2 Humble，`/opt/ros/humble` |
| 工作空间 | `/home/techshare/longxiaoze_ws`，下文简写为 `~/longxiaoze_ws` |
| 源码目录 | `~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver` |
| 实机固件信息 | SoC `0.14.1`，SLAM `0.13.0`；此处记录测试版本 |
| OpenCV | 驱动使用 4.8.0；系统 `cv_bridge` 另依赖 4.5.4，详见安装文档 |

源码包含 ROS 1 和 ROS 2 分支；本次构建及实机验证覆盖 ROS 2 Humble。
ROS 1 和其他发行版未在本机复测。

2026-11-05 已验证：构建成功，1 项回归测试通过，完整 launch 收到图像、
IMU、里程计和点云，Ctrl+C 正常退出。观察窗口为 25 秒。
RGB 参数设置超时仍存在，指定分辨率和帧率是否生效尚未验证。

## 2. 安装与构建入口

首次安装先完成 [install.md](install.md) 中的 ROS、系统依赖和 USB 权限配置，
再下载当前仓库：

```bash
mkdir -p ~/longxiaoze_ws/src
cd ~/longxiaoze_ws/src
git clone https://github.com/Longxiaoze/longxiaoze-robot-driver-odin_ros_driver.git
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
colcon build --packages-select odin_ros_driver \
  --cmake-args -DBUILD_SYSTEM=ROS2 -DBUILD_TESTING=ON
source install/setup.bash
colcon test --packages-select odin_ros_driver
colcon test-result --verbose
```

已有源码时使用 [update.md](update.md) 中的更新流程。
本文采用直接 `colcon build`；旧构建脚本的行为说明见安装文档。

SDK 已随仓库提供，CMake 从源码的 `lib/` 目录自动选取：

| 架构 | SDK 静态库 |
| --- | --- |
| ARM / aarch64 | [lib/liblydHostApi_arm.a](lib/liblydHostApi_arm.a) |
| x86_64 | [lib/liblydHostApi_amd.a](lib/liblydHostApi_amd.a) |

本机实际使用
`/home/techshare/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver/lib/liblydHostApi_arm.a`。
驱动可执行文件安装在
`~/longxiaoze_ws/install/odin_ros_driver/lib/odin_ros_driver/host_sdk_sample`。

## 3. 运行与配置

### 3.1 快速启动

```bash
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
source install/setup.bash
ros2 launch odin_ros_driver odin1_ros2.launch.py
```

连接成功后应出现 `Device ready and streams activated`。
按 Ctrl+C 正常退出。只需要主驱动时，可使用
`ros2 run odin_ros_driver host_sdk_sample`。
话题检查、建图和服务调用见 [easyrun.md](easyrun.md)。

### 3.2 配置与输出路径

主配置为 [config/control_command.yaml](config/control_command.yaml)，
RViz 配置为 [config/odin_ros2.rviz](config/odin_ros2.rviz)。
修改源码配置后重新构建，使安装目录中的辅助节点配置同步更新。
当前主驱动未使用 launch 传入的 `config_file` 参数选择配置文件。

| 内容 | 本机路径 |
| --- | --- |
| 主标定文件 | `~/.ros/odin_ros_driver/calib.yaml` |
| ROS launch 日志 | `~/.ros/log/` |
| 默认设备日志 | `~/longxiaoze_ws/src/odin_ros_driver/log/` |
| 默认地图 | `~/longxiaoze_ws/src/odin_ros_driver/map/` |
| 开启记录后的数据 | `~/longxiaoze_ws/src/odin_ros_driver/recorddata/` |

输出目录中的 `src/odin_ros_driver` 是当前代码保留的运行时路径，
与 Git 源码目录名不同。标定目录可通过 `ODIN_CALIB_DIR` 设置，
地图位置可通过 `mapping_result_dest_dir` 设置；细节见快速运行文档。

### 3.3 Operation Modes / 工作模式

The operation mode can be configured via the `custom_map_mode` parameter in config/control_command.yaml.

#### Odometry mode

Set `custom_map_mode = 0` to enable odometry mode. In this mode, the map frame and odom frame share the same pose.

If the odom data is found to drift, the script command "./set_param.sh algo_reset 1" can be used to dynamically reset the algorithm.

#### SLAM mode

Set `custom_map_mode = 1` to enable slam mode. This mode provides a complete SLAM system that builds upon the Odometry Mode by adding **loop closure detection** and **map saving** capabilities.

After launching the driver, odin1 will automatically perform mapping and cache map data. When the scene capture is complete, users need to execute `./set_param.sh save_map 1` in the driver's source directory to save all map data collected since the program started. The map will be saved to the location specified by the `mapping_result_dest_dir` and `mapping_result_file_name` parameters in config/control_command.yaml. If these parameters are not specified, default values will be used.

After the initial save, you can execute the command again to save a new map. Each save operation will generate a new map file. (Please allow at least 5 seconds between consecutive save operations)

The map origin corresponds to the odom coordinate system's origin at the program's startup.

##### Relocalization mode

To enable relocalization, set `custom_map_mode = 2` and specify the absolute path to the pre-built map using the `relocalization_map_abs_path` parameter in config/control_command.yaml.

Once launched, odin1 will initiate the relocalization process based on the current viewpoint and the specified map. To ensure a high success rate, it is recommended to starting within 1 meter ±10 degrees of the original position and orientation from the SLAM trajectory.

Note that relocalization performance is highly environment-dependent. In highly distinctive scenes, successful matching may occur even beyond the 1m/10° range, while other environments may require more stringent conditions. We advise testing in your target environment to determine practical tolerances.

If relocalization fails initially, the system will temporarily operate in a fallback SLAM mode (map saving is disabled in this state). During this time, you can freely move odin1. It will continue relocalization attempts in the background. Once successful, the TF between map and odom frames will be published. (Tip: Gently shaking or moving the device after initialization can help improve relocalization accuracy.)

The following topics are published in the odom frame: `/odin1/cloud_slam`, `/odin1/odometry`, `/odin1/odometry_highfreq` and `/odin1/path`. To obtain these in the map frame, apply the TF from odom frame to map frame.

## 4. File structure and data format
### 4.1 File structure
```shell
longxiaoze-robot-driver-odin_ros_driver/ // Git repository; ROS package: odin_ros_driver
    src/
        host_sdk_sample.cpp     // Example source code
        sdk_usb_context.cpp     // SDK private USB context adapter
        yaml_parser.cpp         // Source code for reading yaml parameters
        rawCloudRender.cpp      // Source code for RenderCloud
        depth_image_ros_node.cpp //depth_image_ros_node
        depth_image_ros2_node.cpp //depth_image_ros2_node
        pcd2depth_ros.cpp       //Source code for pcd2depth_ros
        pcd2depth_ros2.cpp      //Source code for pcd2depth_ros2
        pointcloud_depth_converter.cpp //Source code for pointcloud_depth_converter
        cloud_reprojection_ros.cpp //Source code for cloud reprojection node (ROS1/ROS2)
        cloud_reprojector.cpp   //Core logic for cloud reprojection
    lib/
        liblydHostApi_amd.a     // Static library for x86_64
        liblydHostApi_arm.a     // Static library for ARM platform
    include/
        host_sdk_sample.h       // Example header file
        lidar_api_type.h        // API data structure header file
        lidar_api.h             // API function declarations
        yaml_parser.h           // Parameter file reading header file
        rawCloudRender.h        // API about RenderCloud
        data_logger.h           // LOG about save_data
        depth_image_ros_node.hpp // depth_image_ros_node
        depth_image_ros2_node.hpp // depth_image_ros2_node
        pointcloud_depth_converter.hpp // pointcloud_depth_convert
        cloud_reprojection_ros_node.hpp // cloud_reprojection_ros_node (ROS1/ROS2)
        cloud_reprojector.hpp   // Core class for cloud reprojection
    config/
        control_command.yaml    // Control parameter file for driver
        calib.yaml              // Optional device-specific backup; primary file is ~/.ros/odin_ros_driver/calib.yaml
    launch_ROS1/
        odin1_ros1.launch       // ROS1 launch file
    launch_ROS2/
        odin1_ros2.launch.py    // ROS2 launch file
    script/
        build_ros.sh            // Legacy ROS1 build script
        build_ros2.sh           // Legacy ROS2 build script; see install.md
    test/
        sdk_usb_context_test.cpp // USB context regression test
    docs/                       // Runtime services and USB fix details
    install.md                  // Installation and build
    easyrun.md                  // Daily operations
    update.md                   // Update, verify, and push
    recorddata/                 // Runtime output; actual path depends on workspace
    log/                        // holds log files
        Driver_{timestamp}/     // holds all log folders for each time driver started
            Conn_{timestamp}/   // holds all log files for each odin1 device connection
                dev_status.csv  // device status log file
    README.md                   // Usage instructions
    CMakeLists.txt              // CMake build file
    LICENSE                     // License file
```
### 4.2 File structure
| Launch File Name         | Description |
|--------------------------|-------------|
| odin1_ros1.launch        | Launch file for ROS1 - Odin1 Basic Operations Demo |
| odin1_ros2.launch.py     | Launch file for ROS2 - Odin1 Basic Operations Demo |


### 4.3 ROS topics
Internal parameters of the Odin ROS driver are defined in config/control_command.yaml. Below are descriptions of the commonly used parameters:

| Topic               |control_command.yaml  | Detailed Description |
|---------------------|----------------------|----------------------|
| odin1/imu                     | sendimu           | Imu Topic |
| odin1/image                   | sendrgb           | RGB Camera Topic, decoded from original jpeg data from device, bgr8 format |
| odin1/image/undistorted         | sendrgbundistort  | undistorted RGB Camera Topic, processed with calib.yaml from device |
| odin1/image/compressed        | sendrgbcompressed | RGB Camera compressed Topic, original jpeg data from device |
| odin1/cloud_raw               | senddtof          | Raw_Cloud Topic |
| odin1/cloud_render            | sendcloudrender   | Render_Cloud Topic, processed with raw point cloud, rgb image, and calib.yaml from device |
| odin1/cloud_slam              | sendcloudslam     | Slam_PointCloud Topic |
| odin1/odometry                | sendodom          | Odom Topic |
| odin1/odometry_highfreq       | sendodom          | high frequency Odom Topic |
| odin1/path                    | showpath          | Odom Path Topic |
| tf                            | sendodom          | tf tree Topic |
| odin1/depth_img_competetion   | senddepth         | Dense depth image Topic. Demo, high computing power required. One-to-one with odin1/image/undistorted. To utilize the data please directly subscribe to this topic instead of echoing it. Original value is already depth data, no need for further convert. |
| odin1/depth_img_competetion_cloud  | senddepth         | Dense Depth_Cloud Topic. Demo, high computing power required |
| odin1/reprojected_image       | sendreprojection  | Reprojected cloud to image Topic. Projects cloud_slam to camera image using odometry. Processed on host device. |

### 4.4 Data format

1. The raw point cloud (cloud_raw) has the following fields:
```
float32 x             // X axis, in meters
float32 y             // Y axis, in meters
float32 z             // Z axis, in meters
uint8  intensity      // Reflectivity, range 0–255
uint16 confidence     // Point confidence, actual value range from 0 to around 1300 in typical scene, higher value means more reliable. Recommanded filtering threshold is 30-35, should be adjusted accordingly.
float32 offset_time   // Time offset relative to the base timestamp unit: s 
```

To work with this custom format in PCL, first define the point type:
```cpp
/*** LS ***/
namespace ls_ros {
    struct EIGEN_ALIGN16 Point {
        float x;
        float y;
        float z;
        uint8_t intensity;
        uint16_t confidence;
        float offset_time;
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    };
}  // namespace ls_ros

POINT_CLOUD_REGISTER_POINT_STRUCT(ls_ros::Point,
      (float, x, x)
      (float, y, y)
      (float, z, z)
      (uint8_t, intensity, intensity)
      (uint16_t, confidence, confidence)
      (float offset_time , offset_time)
)
```
Then, you can easily convert a ROS sensor_msgs::PointCloud2 message into a PCL point cloud:
```
pcl::PointCloud<ls_ros::Point> ls_cloud;
pcl::fromROSMsg(*msg, ls_cloud);
```

2. The slam point cloud (cloud_slam) and directly rendered point cloud (cloud_render) has the following fields:
```
float32 x             // X axis, in meters
float32 y             // Y axis, in meters
float32 z             // Z axis, in meters
float32 rgb           // RGB value
```

### 4.5 Other functionalities

|control_command.yaml   | Detailed Description |
|-----------------------|----------------------|
| use_host_ros_time     | Time synchronization mode: 0 - use odin internal system time as data timestamp (typical and recommended); 1 - use host ROS time upon receive (not recommended for most users); 2 - align odin1 time to host time via NTP-like synchronization, timestamp is the sensor data reception time on host time axis. |
| strict_usb3.0_check   | Strict USB3.0 check, if off, allow connection even if usb connection is below usb 3.0 |
| recorddata            | Record data in specific format that can be imported into MindCloud(TM) for post-processing. Please be aware that this will consume a lot of storage space. Testing shows 9.5G for 10mins of data. The per-frame timestamps written into the recorded files (IMU / image / point cloud / pose / rotate) follow the same alignment policy as `use_host_ros_time`, so under NTP mode (`use_host_ros_time=1` or `2`) the recorded timestamps are NTP-aligned host time instead of odin1 boot time. <br>录制文件 (IMU / 图像 / 点云 / Pose / Rotate) 中每帧的时间戳与 `use_host_ros_time` 采用相同对齐策略：在 NTP 模式 (`use_host_ros_time=1` 或 `2`) 下，录制时间戳为 NTP 对齐后的主机时间，而非 odin1 开机时间。 |
| devstatuslog          | Device status logging, currently save device status (soc temperature, cpu usage, ram usage, dtof sensor temp .etc) and data tx & rx rate to devstatus.csv under log folder. A new file will be created every time the driver is started. |
| showcamerapose        | Display Camera Pose and Field of View. |
| custom_map_mode        | Operation Modes: Mode 0 - Odometry mode: The map frame and odom frame share the same pose. Mode 1 - Mapping (with loop closure) mode: This mode supports map saving. Mode 2 - Relocalization mode: Requires specifying the absolute path to the map file. After successful relocalization, it will output the TF relationship between the map and odom frames.|
| custom_init_pos        | Initialization Position (currently unused). |
| relocalization_map_abs_path        | Absolute Path to Map File: Used for relocalization mode. |
| mapping_result_dest_dir and mapping_result_file_name| Path and Name for Saving Maps in Mapping Mode: If not specified, default values will be used. |

### 4.6 Runtime AE/AWB Tuning via ROS Service / 通过 ROS Service 在线调节 AE/AWB

The driver hosts four ROS services that let a side terminal tune the
camera's auto exposure (AE) and auto white balance (AWB) at runtime,
while the main data streams keep flowing. The same SDK call is shared
with the driver's main control path and serialised by an internal
mutex, so it is safe to invoke these services concurrently with normal
operation.

驱动启动后会注册 4 个 ROS Service，允许在不重启 driver 的前提下，从另一个终端动态调节
相机的自动曝光（AE）和自动白平衡（AWB）。底层 SDK 调用与驱动主控制路径共享同一把
互斥锁，因此可以与正常数据流并发调用。

**Service list / Service 一览**

| Service name | Type / 类型 | Purpose / 用途 |
|---|---|---|
| `/odin1/get_ae`  | `odin_ros_driver/srv/GetAe`  | Query current AE status / 查询当前 AE 状态 |
| `/odin1/get_awb` | `odin_ros_driver/srv/GetAwb` | Query current AWB status / 查询当前 AWB 状态 |
| `/odin1/set_ae`  | `odin_ros_driver/srv/SetAe`  | Set AE mode and (manual) exposure / gain / 设置 AE 模式和手动曝光/增益 |
| `/odin1/set_awb` | `odin_ros_driver/srv/SetAwb` | Set AWB mode and (manual) R/B gain / 设置 AWB 模式和手动 R/B 增益 |

#### 4.6.1 Request fields, ranges, physical meaning / 请求字段、范围与物理含义

**`SetAe.Request`**

| Field | Range / 范围 | Meaning / 含义 |
|---|---|---|
| `mode` | `0` (AUTO) or / 或 `1` (MANUAL) | `0` = device runs its own AE loop, the two floats below are ignored / 设备自动调 AE，下方参数被忽略<br>`1` = device locks AE and applies the provided values / 设备锁 AE 并应用提供的值 |
| `exposure_time` | `0.0001` ~ `0.033` s (manual only / 仅手动模式) | Sensor exposure time per frame. Longer = brighter but more motion blur / 每帧传感器曝光时间。越长越亮但运动模糊增大 |
| `gain` | `1.0` ~ `64.0` (manual only / 仅手动模式) | Analog gain. Higher = brighter output but worse SNR / 模拟增益。越大越亮但信噪比越差 |

**`SetAwb.Request`**

| Field | Range / 范围 | Meaning / 含义 |
|---|---|---|
| `mode`  | `0` (AUTO) or / 或 `1` (MANUAL) | `0` = device runs its own AWB loop / 设备自动 AWB<br>`1` = device locks AWB and applies provided gains / 设备锁定 AWB 并应用所给增益 |
| `rgain` | `0.1` ~ `4.0` (manual only / 仅手动模式) | R channel gain. Higher `rgain` vs `bgain` shifts the image warm (yellow/red) / R 通道增益，相对 bgain 越大，画面越偏暖 |
| `bgain` | `0.1` ~ `4.0` (manual only / 仅手动模式) | B channel gain. Higher `bgain` vs `rgain` shifts the image cool (blue) / B 通道增益，相对 rgain 越大，画面越偏冷 |

> Gr / Gb channels are fixed to 1.0 by the device and are not adjustable.
> Gr / Gb 通道被设备固定为 1.0，不可调节。

#### 4.6.2 Response fields / 响应字段

All four services return a `success` (bool) and `rc` (int32). Get
services additionally return the queried state.
4 个 Service 都返回 `success` (bool) 与 `rc` (int32)。Get 类还会返回查询到的状态字段。

**`GetAe.Response`**

| Field | Typical range / 典型范围 | Meaning / 含义 |
|---|---|---|
| `exposure_time` | `0.0001`~`0.033` s | Current exposure / 当前曝光时间 |
| `gain` | `1.0`~`64.0` | Current analog gain / 当前模拟增益 |
| `iso` | `100`~`6400` | Equivalent ISO / 等效 ISO |
| `brightness` | `0`~`255` | Average frame brightness / 平均帧亮度 |
| `is_converged` | `0` or `1` | `1` = AE settled / AE 已收敛 |
| `env_lv` | `0`~`15` | Ambient luminance index, higher = brighter / 环境光强度指数，越大越亮 |
| `fps` | `~10` / `~14.5` / `~29` | Current frame rate / 当前帧率 |

**`GetAwb.Response`**

| Field | Typical range / 典型范围 | Meaning / 含义 |
|---|---|---|
| `rgain` / `bgain` | `0.1`~`4.0` | R / B channel gain / R / B 通道增益 |
| `grgain` / `gbgain` | `1.0` (fixed / 固定) | Gr / Gb gain, device-fixed / Gr / Gb 增益，设备固定 |
| `cct` | `2500`~`8000` K | Correlated color temperature / 相关色温 |
| `ccri` | `-50`~`50` | Color temp deviation index, 0 = on Planckian locus / 色温偏离指数，0 表示在普朗克轨迹上 |
| `is_converged` | `0` or `1` | `1` = AWB settled / AWB 已收敛 |

#### 4.6.3 `rc` return code / `rc` 返回码

| `rc` | Meaning / 含义 |
|---|---|
| `0` | Success / 成功 |
| `400` | Device payload too short / 设备载荷过短 |
| `401` | Device opcode not supported / 设备不支持该 opcode |
| `402` | Device parameter length wrong / 参数长度错误 |
| `403` | **Parameter out of range** / 参数越界 — most common when manual values exceed the table above / 手动值超出上表范围时最常见 |
| `404` | Device-side socket error / 设备端 socket 错误 |
| `405` | Device-side `ae_control` did not respond / 设备端 `ae_control` 无应答（确认 lydapp 已运行） |
| `255` (`0xFF`) | Unknown opcode reported by ae_control / ae_control 报未知 opcode |
| `-1` | SDK not initialised / SDK 未初始化 |
| `-2` ~ `-5` | USB transfer / timeout / malformed reply / USB 传输异常、超时、应答畸形 |
| `-100` | **Driver has not opened the device yet** / driver 还未打开设备，请等设备连接成功 |

#### 4.6.4 Usage examples / 调用示例

ROS2 (Humble) — start the driver in one terminal, then in a side terminal:
ROS2（Humble）—— 在一个终端启动 driver，在另一个终端：

```bash
source install/setup.bash

# Query current state / 查询当前状态
ros2 service call /odin1/get_ae  odin_ros_driver/srv/GetAe
ros2 service call /odin1/get_awb odin_ros_driver/srv/GetAwb

# Set AE to AUTO / 设置 AE 为自动
ros2 service call /odin1/set_ae odin_ros_driver/srv/SetAe "{mode: 0}"

# Set AE to MANUAL with 10 ms exposure and gain 4.0
# 设置 AE 为手动，10 毫秒曝光，增益 4.0
ros2 service call /odin1/set_ae odin_ros_driver/srv/SetAe \
  "{mode: 1, exposure_time: 0.010, gain: 4.0}"

# Set AWB to MANUAL with rgain=1.5, bgain=2.0
# 设置 AWB 为手动，rgain=1.5、bgain=2.0
ros2 service call /odin1/set_awb odin_ros_driver/srv/SetAwb \
  "{mode: 1, rgain: 1.5, bgain: 2.0}"

# Restore AUTO / 一键回自动
ros2 service call /odin1/set_ae  odin_ros_driver/srv/SetAe  "{mode: 0}"
ros2 service call /odin1/set_awb odin_ros_driver/srv/SetAwb "{mode: 0}"

# Inspect srv definition / 查看 srv 完整定义
ros2 interface show odin_ros_driver/srv/SetAe
```

ROS1 (Noetic) — start the driver, then in a side terminal:
ROS1（Noetic）—— 启动 driver 后，新开终端：

```bash
source devel/setup.bash

# Query / 查询
rosservice call /odin1/get_ae
rosservice call /odin1/get_awb

# Set AE manual / 设置 AE 手动
rosservice call /odin1/set_ae  "{mode: 1, exposure_time: 0.010, gain: 4.0}"

# Set AWB manual / 设置 AWB 手动
rosservice call /odin1/set_awb "{mode: 1, rgain: 1.5, bgain: 2.0}"

# Restore AUTO (ROS1 requires all fields to be present)
# 一键回自动（ROS1 要求填齐全部字段）
rosservice call /odin1/set_ae  "{mode: 0, exposure_time: 0.0, gain: 0.0}"
rosservice call /odin1/set_awb "{mode: 0, rgain: 0.0, bgain: 0.0}"

# Inspect srv definition / 查看 srv 完整定义
rossrv show odin_ros_driver/SetAe
```

#### 4.6.5 Recommended starting points by scene / 不同场景推荐起步参数

**AE (`exposure_time`, `gain`)**

| Scene / 场景 | `exposure_time` | `gain` |
|---|---|---|
| Bright outdoor / 明亮室外 | `0.001` ~ `0.005` s | `1.0` ~ `2.0` |
| Normal indoor / 普通室内 | `0.008` ~ `0.015` s | `2.0` ~ `8.0` |
| Dim light / 暗光环境 | `0.020` ~ `0.030` s | `8.0` ~ `32.0` |
| Very dark / 极暗 | `0.033` s | `32.0` ~ `64.0` |

**AWB (`rgain`, `bgain`)**

| Target tone / 目标色调 | `rgain` | `bgain` |
|---|---|---|
| Warm (tungsten, sunset) / 暖（钨丝灯、夕阳） | `2.0` ~ `2.5` | `1.0` ~ `1.2` |
| Neutral (D65 daylight) / 中性（D65 日光） | `1.5` ~ `1.7` | `1.8` ~ `2.0` |
| Cool (cloudy, fluorescent) / 冷（阴天、荧光） | `1.2` ~ `1.4` | `2.2` ~ `2.6` |
| Very cool / 极冷 | `1.0` | `3.0` ~ `4.0` |

#### 4.6.6 Caveats / 注意事项

- The service blocks for up to ~10 s waiting for the device to reply;
  typical latency is tens of milliseconds.
  Service 最长阻塞约 10 秒等设备应答；正常几十毫秒返回。
- Manual mode is **not** persisted across driver / device restart;
  it falls back to AUTO on each new connection.
  手动模式**不会**跨重启保留；每次重连默认回到 AUTO。
- `rc = -100` means the driver has not yet opened the device.
  Wait until the driver logs `device connected` before calling.
  返回 `rc = -100` 表示 driver 还没打开设备，等到 driver 日志显示 `device connected` 再调用。
- The effective maximum `exposure_time` is bounded by the frame
  period `1 / fps`. With `dtof_fps = 290` (29 Hz, period ~34 ms)
  the upper limit 0.033 s is already at the frame boundary.
  最大可用 `exposure_time` 受帧周期 `1/fps` 限制。在 `dtof_fps = 290`（29 Hz、周期 ~34 ms）下，上限 0.033 s 已经贴到帧边界。

## 4.7 Runtime Device Control / 运行时设备控制

In addition to the AE/AWB services above, the driver exposes several runtime ROS services and supports the `set_param.sh` command-file interface for operations such as saving maps, resetting the algorithm, and retrieving device logs.

除上述 AE/AWB 服务外，driver 还暴露了若干运行时 ROS Service，并支持 `set_param.sh` 命令文件接口，用于保存地图、复位算法、拉取设备日志等操作。

See [docs/runtime_service_operations.md](docs/runtime_service_operations.md) for the full list of services, `set_param.sh` usage, request/response fields and examples.

完整服务列表、`set_param.sh` 用法、请求/响应字段及调用示例见 [docs/runtime_service_operations.md](docs/runtime_service_operations.md)。

## 5. FAQ
### 5.1 Segmentation fault upon re-launching host SDK

If the process exits with signal 11 immediately after `device attach`, and GDB
shows `libusb_get_next_timeout` called from
`usb_data_xfer::event_monitor_routine`, see
[USB startup crash](docs/usb_startup_crash.md). The driver includes a context
adapter for this issue in the bundled static SDK; rebuild the driver to apply it.

**Error Message**  
No device connected after 60 seconds 

**Solution**  
1. Please power on Odin module again # Disconnect and reconnect odin power

2. Reinitialize Odin SDK # Execute SDK after device reboot


### 5.2 SDK library not found / SDK 库找不到

本仓库的 SDK 位于 `lib/`，ARM 使用 `liblydHostApi_arm.a`，
x86_64 使用 `liblydHostApi_amd.a`。先检查架构、文件和 CMake 缓存：

```bash
uname -m
ls -lh ~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver/lib/
grep -E 'LYD_HOST_API_LIB|CMAKE_HOME_DIRECTORY' ~/longxiaoze_ws/build/odin_ros_driver/CMakeCache.txt
```

移动过源码或更换架构后，从工作空间根目录重建：

```bash
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
colcon build --packages-select odin_ros_driver --cmake-clean-cache \
  --cmake-args -DBUILD_SYSTEM=ROS2 -DBUILD_TESTING=ON
```

依赖及 OpenCV 运行库缺失的排查见 [install.md](install.md)。

### 5.3 Docker GUI passthrough failure

**Error Message**  
Unable to open X display or No protocol specified

**Resolution** 
```shell
xhost + #This command enables graphical passthrough to Docker containers
```

### 5.4 ROS driver exit with get version failed error

**Error Message**  
```shell
<ERROR><api.cpp:lidar_get_version:672>: get device version fail.
get version failed.
```

**Resolution** 

Device firmware version is too low, please update to latest version.


### 5.5 RVIZ has not responded for a long time

**Error Message**  
Rviz does not respond, and after a while the terminal prints Device disconnected, waiting for reconnection...

**Resolution** 

Please power on Odin module again

### 5.6 Device not responding

**Error Message**  
Missed ok response from device,probably wrong interaction procedure.

**Resolution** 

Please adopt the solution mentioned in 5.1

### 5.7 Device has no external calibration file 

**Error Message**  
ERROR：Missing camera node 'cam_0'

**Resolution** 

Please plug and unplug the USB again

### 5.8 ROS Driver report device disconnected immediately after stream started

**Error Message**  

```shell
Device ready and streams activated
Device detaching...
Wating for device reconnection...
Device disconnected, waiting for reconnection...
```

**Reason**

Mostly common on ros2 environment and connected to complex network environment, such as office wifi & ethernet. ROS2 default to broadcast, and complex network environment will cause ros2 publish to block, leading to device disconnection.

**Resolution** 

If cross-device communication is not required, please restrict ros2 to localhost only with:
```shell
export ROS_LOCALHOST_ONLY=1
```

If cross-device communication is required, please simplify the network environment as much as possible. Mini local network with only required devices is recommended.

### 5.9 ROS Driver died immediately after stream started

**Error Message**  

```shell
Device ready and streams activated
[host_sdk_sample-2] process has died ......
```

**Test**

Disable odin1/image	with sendrgb = 0 in control_command.yaml and try again. If the driver now works, it is likely that the issue is related to multiple version of opencv is installed on the system.

**Resolution** 

Check the linked OpenCV and cv_bridge versions with `ldd`; see [install.md](install.md) and section 5.16. Match their build versions before rebuilding the driver.

### 5.10 ROS Driver printing "TF_OLD_DATA ignoring data" warning

**Error Message**  

```shell
[rviz2-3] Warning: TF_OLD_DATA ignoring data from the past for frame odin1_base_link at time 20.547632 according to authority Authority undetectable
[rviz2-3] Possible reasons are listed at http://wiki.ros.org/tf/Errors%20explained
[rviz2-3]          at line 294 in ./src/buffer_core.cpp
```

**Reason**

This is a ros & rviz feature to warn user that some tf data is being ignored due to timestamp conflicts. It happens when user keeps ros driver running and power-cycles odin device, which cause odin's internal system time being reset and now data timestamps conflicts with old data recieved by rviz during last run.

**Resolution** 

There's a reset button on bottom of rviz gui. Click on this button will reset rviz's internal state and stop the warning.

### 5.11 ROS Driver printing "unknown cmd code: xx" error

**Error Message**  

```shell
<ERROR><api.cpp:cmd_data_deal:418>: unknow command code 21.
```

**Reason**

This is due to ros driver version mismatch with device firmware version, resulting in ros driver unable to decode new data added in newer firmware.

**Resolution** 

Please make sure you are using most up-to-date ros driver and device firmware.

### 5.12 USB device access error (LIBUSB_ERROR_BUSY or LIBUSB_ERROR_ACCESS)

**Error Message**  

```shell
libusb: error [udev_hotplug_event] ignoring udev action bind
LIBUSB_ERROR_BUSY
```

or

```shell
libusb: error [_get_usbfs_fd] libusb couldn't open USB device /dev/bus/usb/xxx/xxx, errno=13
LIBUSB_ERROR_ACCESS
```

**Reason**

- **LIBUSB_ERROR_BUSY**: Another process is already using the USB device. This commonly happens when multiple instances of the ROS driver are running, or another application (such as a previous crashed instance) still holds the device handle.

- **LIBUSB_ERROR_ACCESS**: The current user does not have permission to access the USB device. This is typically caused by missing udev rules or insufficient user privileges.

**Resolution** 

For **LIBUSB_ERROR_BUSY**:

1. Check if another instance of the driver is running:
```shell
ps aux | grep host_sdk_sample
```

2. Kill any existing instances:
```shell
killall host_sdk_sample
```

3. If the issue persists, unplug and replug the USB device to reset the device state.

For **LIBUSB_ERROR_ACCESS**:

1. Add udev rules for the device. Create a file `/etc/udev/rules.d/99-odin.rules` with the following content:
```shell
SUBSYSTEM=="usb", ATTR{idVendor}=="2207", ATTR{idProduct}=="0019", MODE="0666", GROUP="plugdev"
```

2. Reload udev rules:
```shell
sudo udevadm control --reload-rules
sudo udevadm trigger
```

3. Alternatively, run the driver with sudo (not recommended for production):
```shell
sudo -E ros2 launch odin_ros_driver odin_ros_driver.launch.py
```

4. Make sure your user is in the `plugdev` group:
```shell
sudo usermod -aG plugdev $USER
```
Then log out and log back in for the group change to take effect.

### 5.13 ros2 bag drops high-frequency topics (IMU / odometry_highfreq) / ros2 bag 录制丢失高频话题（IMU / odometry_highfreq）

**Symptom / 现象**

When recording with `ros2 bag record`, low-frequency topics (cloud, image, odometry, wiwc) are intact, but `/odin1/imu` (400 Hz) and `/odin1/odometry_highfreq` (400 Hz) show missing samples — analysis scripts report inter-message intervals that are 2× or more of the expected period, while no drop is reported on the SDK side or by an online subscriber such as `ros2 topic hz`.

使用 `ros2 bag record` 录制时，低频话题（cloud、image、odometry、wiwc）完整无丢，但 `/odin1/imu`（400 Hz）和 `/odin1/odometry_highfreq`（400 Hz）会出现丢帧——分析脚本上看到消息间隔达到正常周期的 2 倍以上，而 SDK 侧不报丢，独立的 `ros2 topic hz` 订阅者也看不到丢。

**Reason / 原因**

The driver publishes `/odin1/imu` and `/odin1/odometry_highfreq` with `RELIABLE` QoS. By default `ros2 bag record` subscribes with `history = keep_last`, `depth = 10`, which only buffers ~25 ms of samples at 400 Hz. Whenever the recorder is briefly delayed (disk flush, mcap/sqlite chunk write, scheduler jitter), its subscription queue overflows and DDS silently drops the oldest samples on the **subscriber side**. The SDK and publisher are unaffected, which is why no drop appears in the driver logs or in `ros2 topic hz`.

驱动以 `RELIABLE` QoS 发布 `/odin1/imu` 与 `/odin1/odometry_highfreq`。`ros2 bag record` 默认订阅使用 `history = keep_last`、`depth = 10`，在 400 Hz 下只能缓冲约 25 ms。一旦录制端有短暂阻塞（落盘 flush、mcap/sqlite chunk 写入、调度抖动），订阅队列就会溢出，DDS 在**订阅端**静默丢掉最旧的样本。SDK 与 publisher 不受影响，因此驱动日志和 `ros2 topic hz` 都看不到丢。

**Resolution / 解决方案**

Use the provided QoS override file `script/rosbag2_qos.yaml` to raise the subscriber-side queue depth on the recorder for the two high-rate topics:

使用本仓库提供的 QoS 配置 `script/rosbag2_qos.yaml`，把高频话题的录制订阅 depth 拉大：

```yaml
# script/rosbag2_qos.yaml
/odin1/imu:
  reliability: reliable
  history: keep_last
  depth: 4000

/odin1/odometry_highfreq:
  reliability: reliable
  history: keep_last
  depth: 4000
```

Apply it when recording / 录制时通过 `--qos-profile-overrides-path` 应用：

```shell
ros2 bag record -a \
    --qos-profile-overrides-path src/odin_ros_driver/script/rosbag2_qos.yaml \
    -o my_bag
```

Or only the high-rate topics / 也可以只录制高频话题：

```shell
ros2 bag record \
    --qos-profile-overrides-path src/odin_ros_driver/script/rosbag2_qos.yaml \
    -o my_bag \
    /odin1/imu /odin1/odometry_highfreq /odin1/odometry /odin1/wiwc /odin1/cloud_raw
```

**Optional further tuning / 可选的进一步优化**

If drops still occur after applying the override (typically on slower disks), try the following in addition / 套用上述 override 后仍有丢包时（通常发生在慢盘上），可叠加以下措施：

```shell
# Use mcap backend with a larger internal cache (faster than sqlite3).
# 使用 mcap 后端 + 更大的内部缓存（比 sqlite3 快）。
ros2 bag record -s mcap --max-cache-size 1073741824 \
    --qos-profile-overrides-path src/odin_ros_driver/script/rosbag2_qos.yaml \
    -o my_bag \
    /odin1/imu /odin1/odometry_highfreq ...

# Enlarge kernel UDP socket buffers (the most common hidden bottleneck for
# 400 Hz RELIABLE traffic, default is only 208 KB).
# 放大内核 UDP socket buffer（400 Hz RELIABLE 流量最常见的隐藏瓶颈，默认仅 208 KB）。
sudo sysctl -w net.core.rmem_max=33554432
sudo sysctl -w net.core.wmem_max=33554432
```

**Does ROS1 have the same problem? / ROS1 是否存在同样的问题？**

No. ROS1 uses TCP-based publish/subscribe with a single `queue_size` parameter on each side, and has no QoS profile mismatch between publisher and subscriber. The ROS1 publisher path in this driver already sizes the IMU and `odometry_highfreq` publishers to `queue_size = 4000` (`include/host_sdk_sample.h`, see `initialize_publishers` ROS1 branch), and `rosbag record` uses TCP transport which is reliable by construction. As a result this specific drop pattern does not occur under ROS1; no additional configuration is required.

不存在。ROS1 使用基于 TCP 的发布/订阅，发布端与订阅端各自只有一个 `queue_size` 参数，不存在 ROS2 那种 QoS profile 不匹配的问题。本驱动 ROS1 路径已经把 IMU 与 `odometry_highfreq` 的发布队列设置为 `queue_size = 4000`（见 `include/host_sdk_sample.h` 中 `initialize_publishers` 的 ROS1 分支），并且 `rosbag record` 使用 TCP 传输本身即可靠传递。因此在 ROS1 下不会出现该丢帧现象，也不需要额外配置。

### 5.14 On Ubuntu 24.04 (ROS2 Jazzy) point clouds / TF fail to transform in RViz / Ubuntu 24.04（ROS2 Jazzy）下 RViz 点云 / TF 无法转换

**Symptom / 现象**

On Ubuntu 24.04 (ROS2 Jazzy), when the RViz **Fixed Frame** is set to `odom`, `cloud_raw` (in the `lidar` frame) fails to display and RViz reports a transform / extrapolation error for the `odom -> imu -> lidar` chain. On Ubuntu 22.04 (Humble) the same setup works.

在 Ubuntu 24.04（ROS2 Jazzy）下，当 RViz 的 **Fixed Frame** 设为 `odom` 时，`cloud_raw`（`lidar` 系）无法显示，RViz 对 `odom -> imu -> lidar` 链条报 transform / 外推（extrapolation）错误。相同配置在 Ubuntu 22.04（Humble）下却正常。

**Reason / 原因**

The device point-cloud timestamp is typically ~100 ms ahead of the odometry timestamp. ROS2 Jazzy's tf2 performs a strict timestamp lookup and will not extrapolate, so when the TF is published only at the odometry rate (and only at the odometry timestamp), tf2 cannot find a transform bracketing the cloud timestamp and the lookup fails.

设备点云时间戳通常比里程计时间戳超前约 100 ms。ROS2 Jazzy 的 tf2 采用严格的时间戳查找且不做外推（extrapolation），因此当 TF 只在里程计频率、且只按里程计时间戳发布时，tf2 找不到能覆盖点云时间戳的变换，查找失败。

**Resolution / 解决方案**

Set `tf_extra_publish_rate` to a value greater than `0` (e.g. `100`) in `config/control_command.yaml`. This starts an extra timer that keeps re-publishing the `odom -> imu` and `imu -> lidar` transforms at the configured rate with advancing timestamps, so tf2 always has a transform covering the cloud timestamp.

在 `config/control_command.yaml` 中把 `tf_extra_publish_rate` 设为大于 `0` 的值（例如 `100`）。它会启动一个额外的定时器，按设定频率、以推进的时间戳持续补发 `odom -> imu` 与 `imu -> lidar` 变换，使 tf2 始终有覆盖点云时间戳的变换。

```yaml
# config/control_command.yaml
tf_extra_publish_rate: 100  # 0: off; >0: rate in Hz (recommended on ROS2 Jazzy / Ubuntu 24.04)
```

On Ubuntu 22.04 (Humble) or ROS1 this is usually not required and the parameter can be left at `0`.

在 Ubuntu 22.04（Humble）或 ROS1 下通常不需要该配置，参数可保持为 `0`。

### 5.15 USB "transfer no mem" / "transfer error" on low-performance hosts / 低性能主机上出现 "transfer no mem" / "transfer error"

**Symptom / 现象**

```shell
<ERROR>: submit x transfer fail LIBUSB_ERROR_NO_MEM.
<ERROR>: transfer error: LIBUSB_TRANSFER_ERROR.
```

The driver runs fine on a powerful PC but, when deployed on a lower-performance board (e.g. an embedded ARM host), after a while the SDK starts reporting `LIBUSB_ERROR_NO_MEM` on send, followed by a cascade of `transfer error`. The module's sending rate is constant; the only thing that changed is the host the SDK runs on.

驱动在性能强的 PC 上运行正常，但部署到性能较弱的板子（例如嵌入式 ARM 主机）上跑一段时间后，SDK 发送时开始报 `LIBUSB_ERROR_NO_MEM`，随后接连出现 `transfer error`。模组的发送速率是恒定的，唯一改变的条件是 SDK 运行在哪台机器上。

**Reason / 原因**

libusb async transfers queued to the kernel consume a limited per-device usbfs memory pool (`/sys/module/usbcore/parameters/usbfs_memory_mb`, default **16 MB**). On a slow host the single libusb event thread reaps completed transfers more slowly than new ones are submitted, so outstanding OUT (send) transfers pile up in the usbfs pool until submission fails with `LIBUSB_ERROR_NO_MEM`. Once submission starts failing, subsequent transfers report `LIBUSB_TRANSFER_ERROR`.

libusb 异步传输提交到内核后，会占用每个设备有限的 usbfs 内存池（`/sys/module/usbcore/parameters/usbfs_memory_mb`，默认 **16 MB**）。在慢主机上，单一的 libusb 事件线程回收已完成传输的速度跟不上新传输的提交速度，于是在途的 OUT（发送）传输在 usbfs 池中不断堆积，直到提交失败并返回 `LIBUSB_ERROR_NO_MEM`。一旦提交开始失败，后续传输就会报 `LIBUSB_TRANSFER_ERROR`。

**Resolution / 解决方案**

1. Raise the usbfs memory limit at runtime (takes effect immediately, resets on reboot) / 在运行时调大 usbfs 内存上限（立即生效，重启后失效）：

```shell
# Check current value / 查看当前值 (default 16)
cat /sys/module/usbcore/parameters/usbfs_memory_mb

# Raise to 128 MB / 调大到 128 MB
echo 128 | sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb
```

2. Make it persistent across reboots / 让配置在重启后仍生效（二选一）：

```shell
# Option A: kernel boot parameter (edit GRUB / board bootargs)
# 方式 A：内核启动参数（编辑 GRUB / 板子的 bootargs）
usbcore.usbfs_memory_mb=128

# Option B: modprobe config
# 方式 B：modprobe 配置
echo "options usbcore usbfs_memory_mb=128" | sudo tee /etc/modprobe.d/usbcore.conf
```

**Note / 说明**

Raising `usbfs_memory_mb` is a mitigation that increases headroom. The SDK USB layer already bounds the number of in-flight OUT transfers (flow control) and releases transfer buffers on every terminal state to prevent the memory leaks that previously turned a transient error into a runaway. Keeping a larger usbfs pool (e.g. 128 MB) is still recommended on slow hosts or when transferring large files (e.g. relocalization maps).

调大 `usbfs_memory_mb` 属于提高余量的缓解措施。SDK 的 USB 层已经对在途 OUT 传输数量做了上限限制（流控），并在每个终止状态释放传输缓冲，避免了此前"瞬时错误演变为不可恢复的内存泄漏雪崩"的问题。在慢主机上或传输大文件（如重定位地图）时，仍建议保留较大的 usbfs 池（例如 128 MB）。

### 5.16 OpenCV / cv_bridge 版本与链接路径

当前 `CMakeLists.txt` 使用 `find_package(OpenCV REQUIRED)`，
没有自动从 `cv_bridge-extras.cmake` 读取版本并锁定 OpenCV 的实现。
若需指定 OpenCV，需要通过 `OpenCV_DIR` 明确传入对应 CMake 配置目录，
并检查 `cv_bridge` 的实际链接依赖。

本机实际路径为：

| 组件 | 路径 / 版本 |
| --- | --- |
| 驱动 OpenCV 配置 | `/usr/lib/cmake/opencv4`，4.8.0 |
| 驱动 OpenCV 核心库 | `/usr/lib/libopencv_core.so.4.8.0` |
| 系统 cv_bridge | `/opt/ros/humble/lib/libcv_bridge.so` |
| cv_bridge 引用的 OpenCV 核心库 | `/usr/lib/aarch64-linux-gnu/libopencv_core.so.4.5.4d` |

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ldd ~/longxiaoze_ws/install/odin_ros_driver/lib/odin_ros_driver/host_sdk_sample \
  | grep -E 'cv_bridge|opencv|libtbb|not found'
```

本机存在 OpenCV/TBB 多版本链接提示；短时实测已收到图像并正常退出。
若出现首帧图像崩溃，应核对并统一驱动与 `cv_bridge` 的 OpenCV 构建版本。
设备连接后立即发生、堆栈位于 `libusb_get_next_timeout` 的崩溃，
对应 [USB 上下文修复](docs/usb_startup_crash.md)。

终端历史中的 Jetson OpenCV 运行库补装方法和检查命令见 [install.md](install.md)。

## 6.  Contact Information​​

You can contact our support through support@manifoldtech.cn

To help diagnose the issue, please provide the following details to our FAE engineer:

1. Current firmware version​​ 
```shell
[device_version_capture]: ros_driver_version: [Version Number]
```
2. Photos of power adapter and converter cable​​ in use.

3. Does the issue happen occasionally or consistently?

4. Provide images of the problem scenario.

5. Did the troubleshooting methods in ​​Section V​​ resolve the issue?

6. Expected timeline for issue resolution.
