from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description() -> LaunchDescription:
    launch_arguments = [
        DeclareLaunchArgument(
            'image_topic',
            default_value='/image_raw',
            description='Input sensor_msgs/msg/Image topic.',
        ),
        DeclareLaunchArgument(
            'camera_info_topic',
            default_value='/camera_info',
            description='Output sensor_msgs/msg/CameraInfo topic.',
        ),
        DeclareLaunchArgument(
            'frame_id_override',
            default_value='',
            description='Optional frame_id to publish instead of Image.header.frame_id.',
        ),
        DeclareLaunchArgument(
            'width_override',
            default_value='0',
            description='Optional published width override. Use 0 to follow the image.',
        ),
        DeclareLaunchArgument(
            'height_override',
            default_value='0',
            description='Optional published height override. Use 0 to follow the image.',
        ),
        DeclareLaunchArgument(
            'fx',
            default_value='0.0',
            description='Explicit focal length in pixels along X.',
        ),
        DeclareLaunchArgument(
            'fy',
            default_value='0.0',
            description='Explicit focal length in pixels along Y.',
        ),
        DeclareLaunchArgument(
            'cx',
            default_value='0.0',
            description='Explicit principal point X.',
        ),
        DeclareLaunchArgument(
            'cy',
            default_value='0.0',
            description='Explicit principal point Y.',
        ),
        DeclareLaunchArgument(
            'distortion_model',
            default_value='plumb_bob',
            description='CameraInfo distortion model string.',
        ),
        DeclareLaunchArgument(
            'hfov_deg',
            default_value='0.0',
            description='Horizontal field of view in degrees.',
        ),
        DeclareLaunchArgument(
            'vfov_deg',
            default_value='0.0',
            description='Vertical field of view in degrees.',
        ),
        DeclareLaunchArgument(
            'dfov_deg',
            default_value='0.0',
            description='Diagonal field of view in degrees.',
        ),
    ]

    node = Node(
        package='approx_camera_info_publisher',
        executable='approx_camera_info_publisher',
        name='approx_camera_info_publisher',
        output='screen',
        parameters=[{
            'image_topic': ParameterValue(LaunchConfiguration('image_topic'), value_type=str),
            'camera_info_topic': ParameterValue(
                LaunchConfiguration('camera_info_topic'), value_type=str
            ),
            'frame_id_override': ParameterValue(
                LaunchConfiguration('frame_id_override'), value_type=str
            ),
            'width_override': ParameterValue(
                LaunchConfiguration('width_override'), value_type=int
            ),
            'height_override': ParameterValue(
                LaunchConfiguration('height_override'), value_type=int
            ),
            'fx': ParameterValue(LaunchConfiguration('fx'), value_type=float),
            'fy': ParameterValue(LaunchConfiguration('fy'), value_type=float),
            'cx': ParameterValue(LaunchConfiguration('cx'), value_type=float),
            'cy': ParameterValue(LaunchConfiguration('cy'), value_type=float),
            'distortion_model': ParameterValue(
                LaunchConfiguration('distortion_model'), value_type=str
            ),
            'hfov_deg': ParameterValue(LaunchConfiguration('hfov_deg'), value_type=float),
            'vfov_deg': ParameterValue(LaunchConfiguration('vfov_deg'), value_type=float),
            'dfov_deg': ParameterValue(LaunchConfiguration('dfov_deg'), value_type=float),
        }],
    )

    return LaunchDescription(launch_arguments + [node])
