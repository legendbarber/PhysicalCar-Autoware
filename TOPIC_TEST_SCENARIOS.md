# 토픽 직접 발행 테스트 가이드

이 문서는 지금 프로젝트에서 직접 토픽을 보내서 아래 2가지를 확인하기 위한 문서입니다.

1. `tod_autoware_bridge`가 숫자를 제대로 바꾸는지
2. 실제 차량에서 기어 숫자 매핑이 맞는지

직접 확인된 결과는 이것입니다.

- `tod_autoware_bridge`는 전진 명령을 raw gear `4`로 보내야 맞습니다.
- `tod_autoware_bridge`의 후진 기어는 raw gear `2`가 맞습니다.
- `yhs_autoware_vehicle_status_bridge`는 아래처럼 해석하는 것이 맞았습니다.
  - `1 = PARK`
  - `2 = REVERSE`
  - `3 = NEUTRAL`
  - `4 = DRIVE`

즉, 이 프로젝트에서는 전진 기어를 `4`로 맞춰야 합니다.

## 지금 바로 할 일

헷갈리면 이것만 먼저 하시면 됩니다.

1. `terminal 1`, `terminal 4`를 띄웁니다.
2. 관측용 터미널 2개를 엽니다.
3. `speed=0.0`으로 `/tod_cmd`에 기어 숫자 `1`, `2`, `3`, `4`를 하나씩 넣습니다.
4. `/chassis_info_fb.ctrl_fb_gear`와 `/vehicle/status/gear_status`가 어떻게 바뀌는지 적습니다.
5. 현재 확인 결과와 비교해서, 실제 차량도 같은지 확인합니다.

아래에 그대로 따라할 명령을 적어두었습니다.

## 테스트 전에 꼭

- 공도에서 하지 마세요.
- 차량이 실제로 움직일 수 있는 상태라면 바퀴 고정, 충분한 공간, 보조 인원 확보 후 진행하세요.
- 처음 테스트는 반드시 `speed=0.0`으로만 하세요.
- 기어 확인 전에는 저속 주행 테스트를 하지 마세요.

## 현재 토픽 흐름

현재 코드 흐름은 아래입니다.

- `/control/command/control_cmd`
  -> `tod_autoware_bridge_node`
  -> `/tod_cmd`
- `/tod_cmd`
  -> `yhs_can_control_node`
  -> CAN
  -> `/chassis_info_fb`
- `/chassis_info_fb`
  -> `yhs_autoware_vehicle_status_bridge_node`
  -> `/vehicle/status/gear_status`
  -> `/vehicle/status/velocity_status`
  -> `/vehicle/status/steering_status`
  -> `/vehicle/status/control_mode`

## 먼저 띄워둘 노드

### 최소 구성

기어 매핑 테스트만 할 때는 이것만 있어도 됩니다.

- terminal 1: `yhs_can_control`
- terminal 4: `yhs_autoware_vehicle_status_bridge`

각 터미널에서:

```bash
cd ~/yhs_control2_ws
source install/setup.bash
```

terminal 1:

```bash
ros2 launch yhs_can_control yhs_can_control.launch.py
```

terminal 4:

```bash
ros2 launch yhs_autoware_vehicle_status_bridge yhs_autoware_vehicle_status_bridge.launch.py
```

## 관측용 터미널

아래 2개는 꼭 켜두세요.

관측 터미널 A:

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic echo --field ctrl_fb.ctrl_fb_gear /chassis_info_fb
```

관측 터미널 B:

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic echo /vehicle/status/gear_status
```

추가로 보면 좋은 것들:

```bash
ros2 topic echo /vehicle/status/velocity_status
```

```bash
ros2 topic echo /vehicle/status/steering_status
```

```bash
ros2 topic echo --field data /tod_cmd
```

## 결과 읽는 법

`/vehicle/status/gear_status`의 `report` 값은 이렇게 읽으면 됩니다.

- `22` = `PARK`
- `20` = `REVERSE`
- `1` = `NEUTRAL`
- `2` = `DRIVE`
- `0` = `NONE`

## 테스트 1: 차량 안 움직이게 기어 숫자만 확인

이게 가장 중요합니다.

목표:

- 실제 차가 어떤 raw gear 숫자를 쓰는지 확인
- 현재 설정된 `forward_gear: 4`가 실제 차량에도 맞는지 확인

조건:

- `speed=0.0`
- `steering=0.0`
- 짧게만 발행

아래 명령을 한 번에 하나씩 실행하세요.

### 1-1. raw gear = 1 넣기

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic pub -r 10 /tod_cmd std_msgs/msg/Float64MultiArray "{data: [0.0, 50.0, 0.0, 1.0]}"
```

2~3초만 보고 `Ctrl-C`로 끄세요.

기록할 것:

- `/chassis_info_fb.ctrl_fb_gear` 값
- `/vehicle/status/gear_status.report` 값

### 1-2. raw gear = 2 넣기

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic pub -r 10 /tod_cmd std_msgs/msg/Float64MultiArray "{data: [0.0, 50.0, 0.0, 2.0]}"
```

### 1-3. raw gear = 3 넣기

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic pub -r 10 /tod_cmd std_msgs/msg/Float64MultiArray "{data: [0.0, 50.0, 0.0, 3.0]}"
```

### 1-4. raw gear = 4 넣기

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic pub -r 10 /tod_cmd std_msgs/msg/Float64MultiArray "{data: [0.0, 50.0, 0.0, 4.0]}"
```

## 테스트 1에서 기대하는 해석

직접 소프트웨어로 확인한 기준값은 아래입니다.

- raw `1` 넣었더니 `gear_status.report = 22`
  -> raw `1`은 `PARK`
- raw `2` 넣었더니 `gear_status.report = 20`
  -> raw `2`는 `REVERSE`
- raw `3` 넣었더니 `gear_status.report = 1`
  -> raw `3`은 `NEUTRAL`
- raw `4` 넣었더니 `gear_status.report = 2`
  -> raw `4`는 `DRIVE`

이 값들은 이미 브리지 단독 테스트에서 확인되었습니다.

즉 현재 프로젝트 기준 결론은:

- 상태 브리지 해석은 맞음
- 전진 기어는 `4`
- 후진 기어는 `2`

## 테스트 2: Autoware 명령을 넣었을 때 `/tod_cmd`가 어떻게 바뀌는지 확인

이 테스트는 차량 CAN 없이도 할 수 있습니다.

목표:

- `tod_autoware_bridge_node`가 `/control/command/control_cmd`를 `/tod_cmd`로 바꾸는 방식 확인

이 테스트는 terminal 3만 띄워도 됩니다.

terminal 3:

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 launch yhs_can_control tod_autoware_bridge.launch.py
```

관측용:

```bash
cd ~/yhs_control2_ws
source install/setup.bash
ros2 topic echo --field data /tod_cmd
```

### 2-1. 전진 명령 넣기

```bash
ros2 topic pub --once /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.0, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: 0.1, acceleration: 0.0, jerk: 0.0}}"
```

기대:

- `/tod_cmd[0]` 근처 값이 `0.1`
- `/tod_cmd[1]` 근처 값이 `0.0`
- `/tod_cmd[2]` 근처 값이 `0.0`
- `/tod_cmd[3]` 값이 `4`

### 2-2. 후진 명령 넣기

```bash
ros2 topic pub --once /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.0, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: -0.1, acceleration: 0.0, jerk: 0.0}}"
```

기대:

- `/tod_cmd[0]` 근처 값이 `0.1`
- `/tod_cmd[3]` 값이 `2`

### 2-3. 브레이크 변환 확인

```bash
ros2 topic pub --once /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.0, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: 0.0, acceleration: -1.0, jerk: 0.0}}"
```

기대:

- `/tod_cmd[1]` 값이 대략 `20.0`

### 2-4. 조향 변환 확인

```bash
ros2 topic pub --once /control/command/control_cmd autoware_auto_control_msgs/msg/AckermannControlCommand "{stamp: {sec: 0, nanosec: 0}, lateral: {stamp: {sec: 0, nanosec: 0}, steering_tire_angle: 0.1, steering_tire_rotation_rate: 0.0}, longitudinal: {stamp: {sec: 0, nanosec: 0}, speed: 0.0, acceleration: 0.0, jerk: 0.0}}"
```

기대:

- `/tod_cmd[2]` 값이 대략 `5.73`

## 테스트 결과에 따라 무엇을 고칠지

### 경우 1. `/tod_cmd`에서 전진 기어가 4로 나가고, 실제 상태도 DRIVE로 보임

이 경우:

- 현재 설정이 맞습니다.

### 경우 2. `/tod_cmd`에서는 전진 기어가 4로 나가는데, 실제 차량 피드백이 계속 1(PARK)로 남음

이 경우:

- 브리지 숫자 문제보다 차량이 기어 명령을 수락하지 않는 문제일 가능성이 큽니다.
- 제어 enable 상태나 차량 수락 조건을 먼저 확인해야 합니다.

### 경우 3. raw gear 4를 넣었는데도 상태 브리지에서 DRIVE로 안 읽힘

이 경우 의심할 것:

- `src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml`

즉, 상태 브리지의 기어 해석 숫자를 바꿔야 할 수 있습니다.

### 경우 4. `/tod_cmd` 값도 이상하고 상태도 이상함

이 경우 둘 다 봐야 합니다.

- `src/yhs_can_control/params/tod_autoware_bridge.yaml`
- `src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml`

## 수정 후 해야 할 것

파라미터를 바꿨다면 다시 빌드하고 다시 source 해야 합니다.

```bash
cd ~/yhs_control2_ws
colcon build --packages-select yhs_can_control yhs_autoware_vehicle_status_bridge
source install/setup.bash
```

그리고 위 테스트 1을 다시 반복해서 확인하세요.

## 제가 추천하는 실제 순서

정말로 간단하게 하면 이 순서가 가장 좋습니다.

1. terminal 1, terminal 4를 띄웁니다.
2. `/chassis_info_fb.ctrl_fb_gear`와 `/vehicle/status/gear_status`를 관측합니다.
3. `/tod_cmd`에 `speed=0.0`으로 raw gear `1`, `2`, `3`, `4`를 하나씩 넣습니다.
4. 어떤 숫자가 `PARK/REVERSE/NEUTRAL/DRIVE`인지 표로 적습니다.
5. 실제 차량도 `1=PARK`, `2=REVERSE`, `3=NEUTRAL`, `4=DRIVE`인지 확인합니다.

## 테스트 기록용 표

아래처럼 직접 채우면 됩니다.

| 내가 넣은 raw gear | `/chassis_info_fb.ctrl_fb_gear` | `/vehicle/status/gear_status.report` | 내가 해석한 의미 |
| --- | --- | --- | --- |
| 1 |  |  |  |
| 2 |  |  |  |
| 3 |  |  |  |
| 4 |  |  |  |

## 가장 가능성 높은 현재 결론

현재 확인된 결론은 이것입니다.

- 상태 브리지는 `1=PARK`, `2=REVERSE`, `3=NEUTRAL`, `4=DRIVE`로 해석하는 것이 맞음
- 명령 브리지는 전진 기어를 `4`, 후진 기어를 `2`로 써야 함

즉 현재 수정 방향은 이미 확정되었고, 이제는 실제 차량이 이 값을 그대로 받아들이는지만 확인하면 됩니다.
