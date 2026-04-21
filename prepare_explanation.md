# prepare.sh 설명

이 문서는 [prepare.sh](/home/futuredrive/yhs_control2_ws/prepare.sh:1)가 무엇을 세팅하는지 설명한다.

목적:
- Autoware 실행 전에 DDS / ROS 2 통신 환경을 한 번에 정리
- 실차용 기본 환경을 반복 가능하게 맞춤
- 예전에 수동으로 하던 `dds.sh` 작업을 한 파일로 통합

---

## 1. 이 스크립트가 하는 일

`prepare.sh`는 아래 5가지를 설정한다.

1. Linux 커널 `sysctl` 값 적용
2. loopback(`lo`) 인터페이스 multicast 활성화
3. `~/cyclonedds.xml` 생성
4. `~/.autoware_env.sh` 생성
5. `~/.bashrc`에 Autoware 환경 source 라인 추가

선택적으로:
- Jetson이면 `jetson_clocks` 실행 가능

---

## 2. 생성 / 수정되는 파일

### 2-1. `/etc/sysctl.d/60-autoware-cyclonedds.conf`

이 파일은 DDS large message 수신 안정성을 위한 커널 파라미터를 담는다.

들어가는 값:

```conf
net.core.rmem_max=2147483647
net.ipv4.ipfrag_time=3
net.ipv4.ipfrag_high_thresh=134217728
```

의미:
- `net.core.rmem_max`
  - 네트워크 수신 버퍼 최대 크기
  - point cloud / image 같은 큰 메시지 수신에 중요
- `net.ipv4.ipfrag_time`
  - IP fragmentation 조각을 얼마나 오래 보관할지
- `net.ipv4.ipfrag_high_thresh`
  - fragmentation queue 메모리 상한

그리고 아래 명령으로 바로 적용한다.

```bash
sudo sysctl -p /etc/sysctl.d/60-autoware-cyclonedds.conf
```

---

### 2-2. `lo multicast` 활성화

실행 명령:

```bash
sudo ip link set lo multicast on
```

의미:
- ROS 2 / DDS discovery에서 loopback 기반 통신이 제대로 되도록 도움
- Autoware 공식 DDS 설정 문서에서 권장하는 항목

---

### 2-3. `~/cyclonedds.xml`

이 파일은 CycloneDDS 런타임 설정 파일이다.

기본 동작:
- 기본 인터페이스는 `lo`
- `ParticipantIndex=none`
- `SocketReceiveBufferSize min="10MB"`

기본 사용 예:

```xml
<NetworkInterface autodetermine="false" name="lo" priority="default" multicast="default" />
```

옵션:
- `--iface lo`
  - loopback 사용
- `--iface auto`
  - 자동 인터페이스 선택
- `--iface <실제인터페이스이름>`
  - 예: `can0`, `eth0`, `enP8p1s0`

주의:
- 한 PC 안에서 host 프로세스끼리 통신하면 `lo`가 단순하다
- 여러 PC와 ROS 2 통신을 해야 하면 실제 NIC를 쓰는 쪽이 맞다

---

### 2-4. `~/.autoware_env.sh`

이 파일은 Autoware 실행용 환경변수를 모아놓은 파일이다.

기본으로 들어가는 값:

```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export CYCLONEDDS_URI=file:///home/futuredrive/cyclonedds.xml
export ROS_DOMAIN_ID=26
unset ROS_LOCALHOST_ONLY
```

의미:
- `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`
  - CycloneDDS 사용
- `CYCLONEDDS_URI=...`
  - 위에서 만든 `cyclonedds.xml` 사용
- `ROS_DOMAIN_ID=26`
  - ROS graph 분리
- `unset ROS_LOCALHOST_ONLY`
  - 기존에 꼬여 있는 localhost 전용 설정 제거

---

### 2-5. `~/.bashrc`

기본적으로 아래 줄이 들어간다.

```bash
[ -f "$HOME/.autoware_env.sh" ] && source "$HOME/.autoware_env.sh"
```

의미:
- 새 터미널을 열 때마다 Autoware 실행 환경을 자동으로 불러옴

옵션:
- `--no-bashrc`
  - `.bashrc`에는 아무것도 추가하지 않음

---

## 3. 실행 후 현재 셸에 반영되는지

### `bash prepare.sh`

```bash
bash prepare.sh
```

이 경우:
- 파일 생성 / 시스템 설정은 적용됨
- 하지만 현재 셸의 환경변수는 안 바뀜
- 새 터미널을 열거나 아래를 따로 실행해야 함

```bash
source ~/.autoware_env.sh
```

### `source prepare.sh`

```bash
source prepare.sh
```

이 경우:
- 파일 생성 / 시스템 설정 적용
- 현재 셸도 바로 업데이트

---

## 4. 옵션 설명

### `--iface <name>`

예:

```bash
bash prepare.sh --iface lo
bash prepare.sh --iface auto
bash prepare.sh --iface eth0
```

의미:
- CycloneDDS가 어느 인터페이스를 쓸지 정함

### `--ros-domain-id <id>`

예:

```bash
bash prepare.sh --ros-domain-id 26
```

의미:
- `ROS_DOMAIN_ID` 값을 지정

### `--enable-jetson-clocks`

예:

```bash
bash prepare.sh --enable-jetson-clocks
```

의미:
- Jetson 환경에서 `sudo jetson_clocks` 실행
- CPU / GPU / 메모리 클럭을 performance 쪽으로 고정

### `--no-bashrc`

예:

```bash
bash prepare.sh --no-bashrc
```

의미:
- `.bashrc` 자동 source 추가 안 함

### `--dry-run`

예:

```bash
bash prepare.sh --dry-run
```

의미:
- 실제 변경 없이 어떤 작업을 할지만 출력

---

## 5. 이 스크립트가 안 하는 일

`prepare.sh`는 아래는 하지 않는다.

- `run2.sh` 실행
- CAN 인터페이스 up/down
- 센서 드라이버 실행
- Autoware cache 모듈 실행
- map / vehicle / sensor calibration 검증
- `use_sim_time` 변경

즉, 이 스크립트는 “실행 전 공통 환경 준비”까지만 담당한다.

---

## 6. 예전 `dds.sh`와 차이

예전 [dds.sh](/home/futuredrive/yhs_control2_ws/dds.sh:1)도 비슷한 작업을 했지만, `prepare.sh`는 아래 점이 더 낫다.

- 한 파일에 목적이 정리됨
- 재실행해도 비교적 안전함
- `.autoware_env.sh`로 환경을 분리해서 관리함
- `--dry-run` 지원
- `--iface`, `--ros-domain-id`, `--enable-jetson-clocks` 옵션 지원
- 현재 셸에 바로 반영할지(`source`) 선택 가능

---

## 7. 권장 사용법

실차용 기본:

```bash
bash /home/futuredrive/yhs_control2_ws/prepare.sh
source ~/.autoware_env.sh
```

Jetson 성능 모드까지 같이:

```bash
bash /home/futuredrive/yhs_control2_ws/prepare.sh --enable-jetson-clocks
source ~/.autoware_env.sh
```

현재 셸까지 한 번에:

```bash
source /home/futuredrive/yhs_control2_ws/prepare.sh
```

---

## 8. 짧은 결론

`prepare.sh`는 Autoware 실행 전 필요한 DDS / ROS 2 공통 환경을 한 번에 맞춰주는 스크립트다.

핵심은 아래다.

1. 커널 네트워크 버퍼 튜닝
2. loopback multicast 활성화
3. CycloneDDS 설정 파일 생성
4. Autoware 실행용 환경변수 파일 생성
5. 새 터미널에서도 같은 환경이 유지되도록 `.bashrc` 연결
