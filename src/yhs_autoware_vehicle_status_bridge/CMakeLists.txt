cmake_minimum_required(VERSION 3.8)
project(yhs_autoware_vehicle_status_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(autoware_auto_vehicle_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(yhs_can_interfaces REQUIRED)

add_executable(yhs_autoware_vehicle_status_bridge_node
  src/yhs_autoware_vehicle_status_bridge_node.cpp)

target_compile_features(yhs_autoware_vehicle_status_bridge_node PUBLIC c_std_99 cxx_std_17)

ament_target_dependencies(
  yhs_autoware_vehicle_status_bridge_node
  autoware_auto_vehicle_msgs
  rclcpp
  yhs_can_interfaces
)

install(
  TARGETS yhs_autoware_vehicle_status_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch params
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
