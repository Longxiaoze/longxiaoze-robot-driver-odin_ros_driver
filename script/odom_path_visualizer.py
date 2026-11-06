#!/usr/bin/env python3
"""Turn recorded odometry into a path for RViz without publishing any TF."""

from collections import deque

import rclpy
from geometry_msgs.msg import PoseStamped
from nav_msgs.msg import Odometry, Path
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, QoSProfile, qos_profile_sensor_data


class OdometryPathVisualizer(Node):
    def __init__(self):
        super().__init__('odom_path_visualizer')
        max_poses = self.declare_parameter('max_poses', 10000).value
        if max_poses < 2:
            raise ValueError('max_poses must be at least 2')
        self._poses = deque(maxlen=max_poses)
        self._last_stamp_ns = None
        self._frame_id = None
        self._publisher = self.create_publisher(
            Path, '/odin1/visualization_path',
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL),
        )
        self._subscription = self.create_subscription(
            Odometry, '/odin1/odometry', self._on_odometry, qos_profile_sensor_data,
        )

    def _on_odometry(self, message):
        stamp = message.header.stamp
        stamp_ns = stamp.sec * 1_000_000_000 + stamp.nanosec
        # Do not connect separate playback loops or different reference frames.
        if (message.header.frame_id != self._frame_id or
                (self._last_stamp_ns is not None and stamp_ns < self._last_stamp_ns)):
            self._poses.clear()
        self._frame_id = message.header.frame_id
        self._last_stamp_ns = stamp_ns
        self._poses.append(PoseStamped(header=message.header, pose=message.pose.pose))
        self._publisher.publish(Path(header=message.header, poses=list(self._poses)))


def main(args=None):
    rclpy.init(args=args)
    node = OdometryPathVisualizer()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
