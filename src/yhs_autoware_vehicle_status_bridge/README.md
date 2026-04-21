# yhs_autoware_vehicle_status_bridge

This package bridges `yhs_can_interfaces/msg/ChassisInfoFb` from `/chassis_info_fb`
to the Autoware vehicle status topics:

- `/vehicle/status/gear_status` (`autoware_auto_vehicle_msgs/msg/GearReport`)
- `/vehicle/status/velocity_status` (`autoware_auto_vehicle_msgs/msg/VelocityReport`)
- `/vehicle/status/steering_status` (`autoware_auto_vehicle_msgs/msg/SteeringReport`)
- `/vehicle/status/control_mode` (`autoware_auto_vehicle_msgs/msg/ControlModeReport`)

To move this bridge to another ROS 2 workspace, copy these packages together:

- `yhs_autoware_vehicle_status_bridge`
- `autoware_vehicle_msgs` (this directory contains a minimal vendored package named `autoware_auto_vehicle_msgs`)
- `yhs_can_interfaces`

This bridge publishes the standard Autoware Auto vehicle status message types
from `autoware_auto_vehicle_msgs`.

When building the copied set in the target workspace, do not also include another
`autoware_auto_vehicle_msgs` package in the same source tree.
