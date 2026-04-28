from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    params_file_argument = DeclareLaunchArgument(
        'params_file',
        description='Path to a YAML file with node parameters.',
    )

    node = Node(
        package='approx_camera_info_publisher',
        executable='approx_camera_info_publisher',
        name='approx_camera_info_publisher',
        output='screen',
        parameters=[LaunchConfiguration('params_file')],
    )

    return LaunchDescription([params_file_argument, node])
