# Autoware 실행 전 환경 세팅 정리

이 문서는 `run2.sh`로 실차 스택을 띄우기 전에, Autoware가 비교적 안정적으로 동작하도록 먼저 맞춰둘 환경 세팅을 정리한 메모다.

기준:
- Autoware 공식 문서
- Autoware Foundation 공식 레퍼런스 문서
- 실차 기준 우선순위

## 1. 가장 먼저 맞출 것

우선순위는 아래 순서로 보는 게 좋다.

1. DDS / ROS 2 통신 설정
2. 빌드 타입
3. 시간 기준(`use_sim_time`)
4. vehicle / sensor / map / calibration 준비 상태
5. OS / 커널 / 컨테이너 / GPU 환경

---

## 2. DDS / ROS 2 통신 설정

Autoware 공식 문서는 `CycloneDDS`를 권장하고, large message(point cloud / image) 수신을 위해 DDS와 커널 버퍼를 먼저 튜닝하라고 안내한다.

공식 문서:
- https://autowarefoundation.github.io/autoware-documentation/main/installation/additional-settings-for-developers/network-configuration/dds-settings/
- https://autowarefoundation.github.io/autoware-documentation/main/installation/additional-settings-for-developers/network-configuration/multiple-computers/

### 권장 기본값

```bash
sudo ip link set lo multicast on
sudo sysctl -w net.core.rmem_max=2147483647
sudo sysctl -w net.ipv4.ipfrag_time=3
sudo sysctl -w net.ipv4.ipfrag_high_thresh=134217728
```

```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export CYCLONEDDS_URI=file:///home/futuredrive/cyclonedds.xml
export ROS_DOMAIN_ID=26
```

### 확인 명령

```bash
echo $RMW_IMPLEMENTATION
echo $CYCLONEDDS_URI
echo $ROS_DOMAIN_ID
sysctl net.core.rmem_max net.ipv4.ipfrag_time net.ipv4.ipfrag_high_thresh
ip link show lo
```

### 중요 메모

- 최신 공식 문서는 `.bashrc`에 `ROS_LOCALHOST_ONLY=1`을 넣지 말라고 경고한다.
- 여러 PC를 붙일 때는 `cyclonedds.xml`에서 실제 네트워크 인터페이스를 명시하는 쪽이 권장된다.
- 여러 PC를 붙일 때는 시간 동기화도 같이 맞춰야 한다.

---

## 3. CycloneDDS 설정 파일

공식 문서 예시는 대략 아래 형태다.

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<CycloneDDS xmlns="https://cdds.io/config"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="https://cdds.io/config https://raw.githubusercontent.com/eclipse-cyclonedds/cyclonedds/master/etc/cyclonedds.xsd">
  <Domain Id="any">
    <General>
      <Interfaces>
        <NetworkInterface autodetermine="false" name="lo" priority="default" multicast="default" />
      </Interfaces>
      <AllowMulticast>default</AllowMulticast>
      <MaxMessageSize>65500B</MaxMessageSize>
    </General>
    <Discovery>
      <ParticipantIndex>none</ParticipantIndex>
    </Discovery>
    <Internal>
      <SocketReceiveBufferSize min="10MB"/>
      <Watermarks>
        <WhcHigh>500kB</WhcHigh>
      </Watermarks>
    </Internal>
  </Domain>
</CycloneDDS>
```

현재 워크스페이스에서 쓰는 host 설정 파일:
- [cyclonedds.xml](/home/futuredrive/cyclonedds.xml:1)

주의:
- 한 PC 안에서만 host 프로세스끼리 돌릴 때는 `lo` 기반 설정이 간단하다.
- 다른 PC와 통신할 때는 `lo`만 쓰면 안 되고, 실제 NIC를 써야 한다.

---

## 4. 빌드 타입

공식 troubleshooting 문서는 Autoware가 느리거나 point cloud가 밀릴 때 `Release` 또는 `RelWithDebInfo`로 다시 빌드하라고 한다.

공식 문서:
- https://autowarefoundation.github.io/autoware-documentation/main/support/troubleshooting/performance-troubleshooting/

권장:

```bash
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
```

또는

```bash
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

메모:
- Debug 성격으로 빌드되면 pointcloud_preprocessor 등에서 성능 저하가 눈에 띌 수 있다.

---

## 5. 시간 기준 (`use_sim_time`)

실차에서는 보통 `/clock`이 없으므로 `use_sim_time:=false`가 자연스럽다.

반대로 아래 상황에서는 `use_sim_time:=true`가 맞다.
- planning simulator
- CARLA / AWSIM 같은 simulator
- rosbag replay
- `/clock` 퍼블리셔가 실제로 존재하는 환경

공식 참고:
- https://autowarefoundation.github.io/autoware_tools/main/common/tier4_simulated_clock_rviz_plugin/

확인:

```bash
ros2 topic list | grep '^/clock$'
ros2 topic echo --once /clock
```

해석:
- `/clock`이 없으면 `use_sim_time=true`가 꼭 필요하지는 않다.
- 드라이버나 일부 브리지는 겉으로는 돌아가 보여도, localization / planning / control 단계에서 시간 기준 불일치가 문제를 만들 수 있다.

---

## 6. 실차 launch 전 준비물

공식 문서는 실차 launch 전에 아래가 준비되어 있어야 한다고 본다.

공식 문서:
- https://autowarefoundation.github.io/autoware-documentation/main/how-to-guides/integrating-autoware/launch-autoware/
- https://autowarefoundation.github.io/autoware-documentation/main/how-to-guides/integrating-autoware/creating-vehicle-and-sensor-model/creating-vehicle-model/
- https://autowarefoundation.github.io/autoware-documentation/main/how-to-guides/integrating-autoware/creating-vehicle-and-sensor-model/creating-sensor-model/

필수 항목:
- `vehicle_model`
- `sensor_model`
- `map_path`
- `vehicle_id`
- vehicle description package
- sensor kit description package
- sensor calibration
- Autoware 호환 vehicle interface
- pointcloud map
- lanelet2 map

추가 메모:
- GNSS 초기화가 없다면 수동 initial pose가 필요하다.
- perception 모델 파일이 필요한 경우 `autoware_data` 경로도 준비해야 한다.

---

## 7. OS / 하드웨어 / 컨테이너

Autoware Foundation 레퍼런스 문서 기준 권장 사항:
- Ubuntu 22.04
- Kernel 5.15+
- production에는 real-time kernel 권장
- RAM 32GB 이상, 권장은 64GB+
- perception 사용 시 NVIDIA GPU 사실상 필요
- Jetson / x86 모두 containerized deployment 권장

공식 문서:
- https://autowarefoundation.github.io/LSA-reference-design-docs/main/software-configuration/getting-started/
- https://autowarefoundation.github.io/open-ad-kit-docs/latest/version-2.0/start-guide/installation/system-setup-host/

컨테이너 환경이면 추가 확인:
- Docker
- NVIDIA Container Toolkit
- host network
- GPU runtime
- X11 access (RViz 쓸 경우)

---

## 8. 실차에서 바로 확인할 체크리스트

### 8-1. ROS / DDS 환경

```bash
echo $RMW_IMPLEMENTATION
echo $CYCLONEDDS_URI
echo $ROS_DOMAIN_ID
echo ${ROS_LOCALHOST_ONLY:-unset}
```

### 8-2. 커널 / 네트워크 버퍼

```bash
sysctl net.core.rmem_max
sysctl net.ipv4.ipfrag_time
sysctl net.ipv4.ipfrag_high_thresh
ip link show lo
```

### 8-3. 시간

```bash
timedatectl status
ros2 topic list | grep '^/clock$'
```

### 8-4. 성능

```bash
free -h
htop
sudo tegrastats --interval 1000
```

### 8-5. 빌드 타입 확인 메모

가능하면 최근 빌드가 `Release` 또는 `RelWithDebInfo`였는지 다시 확인한다.

---

## 9. 현재 워크스페이스에서 특히 주의할 점

현재 상황 기준 메모:

- `run2.sh`의 terminal 2는 Ouster를 `use_sim_time:=true`로 띄우고 있음
  - [run2.sh](/home/futuredrive/yhs_control2_ws/run2.sh:16)
- `carla_pointcloud_preprocessor`는 기본 `use_sim_time`이 `true`
  - [carla_pointcloud_preprocessor.launch.py](/home/futuredrive/yhs_control2_ws/src/carla_pointcloud_preprocessor/launch/carla_pointcloud_preprocessor.launch.py:154)
- 현재 host 쪽 CycloneDDS는 `/home/futuredrive/cyclonedds.xml`을 사용 중
- Autoware cache 모듈은 `ROS_DOMAIN_ID=26`을 전제로 하는 구성이 있음

즉 실제 실차 운용 전에는 아래를 다시 점검하는 게 좋다.

1. `run2.sh`의 모든 터미널이 같은 `ROS_DOMAIN_ID`, `RMW_IMPLEMENTATION`, `CYCLONEDDS_URI`를 쓰는지
2. `/clock`이 없는 실차 환경이면 `use_sim_time`을 다시 정리할지
3. map / vehicle / sensor model / calibration 경로가 실제 launch와 일치하는지

---

## 10. 짧은 결론

Autoware를 안정적으로 띄우기 위한 우선 환경세팅은 아래 5개로 요약된다.

1. `CycloneDDS + multicast + sysctl`
2. `Release` 또는 `RelWithDebInfo` 빌드
3. `use_sim_time`을 환경에 맞게 통일
4. `vehicle_model / sensor_model / map / calibration` 준비
5. 실차면 네트워크 / 시간 / GPU / 컨테이너 조건까지 같이 점검
