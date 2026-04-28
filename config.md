### senser_kit_calibration.yaml
sensor_kit_base_link:
  lidar_base_link:
    lidar_link:
      x: 0.0
      y: 0.0
      z: 0.04
      roll: 0.0
      pitch: 0.0
      yaw: 3.1416
    imu_link:
      x: 0.0
      y: 0.0
      z: 0.0
      roll: 0.0
      pitch: 0.0
      yaw: 0.0
  gnss_link:
    x: 0.0
    y: 0.0
    z: 0.0
    roll: 0.0
    pitch: 0.0
    yaw: 0.0
    
### sensors_calibration.yaml
base_link:
  sensor_kit_base_link:
    x: 0.795
    y: 0.0
    z: 0.915
    roll: 0.0
    pitch: 0.0
    yaw: 0.0

### vehicle_info.param.yaml
/**:
  ros__parameters:
    wheel_radius: 0.210 # The radius of the wheel, primarily used for dead reckoning.
    wheel_width: 0.120 # The lateral width of a wheel tire, primarily used for dead reckoning.
    wheel_base: 0.850 # between front wheel center and rear wheel center
    wheel_tread: 0.645 # between left wheel center and right wheel center
    front_overhang: 1.2 #0.375 # between front wheel center and vehicle front
    rear_overhang: 1.2 #0.375 # between rear wheel center and vehicle rear
    left_overhang: 0.6 #0.0875 # between left wheel center and vehicle left
    right_overhang: 0.6 #0.0875 # between right wheel center and vehicle right
    vehicle_height: 1.5
    max_steer_angle: 0.4363 # [rad]
