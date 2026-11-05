# 快速运行

适用于已按 [install.md](install.md) 完成编译的 ROS 2 Humble 环境。
本机工作空间为 `/home/techshare/longxiaoze_ws`，源码目录为
`~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver`。

## 1. 启动驱动与 RViz

连接设备电源及 USB 3.0 数据线，在终端执行：

```bash
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
source install/setup.bash
ros2 launch odin_ros_driver odin1_ros2.launch.py
```

正常连接会出现：

```text
Hardware connected, starting software connection...
Software connection successful in ... seconds
Device ready and streams activated
```

默认 RViz 配置为 `config/odin_ros2.rviz`，`Fixed Frame` 为 `odom`。
在启动终端按 **Ctrl+C** 退出；修复后的实测日志为：

```text
sdk deinit complete
[INFO] [host_sdk_sample-1]: process has finished cleanly
```

## 2. 只运行主驱动

无图形界面或只需要 ROS 话题时：

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ros2 run odin_ros_driver host_sdk_sample
```

此命令仅启动主驱动。深度补全、重投影和叠加等独立演示节点由完整 launch 启动。

## 3. 在第二个终端检查数据

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ros2 topic list
ros2 service list
```

以下频率命令每次运行一条，按 Ctrl+C 结束后再检查下一项：

```bash
ros2 topic hz /odin1/imu
ros2 topic hz /odin1/image
ros2 topic hz /odin1/cloud_raw
ros2 topic hz /odin1/cloud_slam
ros2 topic hz /odin1/odometry
```

| 话题 | 数据 | 配置开关 |
| --- | --- | --- |
| `/odin1/imu` | IMU | `sendimu` |
| `/odin1/image` | BGR 图像 | `sendrgb` |
| `/odin1/image/compressed` | 原始压缩图像 | `sendrgbcompressed` |
| `/odin1/image/undistorted` | 去畸变图像 | `sendrgbundistort`，依赖 `sendrgb` |
| `/odin1/cloud_raw` | 原始点云 | `senddtof` |
| `/odin1/cloud_slam` | SLAM 点云 | `sendcloudslam` |
| `/odin1/cloud_render` | 主机着色点云 | `sendcloudrender` |
| `/odin1/odometry` | 里程计 | `sendodom` |
| `/odin1/odometry_highfreq` | 高频里程计 | `sendodom` |

2026-11-05 完整 launch 实测中，25 秒观察窗口内收到以上五项主要数据，
并完成了 Ctrl+C 正常退出。具体计数见 [USB 修复验证记录](docs/usb_startup_crash.md)。

## 4. 修改配置

编辑源码中的配置文件：

```bash
nano ~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver/config/control_command.yaml
```

本机主要配置如下，均位于 `register_keys:` 下：

```yaml
register_keys:
  use_host_ros_time: 0
  streamctrl: 1
  sendrgb: 1
  sendrgbcompressed: 1
  sendrgbundistort: 0
  sendimu: 1
  enable_imu_smooth: 0
  sendodom: 1
  send_odom_baselink_tf: 1
  senddtof: 1
  sendcloudslam: 1
  sendcloudrender: 1
  senddepth: 0
  sendreprojection: 0
  sendoverlay: 0
  custom_map_mode: 1
```

这是关键字段摘要，修改时保留原文件的其他字段。
`use_host_ros_time: 0` 使用设备时间戳。

修改配置后停止驱动，重新执行以下命令，再按第 1 节启动：

```bash
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
colcon build --packages-select odin_ros_driver --cmake-args -DBUILD_SYSTEM=ROS2 -DBUILD_TESTING=ON
source install/setup.bash
```

主驱动在此源码工作空间内读取源码配置；辅助节点从安装目录读取配置。
重新构建会同步安装副本。当前主驱动没有使用 launch 传入的 `config_file`
参数选择配置文件，因此这里直接编辑上述文件。

`senddepth: 0`、`sendreprojection: 0` 时，对应节点启动后正常退出属于预期行为。
当前 launch 即使设置 `sendoverlay: 0` 仍会创建图像叠加节点。

## 5. 建图与保存地图

`custom_map_mode` 含义：

| 值 | 模式 |
| --- | --- |
| `0` | 里程计模式 |
| `1` | SLAM 建图模式，支持保存地图 |
| `2` | 重定位模式，需设置 `relocalization_map_abs_path` |

在模式 `1` 下，设备就绪后，在第二个终端执行：

```bash
cd ~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver
./set_param.sh save_map 1
```

也可以使用 ROS 服务：

```bash
ros2 service call /odin1/save_map odin_ros_driver/srv/SaveMap '{value: 1}'
```

脚本输出 `Command sent` 表示已写入 `/tmp/odin_command.txt`，保存是否完成需查看
驱动日志。服务返回成功也表示请求已接受，地图仍由后台传输。
输出目录和文件名由 `mapping_result_dest_dir`、`mapping_result_file_name` 控制。
连续保存应等待上一次传输完成。

完整服务说明见 [运行时操作](docs/runtime_service_operations.md)，
重定位说明见 [RELOCALIZATION_GUIDE.md](RELOCALIZATION_GUIDE.md)。

## 6. 日志、标定与数据路径

| 内容 | 本机默认路径 |
| --- | --- |
| ROS launch 日志 | `~/.ros/log/` |
| 主标定文件 | `~/.ros/odin_ros_driver/calib.yaml` |
| 设备状态日志 | `~/longxiaoze_ws/src/odin_ros_driver/log/Driver_*/Conn_*/dev_status.csv` |
| 默认地图目录 | `~/longxiaoze_ws/src/odin_ros_driver/map/<启动时间>/` |
| 开启 `recorddata` 后的记录目录 | `~/longxiaoze_ws/src/odin_ros_driver/recorddata/` |
| 命令文件 | `/tmp/odin_command.txt` |

当前代码的日志、地图和记录路径保留了 `src/odin_ros_driver` 这一目录名，
与 Git 源码目录 `src/longxiaoze-robot-driver-odin_ros_driver` 不同。
实际生成位置以启动日志为准，地图路径也可通过配置明确指定。

标定主目录可由 `ODIN_CALIB_DIR` 覆盖，其次使用 `$ROS_HOME/odin_ros_driver`，
再其次使用 `~/.ros/odin_ros_driver`。如需覆盖，在启动完整 launch 前设置环境变量。

## 7. 已知提示

- `lidar_set_rgb_parameter` 超时：当前设备仍出现此提示；已实测收到图像，
  请求的 RGB 分辨率和帧率是否应用尚未验证。
- `Failed to set SCHED_FIFO/SCHED_RR`：本机使用默认调度器继续运行。
- 连接即出现 `exit code -11`：确认已经重新编译包含
  `src/sdk_usb_context.cpp` 的版本，参考 [USB 修复说明](docs/usb_startup_crash.md)。
- OpenCV/TBB 链接版本提示：排查步骤见 [install.md](install.md)。
