#!/usr/bin/env bash

set -euo pipefail

WORKSPACE_DIR="/home/futuredrive/yhs_control2_ws"
BACKEND_DIR="/home/futuredrive/astudio/furive-kit-backend/client-deploy"
SCRIPT_PATH="$(readlink -f "${BASH_SOURCE[0]}")"

run_pane1() {
  cd "$WORKSPACE_DIR"
  sudo ifconfig can2 down
  sudo ip link set can2 up type can bitrate 500000
  unset ROS_LOCALHOST_ONLY
  sudo ip link set lo multicast on
  sudo sysctl -w net.core.rmem_max=2147483647
  source install/setup.bash
  ros2 launch yhs_can_control yhs_can_control.launch.py
}

run_pane2() {
  cd "$WORKSPACE_DIR"
  source install/setup.bash
  ros2 launch ouster_ros sensor.launch.xml \
    sensor_hostname:=os-122124000141.local \
    use_sim_time:=true \
    sensor_frame:=base_link \
    point_type:=original \
    points_topic:=/sensing/lidar/top/pointcloud \
    imu_topic:=/sensing/imu/imu_data
}

run_pane3() {
  cd "$WORKSPACE_DIR"
  source install/setup.bash
  ros2 launch yhs_can_control tod_autoware_bridge.launch.py
}

run_pane4() {
  cd "$WORKSPACE_DIR"
  source install/setup.bash
  ros2 launch yhs_autoware_vehicle_status_bridge yhs_autoware_vehicle_status_bridge.launch.py
}

run_pane5() {
  cd "$WORKSPACE_DIR"
  source install/setup.bash
  ros2 launch carla_pointcloud_preprocessor carla_pointcloud_preprocessor.launch.py
}

run_pane6() {
  cd "$WORKSPACE_DIR"
  source install/setup.bash
  ros2 launch novatel_oem7_driver novatel_oem7_net.launch.py
}

run_pane7() {
  cd "$BACKEND_DIR"
  LOG_FILTER_MODE=problems bash launch_caches_modules.sh
}

write_layout_json() {
  local layout_path="$1"

  cat > "$layout_path" <<EOF
{
  "layout": {
    "vertical": false,
    "main": [
      {
        "ratio": 0.5,
        "children": [
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane1; status=\$?; echo; echo [pane1 exited: \$status]; exec bash'" },
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane2; status=\$?; echo; echo [pane2 exited: \$status]; exec bash'" },
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane3; status=\$?; echo; echo [pane3 exited: \$status]; exec bash'" },
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane4; status=\$?; echo; echo [pane4 exited: \$status]; exec bash'" }
        ]
      },
      {
        "children": [
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane5; status=\$?; echo; echo [pane5 exited: \$status]; exec bash'" },
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane6; status=\$?; echo; echo [pane6 exited: \$status]; exec bash'" },
          { "command": "bash -lc 'cd $WORKSPACE_DIR && $SCRIPT_PATH pane7; status=\$?; echo; echo [pane7 exited: \$status]; exec bash'" }
        ]
      }
    ]
  }
}
EOF
}

launch_terminator() {
  if ! command -v terminator >/dev/null 2>&1; then
    echo "terminator is not installed."
    exit 1
  fi

  if [[ -z "${DISPLAY:-}" ]]; then
    echo "DISPLAY is not set. Run this from a desktop terminal inside X/GUI."
    exit 1
  fi

  echo "Caching sudo credentials for pane 1..."
  sudo -v

  local layout_file
  layout_file="$(mktemp /tmp/yhs_terminator_layout.XXXXXX.json)"
  trap 'rm -f "$layout_file"' EXIT

  write_layout_json "$layout_file"

  terminator -T "yhs_control launcher" -j "$layout_file"
}

print_usage() {
  cat <<'EOF'
Usage:
  ./run.sh         Launch Terminator with 7 panes
  ./run.sh launch  Same as above
  ./run.sh pane1   Run only terminal 1 command
  ./run.sh pane2   Run only terminal 2 command
  ./run.sh pane3   Run only terminal 3 command
  ./run.sh pane4   Run only terminal 4 command
  ./run.sh pane5   Run only terminal 5 command
  ./run.sh pane6   Run only terminal 6 command
  ./run.sh pane7   Run only terminal 7 command
EOF
}

case "${1:-launch}" in
  launch)
    launch_terminator
    ;;
  pane1)
    run_pane1
    ;;
  pane2)
    run_pane2
    ;;
  pane3)
    run_pane3
    ;;
  pane4)
    run_pane4
    ;;
  pane5)
    run_pane5
    ;;
  pane6)
    run_pane6
    ;;
  pane7)
    run_pane7
    ;;
  *)
    print_usage
    exit 1
    ;;
esac
