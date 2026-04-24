# tod_autoware_bridge

Bridges Autoware control commands to the TOD `/tod_cmd` array format expected by `yhs_can_control`.

## Topics

- Subscribes: `/control/command/control` (`autoware_auto_control_msgs/msg/AckermannControlCommand`)
- Subscribes: `/control/command/gear` (`autoware_auto_vehicle_msgs/msg/GearCommand`)
- Publishes: `/tod_cmd` (`std_msgs/msg/Float64MultiArray`)

## `/tod_cmd` layout

`[velocity_mps, brake_cmd, steering_deg, gear_code]`

- `velocity_mps`: absolute speed in m/s
- `brake_cmd`: mapped from negative longitudinal acceleration to `0..255`
- `steering_deg`: steering tire angle converted from radians to degrees
- `gear_code`: TOD gear value (`0=disable, 1=P, 2=R, 3=N, 4=D`)

## Gear mapping

- `NONE -> 0`
- `PARK -> 1`
- `REVERSE`, `REVERSE_2 -> 2`
- `NEUTRAL -> 3`
- `DRIVE`, `DRIVE_2..DRIVE_18`, `LOW`, `LOW_2 -> 4`

If `/control/command/gear` is not available, the bridge can infer `R` or `D` from the sign of the commanded speed.
