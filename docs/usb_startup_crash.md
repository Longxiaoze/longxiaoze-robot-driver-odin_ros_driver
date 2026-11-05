# USB startup crash in the bundled SDK

## Symptom and cause

The driver exits with signal 11 immediately after the SDK reports `device
attach`, before the driver's `Hardware connected` message. A captured backtrace
on Ubuntu 22.04 / ROS 2 Humble / aarch64 shows:

```text
libusb_get_next_timeout
libusb_handle_events_timeout_completed
usb_data_xfer::event_monitor_routine
```

The bundled SDK initializes a private context with `libusb_init(&context)` and
registers its hotplug callback on that context. Its event monitor and shutdown
drain instead call `libusb_handle_events_timeout(NULL, ...)`. The null argument
selects the default context, which the SDK has not initialized. The captured
crash dereferences that null context inside `libusb_get_next_timeout`.

The shutdown path also calls `libusb_interrupt_event_handler(NULL)`. It must
interrupt that same private context, and can otherwise crash during Ctrl+C.
The captured shutdown backtrace shows `pthread_mutex_lock(mutex=0x1d8)` called
from `libusb_interrupt_event_handler`, then `usb_data_xfer::xfer_deinit`.

## Driver workaround

`src/sdk_usb_context.cpp` records the successfully initialized private context,
forwards the SDK's null-context event calls to it, and forgets it on exit.
It also forwards null-context event interrupts to that private context.
Explicit event contexts, timeouts, and libusb return values are preserved.
Event calls before initialization or after shutdown return
`LIBUSB_ERROR_NO_DEVICE`; interrupts without a live context are ignored.

CMake uses GNU linker's `--wrap` for `libusb_init`, `libusb_exit`,
`libusb_handle_events_timeout`, and `libusb_interrupt_event_handler` on
`host_sdk_sample`. This affects references
from the bundled static SDK, leaving calls inside shared libraries unchanged.
The adapter assumes the driver's existing single SDK instance / USB context.
It does not replace libusb or modify the precompiled SDK archives.

The upstream SDK fix is to pass its private context to both event processing
calls and the shutdown interrupt. Once the bundled SDK contains that fix, this
adapter can be removed.

## Build and verify (ROS 2)

From the workspace root:

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select odin_ros_driver \
  --cmake-args -DBUILD_SYSTEM=ROS2 -DBUILD_TESTING=ON
colcon test --packages-select odin_ros_driver
colcon test-result --verbose
source install/setup.bash
ros2 launch odin_ros_driver odin1_ros2.launch.py
```

`sdk_usb_context_test` checks context forwarding, explicit-context handling,
event interrupts, shutdown, failed initialization, and reinitialization using
fake libusb calls.
An attached Odin is needed to verify the SDK event loop and data delivery.

## Validation on 2026-11-05

On the attached Ubuntu 22.04 / ROS 2 Humble / aarch64 host:

- Package build completed successfully.
- `colcon test-result --verbose`: 1 test, 0 errors, 0 failures.
- The original `ros2 launch odin_ros_driver odin1_ros2.launch.py` command ran
  for a 25-second observation window, including startup and discovery.
- The SDK reported `Device ready and streams activated`.
- After SIGINT (Ctrl+C), launch returned 0 and `host_sdk_sample` reported
  `process has finished cleanly`; no host crash occurred.

Received message counts during that observation window:

| Topic | Messages |
| --- | ---: |
| `/odin1/imu` | 5536 |
| `/odin1/odometry` | 135 |
| `/odin1/image` | 43 |
| `/odin1/cloud_raw` | 117 |
| `/odin1/cloud_slam` | 106 |

The device still reports a timeout for `lidar_set_rgb_parameter` with the
configured MJPEG / 1600x1296 / 10 fps request. Image messages were received;
application of the requested RGB parameters remains unverified. This USB
context fix does not address that device command timeout.
