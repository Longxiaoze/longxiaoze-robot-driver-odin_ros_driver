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

## 2. ros2 bag 录制

``` bash
ros2 bag record /odin1/cloud_raw /odin1/imu /odin1/odometry /odin1/tf /odin1/image/compressed /odin1/cloud_slam --qos-profile-overrides-path /home/techshare/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver/rosbag2_qos.yaml
```

## 3. 仅用 RViz 查看 bag

`odin1_rviz.launch.py` 启动 RViz 和一个仅生成可视化轨迹的轻量节点，
不连接设备，也不启动深度补全、重投影或叠加节点。
默认加载 `config/odin_bag_ros2.rviz`，固定坐标系为 `odom`。
默认显示压缩图像、当前彩色 SLAM 点云、最近 10 秒的点云累积和绿色连续轨迹线；
不显示位姿坐标轴、方向箭头或 TF 坐标系。
轨迹节点从 `/odin1/odometry` 生成 `/odin1/visualization_path`（`nav_msgs/Path`），
最多保留 10000 个位姿，不依赖本次录制命令中未包含的 `/odin1/path`。
回放时间戳倒退或坐标系改变时会清空旧轨迹，避免连接两次回放。
IMU 数据仍保存在 bag 中，此配置不提供独立 IMU 显示。

首次使用，安装压缩图像显示插件并重新构建，以安装新增的 launch 和 RViz 配置：

```bash
sudo apt install ros-humble-compressed-image-transport
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
colcon build --packages-select odin_ros_driver --cmake-args -DBUILD_SYSTEM=ROS2
source install/setup.bash
ros2 launch odin_ros_driver odin1_rviz.launch.py
```

在另一个终端回放，替换为实际 bag 目录：

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ros2 bag play /绝对路径/bag目录 --clock
```

按第 2 节的固定录制格式，回放使用 `/odin1/cloud_slam`、
`/odin1/image/compressed` 和 `/odin1/odometry`。
SLAM 点云和生成的轨迹均在 `odom` 坐标系下显示，`Raw Cloud` 保持默认关闭。
此可视化流程不依赖 TF，无需设置 `tf_topic`。

启动 RViz 时使用上面的默认命令，`use_sim_time` 保持默认值 `true`，
由 `ros2 bag play` 的 `--clock` 参数提供回放时钟。

## 4. 建图与保存地图

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
