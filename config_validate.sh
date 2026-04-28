#!/usr/bin/env bash
set -euo pipefail

ROS_SETUP="${ROS_SETUP:-/opt/ros/humble/setup.bash}"
AUTOWARE_ENV="${AUTOWARE_ENV:-/home/futuredrive/astudio/furive-kit-backend/client-deploy/.caches/docker/autoware.env}"
TF_TIMEOUT_SEC="${TF_TIMEOUT_SEC:-5}"
PARAM_TOLERANCE="${PARAM_TOLERANCE:-1e-6}"
TF_TOLERANCE="${TF_TOLERANCE:-1e-3}"

# Expected values from the requested config set.
declare -A EXPECTED_PARAMS=(
  [wheel_radius]=0.210
  [wheel_width]=0.120
  [wheel_base]=0.850
  [wheel_tread]=0.645
  [front_overhang]=1.2
  [rear_overhang]=1.2
  [left_overhang]=0.6
  [right_overhang]=0.6
  [vehicle_height]=1.5
  [max_steer_angle]=0.4363
)

# parent child x y z roll pitch yaw
EXPECTED_TFS=(
  "base_link sensor_kit_base_link 0.795 0.0 0.915 0.0 0.0 0.0"
  "sensor_kit_base_link gnss_link 0.0 0.0 0.0 0.0 0.0 0.0"
  "lidar_base_link lidar_link 0.0 0.0 0.04 0.0 0.0 3.1416"
  "lidar_base_link imu_link 0.0 0.0 0.0 0.0 0.0 0.0"
)

PASS_COUNT=0
FAIL_COUNT=0

log() {
  printf '%s\n' "$*"
}

pass() {
  PASS_COUNT=$((PASS_COUNT + 1))
  printf '[PASS] %s\n' "$*"
}

fail() {
  FAIL_COUNT=$((FAIL_COUNT + 1))
  printf '[FAIL] %s\n' "$*"
}

require_cmd() {
  local cmd=$1
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "[ERROR] Required command not found: $cmd" >&2
    exit 1
  fi
}

source_env() {
  if [ -f "$ROS_SETUP" ]; then
    # shellcheck source=/dev/null
    source "$ROS_SETUP"
  fi

  if [ -f "$AUTOWARE_ENV" ]; then
    # shellcheck source=/dev/null
    source "$AUTOWARE_ENV"
    export ROS_DOMAIN_ID="${AUTOWARE_ROS_DOMAIN_ID:-${ROS_DOMAIN_ID:-}}"
    export ROS_LOCALHOST_ONLY="${AUTOWARE_ROS_LOCALHOST_ONLY:-${ROS_LOCALHOST_ONLY:-}}"
  fi
}

float_eq() {
  python3 - "$1" "$2" "$3" <<'PY'
import math
import sys

expected = float(sys.argv[1])
actual = float(sys.argv[2])
tolerance = float(sys.argv[3])
sys.exit(0 if math.isfinite(actual) and abs(expected - actual) <= tolerance else 1)
PY
}

trim() {
  local value=$1
  value="${value#"${value%%[![:space:]]*}"}"
  value="${value%"${value##*[![:space:]]}"}"
  printf '%s' "$value"
}

ensure_ros_graph() {
  local nodes
  if ! nodes=$(ros2 node list 2>/dev/null); then
    echo "[ERROR] 'ros2 node list' failed. Is Autoware running and ROS_DOMAIN_ID matched?" >&2
    exit 1
  fi

  if [ -z "$nodes" ]; then
    echo "[ERROR] No ROS 2 nodes found. Start Autoware modules first." >&2
    exit 1
  fi

  ALL_NODES="$nodes"
}

find_param_node() {
  local target_param=$1
  local node
  while IFS= read -r node; do
    [ -z "$node" ] && continue
    if ros2 param list "$node" 2>/dev/null | grep -Eq "^[[:space:]]*${target_param}[[:space:]]*$"; then
      printf '%s' "$node"
      return 0
    fi
  done <<<"$ALL_NODES"

  return 1
}

get_param_value() {
  local node=$1
  local param=$2
  local output

  output=$(ros2 param get "$node" "$param" 2>/dev/null || true)
  if [ -z "$output" ]; then
    return 1
  fi

  trim "${output##*: }"
}

check_vehicle_param() {
  local param=$1
  local expected=$2
  local node
  local actual

  if ! node=$(find_param_node "$param"); then
    fail "parameter '$param' was not found on any running node"
    return
  fi

  if ! actual=$(get_param_value "$node" "$param"); then
    fail "parameter '$param' exists on $node but value could not be read"
    return
  fi

  if float_eq "$expected" "$actual" "$PARAM_TOLERANCE"; then
    pass "param $param = $actual on $node"
  else
    fail "param $param expected $expected but got $actual on $node"
  fi
}

get_tf_values() {
  local parent=$1
  local child=$2
  local output

  output=$(timeout "${TF_TIMEOUT_SEC}s" ros2 run tf2_ros tf2_echo "$parent" "$child" 2>&1 || true)

  python3 - "$parent" "$child" <<'PY' <<<"$output"
import re
import sys

text = sys.stdin.read()
translation_matches = re.findall(r"Translation:\s*\[([^\]]+)\]", text)
rpy_matches = re.findall(r"Rotation:\s*in RPY.*?\[([^\]]+)\]", text)

if not translation_matches or not rpy_matches:
    sys.exit(1)

translation = [v.strip() for v in translation_matches[-1].split(",")]
rpy = [v.strip() for v in rpy_matches[-1].split(",")]

if len(translation) != 3 or len(rpy) != 3:
    sys.exit(1)

print(" ".join(translation + rpy))
PY
}

check_tf() {
  local parent=$1
  local child=$2
  local expected_x=$3
  local expected_y=$4
  local expected_z=$5
  local expected_roll=$6
  local expected_pitch=$7
  local expected_yaw=$8
  local values
  local actual_x
  local actual_y
  local actual_z
  local actual_roll
  local actual_pitch
  local actual_yaw

  if ! values=$(get_tf_values "$parent" "$child"); then
    fail "TF $parent -> $child could not be resolved"
    return
  fi

  read -r actual_x actual_y actual_z actual_roll actual_pitch actual_yaw <<<"$values"

  if ! float_eq "$expected_x" "$actual_x" "$TF_TOLERANCE"; then
    fail "TF $parent -> $child x expected $expected_x but got $actual_x"
    return
  fi
  if ! float_eq "$expected_y" "$actual_y" "$TF_TOLERANCE"; then
    fail "TF $parent -> $child y expected $expected_y but got $actual_y"
    return
  fi
  if ! float_eq "$expected_z" "$actual_z" "$TF_TOLERANCE"; then
    fail "TF $parent -> $child z expected $expected_z but got $actual_z"
    return
  fi
  if ! float_eq "$expected_roll" "$actual_roll" "$TF_TOLERANCE"; then
    fail "TF $parent -> $child roll expected $expected_roll but got $actual_roll"
    return
  fi
  if ! float_eq "$expected_pitch" "$actual_pitch" "$TF_TOLERANCE"; then
    fail "TF $parent -> $child pitch expected $expected_pitch but got $actual_pitch"
    return
  fi
  if ! float_eq "$expected_yaw" "$actual_yaw" "$TF_TOLERANCE"; then
    fail "TF $parent -> $child yaw expected $expected_yaw but got $actual_yaw"
    return
  fi

  pass "TF $parent -> $child matches expected transform"
}

main() {
  require_cmd ros2
  require_cmd python3
  require_cmd timeout

  source_env
  ensure_ros_graph

  log "== Vehicle Parameter Validation =="
  while IFS= read -r param; do
    check_vehicle_param "$param" "${EXPECTED_PARAMS[$param]}"
  done < <(printf '%s\n' "${!EXPECTED_PARAMS[@]}" | sort)

  log
  log "== TF Validation =="
  local tf_spec
  for tf_spec in "${EXPECTED_TFS[@]}"; do
    check_tf $tf_spec
  done

  log
  log "== Summary =="
  log "PASS: $PASS_COUNT"
  log "FAIL: $FAIL_COUNT"

  if [ "$FAIL_COUNT" -gt 0 ]; then
    exit 1
  fi
}

main "$@"
