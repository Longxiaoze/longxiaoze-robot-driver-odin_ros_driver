# 更新、验证与推送

本仓库地址为
[Longxiaoze/longxiaoze-robot-driver-odin_ros_driver](https://github.com/Longxiaoze/longxiaoze-robot-driver-odin_ros_driver)，
当前工作分支为 `main`。日常运行见 [easyrun.md](easyrun.md)。

## 1. 本次修复与文档更新

2026-11-05 的改动包括：

- 新增 `src/sdk_usb_context.cpp`，修复 SDK 在启动、退出时向 USB 事件处理函数
  传入空上下文导致的崩溃。
- 在 `CMakeLists.txt` 中为主驱动添加四个 libusb 链接包装，并注册回归测试。
- 新增 `test/sdk_usb_context_test.cpp`，验证上下文转发、事件中断、
  初始化失败、释放和重新初始化。
- 更新 README 中的仓库地址、工作空间、SDK 库路径及 OpenCV 说明。
- 新增 `install.md`、`easyrun.md`、`update.md`，整理安装、运行和维护流程。
- 当前 `package.xml` 的版本为 `0.14.4`。

构建、单元测试、实机图像/IMU/里程计/点云接收和正常退出均已验证，
详细记录见 [USB 修复说明](docs/usb_startup_crash.md)。
RGB 参数设置超时仍需单独排查。

## 2. 拉取前备份配置

先在驱动终端按 Ctrl+C 停止运行，再执行：

```bash
cd ~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver
git status --short
git branch --show-current
git remote -v

ODIN_BACKUP_DIR="$HOME/odin_driver_backups/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$ODIN_BACKUP_DIR"
cp -a config "$ODIN_BACKUP_DIR/"
printf '配置备份目录：%s\n' "$ODIN_BACKUP_DIR"
```

如果 `git status --short` 显示未提交的改动，先检查并提交需要保留的内容，
再继续拉取。本次修复的提交命令见第 5 节。
配置备份目录位于仓库外；记录命令输出的路径，供新终端使用。

## 3. 更新源码与重新构建

工作区干净且位于 `main` 分支时：

```bash
cd ~/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver
git pull --ff-only origin main
```

如果提示分支已分叉，先检查本地与远端提交，处理后再继续。
本流程使用快进更新。

更新后对照备份检查 `config/control_command.yaml`，合并需要保留的设备参数，
同时保留新版本增加的字段。

在新的构建终端中执行：

```bash
source /opt/ros/humble/setup.bash
cd ~/longxiaoze_ws
colcon build --packages-select odin_ros_driver \
  --cmake-args \
  -DBUILD_SYSTEM=ROS2 \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
source install/setup.bash
colcon test --packages-select odin_ros_driver
colcon test-result --verbose
```

如修改过依赖查找路径、SDK 路径或 CMake 配置，可在上述构建命令的
`--cmake-args` 前增加 `--cmake-clean-cache`，然后重新构建。
日常更新采用包级增量构建。

## 4. 更新后验证

```bash
source /opt/ros/humble/setup.bash
source ~/longxiaoze_ws/install/setup.bash
ros2 pkg prefix odin_ros_driver
ros2 launch odin_ros_driver odin1_ros2.launch.py
```

确认出现 `Device ready and streams activated`，在第二个已加载环境的终端中
检查图像、IMU、里程计和点云。最后在启动终端按 Ctrl+C，确认
`host_sdk_sample` 输出 `process has finished cleanly`。

可检查构建产物是否包含 USB 包装函数：

```bash
nm -C ~/longxiaoze_ws/install/odin_ros_driver/lib/odin_ros_driver/host_sdk_sample \
  | grep '__wrap_libusb_'
```

应看到 `init`、`exit`、`handle_events_timeout` 和
`interrupt_event_handler` 对应的四个包装函数。

## 5. 提交并推送本次改动

下列命令提交本次驱动修复和文档更新；构建产物保留在工作空间的
`build/`、`install/`、`log/` 中。

```bash
cd /home/techshare/longxiaoze_ws/src/longxiaoze-robot-driver-odin_ros_driver
git status --short
git diff --check
git add README.md install.md easyrun.md update.md \
  CMakeLists.txt package.xml src/sdk_usb_context.cpp \
  test/sdk_usb_context_test.cpp docs/usb_startup_crash.md
git diff --cached --stat
git commit -m "修复 Odin USB 上下文崩溃并完善安装运行文档"
git push origin main
```

如果推送被拒绝且提示远端有新提交，在本地改动已提交后执行
`git pull --rebase origin main`，解决可能的冲突、重新验证，再执行
`git push origin main`。

当前远端地址可以通过 `git remote -v` 检查。如果仍指向其他仓库，改为：

```bash
git remote set-url origin https://github.com/Longxiaoze/longxiaoze-robot-driver-odin_ros_driver.git
```
