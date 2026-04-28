# approx_camera_info_publisher

`approx_camera_info_publisher`는 `sensor_msgs/msg/Image` 토픽을 구독하고, 이에 대응하는 `sensor_msgs/msg/CameraInfo` 토픽을 발행하는 ROS 2 Python 패키지입니다. RViz의 `Camera` display가 이미지 스트림 위에 오버레이를 투영할 수 있도록 근사 `CameraInfo`를 제공하는 용도로 사용합니다.

## 경고

이 패키지가 발행하는 `CameraInfo`는 근사값입니다. 시각화, 디버깅, 초기 통합 단계에서는 유용하지만, 정확한 투영이 중요한 경우에는 실제 카메라 캘리브레이션 결과로 반드시 교체해야 합니다.

## 패키지 목적

- 입력 `sensor_msgs/msg/Image` 토픽을 구독합니다.
- 들어오는 각 이미지마다 하나의 `sensor_msgs/msg/CameraInfo` 메시지를 발행합니다.
- 입력 이미지의 타임스탬프를 그대로 맞춥니다.
- 기본적으로 이미지의 frame ID를 유지하고, 필요하면 override할 수 있습니다.
- 명시적 intrinsics 방식과 FOV 기반 근사 방식을 모두 지원합니다.

## 빌드 방법

```bash
cd ~/ros_ws
source /opt/ros/humble/setup.bash
colcon build --packages-select approx_camera_info_publisher
source install/setup.bash
```

이 패키지는 ROS 2 Humble / Jazzy 호환을 목표로 작성되었습니다. Jazzy에서는 `/opt/ros/jazzy/setup.bash`를 source하면 됩니다.

## 실행 방법

### 1. `ros2 run`으로 직접 실행

HFOV 모드:

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher --ros-args -p image_topic:=/sensing/camera/front/image_raw -p camera_info_topic:=/sensing/camera/front/camera_info -p hfov_deg:=90.0
```

명시적 intrinsics 모드:

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher --ros-args -p image_topic:=/sensing/camera/front/image_raw -p camera_info_topic:=/sensing/camera/front/camera_info -p fx:=960.0 -p fy:=960.0 -p cx:=960.0 -p cy:=540.0
```

### 2. launch 파일로 실행

```bash
ros2 launch approx_camera_info_publisher approx_camera_info.launch.py image_topic:=/sensing/camera/front/image_raw camera_info_topic:=/sensing/camera/front/camera_info hfov_deg:=90.0
```

### 3. 파라미터 파일로 실행

```bash
ros2 launch approx_camera_info_publisher approx_camera_info_with_params.launch.py params_file:=/path/to/hfov_example.yaml
```

## 파라미터 설명

| 파라미터 | 타입 | 기본값 | 설명 |
| --- | --- | --- | --- |
| `image_topic` | string | `""` | 입력 이미지 토픽입니다. 필수입니다. |
| `camera_info_topic` | string | `""` | 출력 `CameraInfo` 토픽입니다. 필수입니다. |
| `frame_id_override` | string | `""` | 비어 있지 않으면 `Image.header.frame_id` 대신 이 값을 사용합니다. |
| `width_override` | int | `0` | 0보다 크면 발행할 width를 override합니다. |
| `height_override` | int | `0` | 0보다 크면 발행할 height를 override합니다. |
| `fx` | double | `0.0` | X축 방향 명시적 초점거리(pixel)입니다. |
| `fy` | double | `0.0` | Y축 방향 명시적 초점거리(pixel)입니다. |
| `cx` | double | `0.0` | 명시적 principal point X입니다. |
| `cy` | double | `0.0` | 명시적 principal point Y입니다. |
| `distortion_model` | string | `plumb_bob` | 왜곡 모델 이름입니다. |
| `d` | double array | `[0, 0, 0, 0, 0]` | 왜곡 계수입니다. 빈 리스트면 기본값 5개 0을 사용합니다. |
| `r` | double array | `[]` | Rectification matrix입니다. 빈 리스트면 identity를 사용합니다. |
| `p` | double array | `[]` | Projection matrix입니다. 빈 리스트면 `fx`, `fy`, `cx`, `cy`로 monocular 기본값을 생성합니다. |
| `hfov_deg` | double | `0.0` | 수평 시야각(degree)입니다. |
| `vfov_deg` | double | `0.0` | 수직 시야각(degree)입니다. |
| `dfov_deg` | double | `0.0` | 대각선 시야각(degree)입니다. |

### 모드 선택 규칙

- `fx`, `fy`, `cx`, `cy`가 모두 설정되어 있으면 명시적 intrinsics 모드를 사용합니다.
- 그렇지 않으면 FOV 근사 모드를 사용합니다.
- FOV 우선순위는 아래와 같습니다.
  1. `hfov_deg`와 `vfov_deg`가 둘 다 있는 경우
  2. `hfov_deg`만 있는 경우
  3. `vfov_deg`만 있는 경우
  4. `dfov_deg`만 있는 경우
- 명시적 intrinsics도 없고 FOV 파라미터도 없으면, 노드는 즉시 명확한 에러를 출력하고 종료합니다.

## 명시적 intrinsics 모드 예시

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

예제 YAML: [`config/explicit_intrinsics_example.yaml`](config/explicit_intrinsics_example.yaml)

## HFOV 근사 모드 예시

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher \
  --ros-args \
  -p image_topic:=/sensing/camera/front/image_raw \
  -p camera_info_topic:=/sensing/camera/front/camera_info \
  -p hfov_deg:=90.0
```

예제 YAML: [`config/hfov_example.yaml`](config/hfov_example.yaml)

## DFOV 근사 모드 예시

```bash
ros2 run approx_camera_info_publisher approx_camera_info_publisher \
  --ros-args \
  -p image_topic:=/sensing/camera/front/image_raw \
  -p camera_info_topic:=/sensing/camera/front/camera_info \
  -p dfov_deg:=110.0
```

예제 YAML: [`config/dfov_example.yaml`](config/dfov_example.yaml)

## ROS 2 CLI로 확인하는 방법

발행된 `CameraInfo` 한 개를 확인:

```bash
ros2 topic echo --once /sensing/camera/front/camera_info
```

`CameraInfo`가 계속 발행되는지 확인:

```bash
ros2 topic hz /sensing/camera/front/camera_info
```

입력 이미지의 header를 확인해서 timestamp와 frame ID를 비교:

```bash
ros2 topic echo --once /sensing/camera/front/image_raw/header
```

확인 포인트:

- `CameraInfo.header.stamp`가 입력 이미지와 일치하는지
- `CameraInfo.header.frame_id`가 이미지 frame이거나 override 값인지
- `width`, `height`가 이미지 값 또는 override 값과 일치하는지
- `k`, `p`에 0이 아닌 focal length가 정상적으로 들어가는지

## RViz Camera display에서 테스트하는 방법

1. 이미지 발행 노드와 이 노드를 실행합니다.
2. RViz를 엽니다.
3. `Camera` display를 추가합니다.
4. `Image Topic`에 이미지 토픽을 설정합니다.
5. `Camera Info Topic`에 발행 중인 `camera_info` 토픽을 설정합니다.
6. 카메라 프레임이 TF로 다른 프레임들과 연결되어 있는지 확인합니다.
7. `Marker`, `Path`, `PointCloud2`, `RobotModel` 같은 다른 display를 추가합니다.
8. 해당 오버레이가 카메라 화면 위로 투영되는지 확인합니다.

## 자주 발생하는 문제와 원인

### `frame_id` 불일치

RViz에서 이미지는 보이는데 오버레이가 어긋나거나 보이지 않으면, `CameraInfo.header.frame_id`가 TF에서 사용하는 카메라 프레임과 맞지 않을 수 있습니다. 이 경우 `frame_id_override`를 사용하세요.

### image와 `camera_info` 해상도 불일치

이미지 해상도와 `CameraInfo.width`, `CameraInfo.height`가 다르면 투영이 이상해질 수 있습니다. `width_override`, `height_override`를 intrinsics와 맞추거나, 특별한 이유가 없으면 0으로 두고 입력 이미지를 그대로 따라가게 두는 것이 안전합니다.

### `K` 또는 `P`가 0인 경우

`K`, `P`에 focal length가 0으로 들어가면 RViz가 정상적으로 투영하지 못합니다. 보통 명시적 intrinsics가 불완전하거나, 유효한 FOV 파라미터 없이 노드를 실행했을 때 발생합니다.

### TF 누락

RViz `Camera` display는 카메라 프레임과 다른 display 프레임 사이에 유효한 TF 체인이 필요합니다. 이미지와 `CameraInfo`가 정상이어도 TF가 없으면 오버레이가 보이지 않을 수 있습니다.

## 포함된 파일

- 실행 파일: `approx_camera_info_publisher`
- Launch 파일:
  - `launch/approx_camera_info.launch.py`
  - `launch/approx_camera_info_with_params.launch.py`
- 예제 파라미터 파일:
  - `config/explicit_intrinsics_example.yaml`
  - `config/hfov_example.yaml`
  - `config/dfov_example.yaml`
