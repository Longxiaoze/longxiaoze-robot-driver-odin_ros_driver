"""Visualize recorded Odin topics without starting the hardware driver."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    package_dir = get_package_share_directory('odin_ros_driver')

    return LaunchDescription([
        DeclareLaunchArgument(
            'rviz_config',
            default_value=os.path.join(package_dir, 'config', 'odin_bag_ros2.rviz'),
            description='RViz configuration for the recorded Odin topics',
        ),
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use /clock from ros2 bag play --clock; set false for live topics',
        ),
        DeclareLaunchArgument(
            'tf_topic',
            default_value='/tf',
            description='Recorded TF topic; use /odin1/tf if the bag contains that name',
        ),
        Node(
            package='odin_ros_driver',
            executable='odom_path_visualizer.py',
            name='odom_path_visualizer',
            output='screen',
            parameters=[{
                'use_sim_time': ParameterValue(
                    LaunchConfiguration('use_sim_time'), value_type=bool,
                ),
            }],
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            name='odin_bag_rviz',
            output='screen',
            arguments=['-d', LaunchConfiguration('rviz_config')],
            parameters=[{
                'use_sim_time': ParameterValue(
                    LaunchConfiguration('use_sim_time'), value_type=bool,
                ),
            }],
            remappings=[('/tf', LaunchConfiguration('tf_topic'))],
        ),
    ])
