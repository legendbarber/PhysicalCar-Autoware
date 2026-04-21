from copy import deepcopy
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer
from launch_ros.actions import SetParameter
from launch_ros.descriptions import ComposableNode
import yaml


def _load_ros_parameters(param_file):
    with open(param_file, "r", encoding="utf-8") as file:
        return yaml.safe_load(file)["/**"]["ros__parameters"]


def get_vehicle_info(context):
    vehicle_info_param_file = LaunchConfiguration("vehicle_info_param_file").perform(context)
    global_params = _load_ros_parameters(vehicle_info_param_file)
    return {
        "min_x": -(global_params["rear_overhang"]),
        "max_x": global_params["front_overhang"] + global_params["wheel_base"],
        "min_y": -(global_params["wheel_tread"] / 2.0 + global_params["right_overhang"]),
        "max_y": global_params["wheel_tread"] / 2.0 + global_params["left_overhang"],
        "min_z": 0.0,
        "max_z": global_params["vehicle_height"],
    }


def get_vehicle_mirror_info(context):
    mirror_param_file = LaunchConfiguration("vehicle_mirror_param_file").perform(context)
    return _load_ros_parameters(mirror_param_file)


def launch_setup(context, *args, **kwargs):
    crop_box_filter_package = LaunchConfiguration("crop_box_filter_package").perform(context)
    crop_box_filter_plugin = LaunchConfiguration("crop_box_filter_plugin").perform(context)

    base_parameters = {
        "input_frame": LaunchConfiguration("input_frame"),
        "output_frame": LaunchConfiguration("output_frame"),
        "use_sim_time": LaunchConfiguration("use_sim_time"),
        "negative": True,
        "processing_time_threshold_sec": 0.01,
    }

    vehicle_info = get_vehicle_info(context)
    self_crop_parameters = deepcopy(base_parameters)
    self_crop_parameters.update(vehicle_info)

    mirror_info = get_vehicle_mirror_info(context)
    mirror_crop_parameters = deepcopy(base_parameters)
    mirror_crop_parameters.update(
        {
            "min_x": mirror_info["min_longitudinal_offset"],
            "max_x": mirror_info["max_longitudinal_offset"],
            "min_y": mirror_info["min_lateral_offset"],
            "max_y": mirror_info["max_lateral_offset"],
            "min_z": mirror_info["min_height_offset"],
            "max_z": mirror_info["max_height_offset"],
        }
    )

    input_topic = LaunchConfiguration("input_topic")
    output_topic = LaunchConfiguration("output_topic")

    nodes = [
        ComposableNode(
            package=crop_box_filter_package,
            plugin=crop_box_filter_plugin,
            name="carla_crop_box_filter_self",
            remappings=[
                ("input", input_topic),
                ("output", "self_cropped/pointcloud"),
            ],
            parameters=[self_crop_parameters],
            extra_arguments=[
                {"use_intra_process_comms": LaunchConfiguration("use_intra_process")}
            ],
        ),
        ComposableNode(
            package=crop_box_filter_package,
            plugin=crop_box_filter_plugin,
            name="carla_crop_box_filter_mirror",
            remappings=[
                ("input", "self_cropped/pointcloud"),
                ("output", "mirror_cropped/pointcloud"),
            ],
            parameters=[mirror_crop_parameters],
            extra_arguments=[
                {"use_intra_process_comms": LaunchConfiguration("use_intra_process")}
            ],
        ),
        ComposableNode(
            package="topic_tools",
            plugin="topic_tools::RelayNode",
            name="carla_pointcloud_relay",
            parameters=[
                {
                    "input_topic": "mirror_cropped/pointcloud",
                    "output_topic": output_topic,
                    "type": "sensor_msgs/msg/PointCloud2",
                    "use_sim_time": LaunchConfiguration("use_sim_time"),
                }
            ],
            extra_arguments=[
                {"use_intra_process_comms": LaunchConfiguration("use_intra_process")}
            ],
        ),
    ]

    return [
        ComposableNodeContainer(
            name=LaunchConfiguration("pointcloud_container_name"),
            namespace="",
            package="rclcpp_components",
            executable="component_container_mt",
            composable_node_descriptions=nodes,
            output="screen",
        )
    ]


def generate_launch_description():
    package_share_dir = get_package_share_directory("carla_pointcloud_preprocessor")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "pointcloud_container_name",
                default_value="carla_pointcloud_container",
            ),
            DeclareLaunchArgument(
                "vehicle_info_param_file",
                default_value=os.path.join(
                    package_share_dir,
                    "config",
                    "vehicle",
                    "vehicle_info.param.yaml",
                ),
            ),
            DeclareLaunchArgument(
                "vehicle_mirror_param_file",
                default_value=os.path.join(
                    package_share_dir,
                    "config",
                    "vehicle",
                    "mirror.param.yaml",
                ),
            ),
            DeclareLaunchArgument("use_sim_time", default_value="false"),
            DeclareLaunchArgument("use_multithread", default_value="true"),
            DeclareLaunchArgument("use_intra_process", default_value="true"),
            SetParameter(name="use_sim_time", value=LaunchConfiguration("use_sim_time")),
            DeclareLaunchArgument("input_frame", default_value="base_link"),
            DeclareLaunchArgument("output_frame", default_value="base_link"),
            DeclareLaunchArgument(
                "input_topic",
                default_value="/sensing/lidar/top/pointcloud",
            ),
            DeclareLaunchArgument(
                "output_topic",
                default_value="/sensing/lidar/concatenated/pointcloud",
            ),
            DeclareLaunchArgument(
                "crop_box_filter_package",
                default_value="carla_crop_box_filter",
            ),
            DeclareLaunchArgument(
                "crop_box_filter_plugin",
                default_value="carla::crop_box_filter::CropBoxFilter",
            ),
            OpaqueFunction(function=launch_setup),
        ]
    )
