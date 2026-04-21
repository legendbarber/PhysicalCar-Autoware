#!/usr/bin/python3

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

import os


def generate_launch_description():
    share_dir = get_package_share_directory("yhs_can_control")
    parameter_file = LaunchConfiguration("params_file")

    params_declare = DeclareLaunchArgument(
        "params_file",
        default_value=os.path.join(share_dir, "params", "tod_autoware_bridge.yaml"),
        description="Path to the ROS2 parameters file to use.",
    )

    bridge_node = Node(
        package="yhs_can_control",
        executable="tod_autoware_bridge_node",
        name="tod_autoware_bridge_node",
        output="screen",
        parameters=[parameter_file],
    )

    return LaunchDescription([
        params_declare,
        bridge_node,
    ])
