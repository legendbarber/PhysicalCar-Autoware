# terminal 1
cd ~/yhs_control2_ws
sudo ifconfig can2 down
sudo ip link set can2 up type can bitrate 500000
unset ROS_LOCALHOST_ONLY
sudo ip link set lo multicast on
sudo sysctl -w net.core.rmem_max=2147483647
source install/setup.bash
ros2 launch yhs_can_control yhs_can_control.launch.py

# terminal 2
cd ~/yhs_control2_ws
source install/setup.bash
ros2 launch ouster_ros sensor.launch.xml \
  sensor_hostname:=os-122124000141.local \
  use_sim_time:=false \
  sensor_frame:=base_link \
  point_type:=original \
  points_topic:=/sensing/lidar/top/pointcloud \
  imu_topic:=/sensing/imu/imu_data \
  timestamp_mode:=TIME_FROM_ROS_TIME


# terminal 3
cd ~/yhs_control2_ws
source install/setup.bash
ros2 launch yhs_can_control tod_autoware_bridge.launch.py 

# terminal 4
cd ~/yhs_control2_ws
source install/setup.bash
ros2 run yhs_autoware_vehicle_status_bridge yhs_autoware_vehicle_status_bridge_node --ros-args \
  --params-file src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml \
  -p force_autonomous_control_mode:=true \
  -p use_veh_diag_for_control_mode:=false
  
# terminal 5
cd ~/yhs_control2_ws
source install/setup.bash
ros2 launch carla_pointcloud_preprocessor carla_pointcloud_preprocessor.launch.py

# terminal 6
cd ~/yhs_control2_ws
source install/setup.bash
ros2 launch novatel_oem7_driver novatel_oem7_net.launch.py

# terminal 7
cd ~/astudio/furive-kit-backend/client-deploy/
LOG_FILTER_MODE=problems bash launch_caches_modules.sh 
---
# rosbag
# terminal 1
cd ~/yhs_control2_ws
export ROS_LOCALHOST_ONLY=1
source install/setup.bash
python3 ./rosbags/freeze_frame.py ./rosbags/scene1 --offset 6.0 --mode stack --rate 5 --clock-rate 100

# terminal 2
cd ~/yhs_control2_ws
export ROS_LOCALHOST_ONLY=1
source install/setup.bash
ros2 run yhs_autoware_vehicle_status_bridge yhs_autoware_vehicle_status_bridge_node --ros-args \
  --params-file src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml \
  -p force_autonomous_control_mode:=true \
  -p use_veh_diag_for_control_mode:=false

# terminal 3
# cd ~/yhs_control2_ws
# source install/setup.bash
# ros2 launch yhs_can_control tod_autoware_bridge.launch.py

# terminal 4
cd ~/yhs_control2_ws
export ROS_LOCALHOST_ONLY=1
source install/setup.bash
ros2 launch carla_pointcloud_preprocessor carla_pointcloud_preprocessor.launch.py use_sim_time:=true

# terminal 7
cd ~/astudio/furive-kit-backend/client-deploy/
LOG_FILTER_MODE=problems bash launch_caches_modules.sh 
