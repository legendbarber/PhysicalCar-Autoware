#!/usr/bin/python3

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    share_dir = get_package_share_directory("tod_autoware_bridge")
    params_file = LaunchConfiguration("params_file")

    params_declare = DeclareLaunchArgument(
        "params_file",
        default_value=os.path.join(share_dir, "params", "cfg.yaml"),
        description="Path to the ROS 2 parameters file to use.",
    )

    bridge_node = Node(
        package="tod_autoware_bridge",
        executable="tod_autoware_bridge_node",
        name="tod_autoware_bridge_node",
        output="screen",
        parameters=[params_file],
    )

    return LaunchDescription([
        params_declare,
        bridge_node,
    ])
