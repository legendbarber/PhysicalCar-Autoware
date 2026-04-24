cmake_minimum_required(VERSION 3.8)
project(tod_autoware_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(autoware_auto_control_msgs REQUIRED)
find_package(autoware_auto_vehicle_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(tod_autoware_bridge_node
  src/main.cpp
  src/tod_autoware_bridge_node.cpp
)

target_include_directories(tod_autoware_bridge_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(tod_autoware_bridge_node PUBLIC c_std_99 cxx_std_17)

ament_target_dependencies(tod_autoware_bridge_node
  autoware_auto_control_msgs
  autoware_auto_vehicle_msgs
  rclcpp
  std_msgs
)

install(TARGETS tod_autoware_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch params
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
