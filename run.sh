#!/usr/bin/env bash

WS="$HOME/yhs_control2_ws"
BACKEND="$HOME/astudio/furive-kit-backend/client-deploy"

open_term() {
  local title="$1"
  local cmd="$2"

  gnome-terminal \
    --title="$title" \
    -- bash -lc "$cmd; echo; echo '[DONE or EXITED] $title'; exec bash"
}

open_term "1_can_control" "
cd '$WS' &&
sudo -v &&
sudo ip link set can2 down &&
sudo ip link set can2 up type can bitrate 500000 &&
unset ROS_LOCALHOST_ONLY &&
sudo ip link set lo multicast on &&
sudo sysctl -w net.core.rmem_max=2147483647 &&
source install/setup.bash &&
ros2 launch yhs_can_control yhs_can_control.launch.py
"

open_term "2_sensor_tf" "
cd '$WS' &&
source install/setup.bash &&
ros2 run tf2_ros static_transform_publisher \
  0.795 0 0.915 0 0 0 \
  base_link sensor_kit_base_link &
ros2 run tf2_ros static_transform_publisher \
  0 0 0 0 0 0 \
  sensor_kit_base_link os_sensor &
wait
"

open_term "3_ouster_ros" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch ouster_ros sensor.launch.xml \
  sensor_hostname:=os-122124000141.local \
  use_sim_time:=false \
  sensor_frame:=os_sensor \
  lidar_frame:=os_lidar \
  imu_frame:=os_imu \
  point_cloud_frame:=os_lidar \
  point_type:=original \
  points_topic:=/sensing/lidar/top/pointcloud \
  imu_topic:=/sensing/imu/imu_data \
  timestamp_mode:=TIME_FROM_ROS_TIME \
  viz:=true
"

open_term "4_tod_autoware_bridge" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch yhs_can_control tod_autoware_bridge.launch.py
"

open_term "5_vehicle_status_bridge" "
cd '$WS' &&
source install/setup.bash
ros2 run yhs_autoware_vehicle_status_bridge yhs_autoware_vehicle_status_bridge_node --ros-args \
  --params-file src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml \
  -p force_autonomous_control_mode:=true \
  -p use_veh_diag_for_control_mode:=false
"

open_term "6_pointcloud_preprocessor" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch carla_pointcloud_preprocessor carla_pointcloud_preprocessor.launch.py
" 

open_term "7_novatel" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch novatel_oem7_driver oem7_net.launch.py
"

# TODO: replace the identity transform below with the real camera mounting pose.
open_term "8_test_camera_tf" "
cd '$WS' &&
source install/setup.bash &&
ros2 run tf2_ros static_transform_publisher \
  0 0 0 0 0 0 \
  sensor_kit_base_link test_camera/camera_link &
ros2 run tf2_ros static_transform_publisher \
  0 0 0 -1.57079632679 0 -1.57079632679 \
  test_camera/camera_link test_camera/camera_optical_link &
wait
"

open_term "9_test_camera_image" "
cd '$WS' &&
source install/setup.bash &&
python3 cam.py \
  --cam 3 \
  --topic /test/camera/image_raw \
  --width 1920 \
  --height 1080 \
  --pub-fps 5 \
  --frame-id test_camera/camera_optical_link
"

open_term "10_test_camera_info" "
cd '$WS' &&
source install/setup.bash &&
ros2 run approx_camera_info_publisher approx_camera_info_publisher \
  --ros-args \
  -p image_topic:=/test/camera/image_raw \
  -p camera_info_topic:=/test/camera/camera_info \
  -p frame_id_override:=test_camera/camera_optical_link \
  -p hfov_deg:=90.0
"


open_term "11_backend" "
cd '$BACKEND' &&
LOG_FILTER_MODE=problems bash launch_caches_modules.sh
"
