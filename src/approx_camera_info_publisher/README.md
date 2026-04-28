# approx_camera_info_publisher

`approx_camera_info_publisher` is a ROS 2 Python package that listens to a `sensor_msgs/msg/Image` topic and publishes a matching `sensor_msgs/msg/CameraInfo` topic. It is meant for cases where you need approximate camera intrinsics so RViz `Camera` display can project overlays onto an image stream.

## Warning

This package publishes approximate `CameraInfo`. It is useful for visualization, debugging, and early integration, but it should be replaced with real camera calibration when projection accuracy matters.

## Package purpose

- Subscribe to an input `sensor_msgs/msg/Image` topic.
- Publish one `sensor_msgs/msg/CameraInfo` message for each incoming image.
- Match the incoming image timestamp.
- Preserve the image frame ID by default, or override it when requested.
- Support either explicit intrinsics or FOV-based approximation.

## Build instructions

```bash
cd ~/ros_ws
source /opt/ros/humble/setup.bash
colcon build --packages-select approx_camera_info_publisher
source install/setup.bash
```

The package is designed for ROS 2 Humble and Jazzy. On Jazzy, source `/opt/ros/jazzy/setup.bash` instead.

## Run instructions

### 1. Direct `ros2 run`

HFOV mode:

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher --ros-args -p image_topic:=/sensing/camera/front/image_raw -p camera_info_topic:=/sensing/camera/front/camera_info -p hfov_deg:=90.0
```

Explicit intrinsics mode:

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher --ros-args -p image_topic:=/sensing/camera/front/image_raw -p camera_info_topic:=/sensing/camera/front/camera_info -p fx:=960.0 -p fy:=960.0 -p cx:=960.0 -p cy:=540.0
```

### 2. Launch file with arguments

```bash
ros2 launch approx_camera_info_publisher approx_camera_info.launch.py image_topic:=/sensing/camera/front/image_raw camera_info_topic:=/sensing/camera/front/camera_info hfov_deg:=90.0
```

### 3. Parameter file mode

```bash
ros2 launch approx_camera_info_publisher approx_camera_info_with_params.launch.py params_file:=/path/to/hfov_example.yaml
```

## Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `image_topic` | string | `""` | Input image topic. Required. |
| `camera_info_topic` | string | `""` | Output camera info topic. Required. |
| `frame_id_override` | string | `""` | If non-empty, overrides `Image.header.frame_id`. |
| `width_override` | int | `0` | If greater than zero, overrides published width. |
| `height_override` | int | `0` | If greater than zero, overrides published height. |
| `fx` | double | `0.0` | Explicit focal length in pixels along X. |
| `fy` | double | `0.0` | Explicit focal length in pixels along Y. |
| `cx` | double | `0.0` | Explicit principal point X. |
| `cy` | double | `0.0` | Explicit principal point Y. |
| `distortion_model` | string | `plumb_bob` | Distortion model name. |
| `d` | double array | `[0, 0, 0, 0, 0]` | Distortion coefficients. Empty list falls back to the default five zeros. |
| `r` | double array | `[]` | Rectification matrix. Empty list uses identity. |
| `p` | double array | `[]` | Projection matrix. Empty list uses a default monocular matrix from `fx`, `fy`, `cx`, and `cy`. |
| `hfov_deg` | double | `0.0` | Horizontal field of view in degrees. |
| `vfov_deg` | double | `0.0` | Vertical field of view in degrees. |
| `dfov_deg` | double | `0.0` | Diagonal field of view in degrees. |

### Mode selection

- Explicit intrinsics mode is used when `fx`, `fy`, `cx`, and `cy` are configured together.
- Otherwise, FOV approximation mode is used.
- FOV priority is:
  1. `hfov_deg` and `vfov_deg` together
  2. `hfov_deg` only
  3. `vfov_deg` only
  4. `dfov_deg` only
- If neither explicit intrinsics nor any FOV parameter is provided, the node exits with a clear error.

## Explicit intrinsics mode example

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher \
  --ros-args \
  -p image_topic:=/sensing/camera/front/image_raw \
  -p camera_info_topic:=/sensing/camera/front/camera_info \
  -p fx:=960.0 \
  -p fy:=960.0 \
  -p cx:=960.0 \
  -p cy:=540.0
```

Example YAML: [`config/explicit_intrinsics_example.yaml`](config/explicit_intrinsics_example.yaml)

## HFOV approximation mode example

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher \
  --ros-args \
  -p image_topic:=/sensing/camera/front/image_raw \
  -p camera_info_topic:=/sensing/camera/front/camera_info \
  -p hfov_deg:=90.0
```

Example YAML: [`config/hfov_example.yaml`](config/hfov_example.yaml)

## DFOV approximation mode example

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher \
  --ros-args \
  -p image_topic:=/sensing/camera/front/image_raw \
  -p camera_info_topic:=/sensing/camera/front/camera_info \
  -p dfov_deg:=110.0
```

Example YAML: [`config/dfov_example.yaml`](config/dfov_example.yaml)

## How to verify with ROS 2 CLI

Inspect one published `CameraInfo` message:

```bash
ros2 topic echo --once /sensing/camera/front/camera_info
```

Check that `CameraInfo` is being published continuously:

```bash
ros2 topic hz /sensing/camera/front/camera_info
```

Inspect the incoming image header to compare timestamp and frame ID:

```bash
ros2 topic echo --once /sensing/camera/front/image_raw/header
```

What to check:

- `header.stamp` on `CameraInfo` should match the incoming image message.
- `header.frame_id` should either match the image frame or the configured override.
- `width` and `height` should match the image or the configured overrides.
- `k` and `p` should contain non-zero focal lengths in the expected entries.

## How to test in RViz Camera display

1. Start the image publisher and this node.
2. Open RViz.
3. Add a `Camera` display.
4. Set the `Image Topic` to the image stream.
5. Set the `Camera Info Topic` to the published `camera_info` topic.
6. Make sure the camera frame is connected to the rest of the scene through TF.
7. Add another RViz display such as `Marker`, `Path`, `PointCloud2`, or `RobotModel`.
8. Confirm that RViz projects those overlays into the camera view.

## Common issues and causes

### `frame_id` mismatch

If RViz shows the image but overlays are misplaced or missing, the `CameraInfo.header.frame_id` may not match the camera frame used by TF. Use `frame_id_override` if the image header frame is wrong.

### Image and `camera_info` resolution mismatch

If the image stream is one resolution but `CameraInfo.width` and `CameraInfo.height` describe another, projection will look wrong. Keep `width_override` and `height_override` aligned with the intrinsics or leave them at zero to follow the incoming image.

### `K` or `P` equal to zero

If `K` or `P` contains zero focal lengths, RViz cannot project overlays correctly. This usually means the node was started without complete explicit intrinsics and without a valid FOV parameter.

### Missing TF

RViz `Camera` display needs a valid TF chain between the camera frame and the frames used by other displays. If TF is missing, overlays may not appear even when the image and `CameraInfo` topics are both correct.

## Included files

- Node executable: `approx_camera_info_publisher`
- Launch files:
  - `launch/approx_camera_info.launch.py`
  - `launch/approx_camera_info_with_params.launch.py`
- Example parameter files:
  - `config/explicit_intrinsics_example.yaml`
  - `config/hfov_example.yaml`
  - `config/dfov_example.yaml`
