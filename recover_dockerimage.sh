#!/usr/bin/env bash
set -u

REMOTE_IMAGES=(
  "nvcr.io/nvidia/isaac/ros:aarch64-ros2_humble_77e6a678c2058abf96bedcb8f7dd4330"
  "furivezo/nvblox_nav2:latest"
  "furivezo/nvblox_src:v250919"
  "furivezo/nvblox_src:v250920"
  "furivezo/nvblox_src:v251001"
  "furivezo/nvblox_src:v251009"
  "furivezo/nvblox_src:v251015"
  "furivezo/nvblox_src:v251016"
  "furivezo/nvblox_src:v1.0"
  "reg.autonomystudio.net/fd-module/future-nav2:jetson"
)

FAILED=()

for image in "${REMOTE_IMAGES[@]}"; do
  echo
  echo "==== PULL $image ===="
  if ! docker pull "$image"; then
    echo "!!!! FAILED: $image"
    FAILED+=("$image")
  fi
done

echo
echo "==== LOCAL IMAGES ===="
docker images

echo
echo "==== FAILED PULLS ===="
if [ "${#FAILED[@]}" -eq 0 ]; then
  echo "none"
else
  printf '%s\n' "${FAILED[@]}"
fi

cat <<'EOF'

==== LOCAL-ONLY IMAGES THAT CANNOT BE RESTORED BY PULL ====
These were local image tags. Rebuild them from their original Dockerfiles/build scripts if needed:

  furive-kit:base-cb9e33bb
  isaac_ros_dev-aarch64:latest
  nova_carter-image:latest
  ros2_humble-image:latest
  my_nav_image_saved:latest
  my_nav_container_v2:latest
  realsense-image:latest
  aarch64-image:latest
  isaac_ros_deploy-base:latest
  isaac_ros_deploy-installed:latest
  isaac_ros_deploy:latest
  furive_nav2_image:jetson

Likely rebuild locations to inspect:
  /home/futuredrive/astudio/furive-kit-backend/client-deploy/build.sh
  /home/futuredrive/astudio/furive-kit-backend/docker-deploy/build.sh
  /home/futuredrive/furive-launcher/docker/Dockerfile
  /home/futuredrive/deplot_nav_ws/build.sh
  /home/futuredrive/workspace/realsense_ws/Dockerfile.user
EOF
