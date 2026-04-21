from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_share = get_package_share_directory("yhs_autoware_vehicle_status_bridge")

    return LaunchDescription([
        Node(
            package="yhs_autoware_vehicle_status_bridge",
            executable="yhs_autoware_vehicle_status_bridge_node",
            name="yhs_autoware_vehicle_status_bridge_node",
            output="screen",
            parameters=[
                f"{package_share}/params/yhs_autoware_vehicle_status_bridge.param.yaml",
            ],
        )
    ])
