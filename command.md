# 조향 
ros2 topic pub -r 10 /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.3, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: 0.0, acceleration: 0.0, jerk: 0.0}}"
---
layout:
  dim: []
  data_offset: 0
data:
- 0.0
- 0.0
- 17.188734536943613
- 1.0
---

# 전진
ros2 topic pub -r 10 /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.0, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: 0.5, acceleration: 0.2, jerk: 0.0}}"
---
layout:
  dim: []
  data_offset: 0
data:
- 0.5
- 0.0
- 0.0
- 1.0
---

# 후진
ros2 topic pub -r 10 /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.0, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: -0.3, acceleration: 0.2, jerk: 0.0}}"
---
layout:
  dim: []
  data_offset: 0
data:
- 0.30000001192092896
- 0.0
- 0.0
- 2.0
---


CCACHE_DISABLE=1 colcon build --symlink-install
colcon build --packages-select ouster_ros
colcon build --packages-select yhs_can_control
colcon build --packages-select yhs_can_control ouster_ros

지금 기어는 1(D), 2(R), 3(N)인상태이다. 이거 만약에 아닐시에 그냥 밑에있는 파라미터바꾸고 재빌드한다
yhs_autoware_vehicle_status_bridge.param.yaml (line 8)


futuredrive@futuredrive-desktop:~/yhs_control2_ws$ ros2 launch ouster_ros sensor.launch.xml sensor_hostname:=os-122124000141.local point_type:=original points_topic:=/sensing/lidar/top/point_cloud imu_topic:=/sensing/imu/imu_data
[INFO] [launch]: All log files can be found below /home/futuredrive/.ros/log/2026-04-10-10-48-53-435033-futuredrive-desktop-18906
[INFO] [launch]: Default logging verbosity is set to INFO
[INFO] [os_driver-1]: process started with pid [18907]
[INFO] [bash-2]: process started with pid [18909]
[os_driver-1] [WARN] [1775785733.789177521] [ouster.os_driver]: lidar port set to zero, the client will assign a random port number!
[os_driver-1] [WARN] [1775785733.789314047] [ouster.os_driver]: imu port set to zero, the client will assign a random port number!
[os_driver-1] [INFO] [1775785733.789394913] [ouster.os_driver]: auto start requested
[os_driver-1] [INFO] [1775785734.789815819] [ouster.os_driver]: auto start initiated
[os_driver-1] [INFO] [1775785734.790063824] [ouster.os_driver]: Will use automatic UDP destination
[os_driver-1] [INFO] [1775785734.790119611] [ouster.os_driver]: Contacting sensor os-122124000141.local ...
[bash-2] [INFO] [1775785739.550988144] [rviz2]: Stereo is NOT SUPPORTED
[bash-2] [INFO] [1775785739.551155091] [rviz2]: OpenGl version: 4.6 (GLSL 4.6)
[bash-2] [INFO] [1775785739.581522604] [rviz2]: Stereo is NOT SUPPORTED
[os_driver-1] [ERROR] [1775785739.793544197] [ouster.os_driver]: Error connecting to sensor os-122124000141.local, details: CurlClient::execute_request failed for the url: [os-122124000141.local/api/v1/sensor/metadata/sensor_info] with the error message: Couldn't resolve host name

http://os-122124000141.local/
ping os-122124000141.local

futuredrive@futuredrive-desktop:~/yhs_control2_ws$ ros2 topic list
/clicked_point
/goal_pose
/initialpose
/ouster/imu
/ouster/metadata
/ouster/nearir_image
/ouster/os_driver/transition_event
/ouster/points
/ouster/range_image
/ouster/reflec_image
/ouster/scan
/ouster/signal_image
/ouster/telemetry
/parameter_events
/rosout
/tf
/tf_static

docker rm -f $(docker ps -aq)

getent hosts os-122124000141.local
sudo ip addr add 192.168.20.99/24 dev eth0
ping -c 3 192.168.20.100



Node name: crop_box_filter_measurement_range
Node namespace: /localization/util


Node name: concatenate_data
Node namespace: /sensing/lidar
Topic type: sensor_msgs/msg/Poi