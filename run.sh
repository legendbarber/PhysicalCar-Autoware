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

open_term "2_ouster_ros" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch ouster_ros sensor.launch.xml \
  sensor_hostname:=os-122124000141.local \
  use_sim_time:=false \
  sensor_frame:=base_link \
  point_type:=original \
  points_topic:=/sensing/lidar/top/pointcloud \
  imu_topic:=/sensing/imu/imu_data \
  timestamp_mode:=TIME_FROM_ROS_TIME
"

open_term "3_tod_autoware_bridge" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch yhs_can_control tod_autoware_bridge.launch.py
"

open_term "4_vehicle_status_bridge" "
cd '$WS' &&
source install/setup.bash
ros2 run yhs_autoware_vehicle_status_bridge yhs_autoware_vehicle_status_bridge_node --ros-args \
  --params-file src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml \
  -p force_autonomous_control_mode:=true \
  -p use_veh_diag_for_control_mode:=false
"

open_term "5_pointcloud_preprocessor" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch carla_pointcloud_preprocessor carla_pointcloud_preprocessor.launch.py
"

open_term "6_novatel" "
cd '$WS' &&
source install/setup.bash &&
ros2 launch novatel_oem7_driver novatel_oem7_net.launch.py
"

open_term "7_backend" "
cd '$BACKEND' &&
LOG_FILTER_MODE=problems bash launch_caches_modules.sh
"