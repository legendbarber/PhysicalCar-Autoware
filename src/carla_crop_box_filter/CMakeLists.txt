cmake_minimum_required(VERSION 3.8)
project(carla_crop_box_filter)

find_package(ament_cmake REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(tf2_ros REQUIRED)

add_library(carla_crop_box_filter_component SHARED
  src/crop_box_filter_node.cpp
)

target_include_directories(carla_crop_box_filter_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(carla_crop_box_filter_component PUBLIC c_std_99 cxx_std_17)

ament_target_dependencies(carla_crop_box_filter_component
  geometry_msgs
  rclcpp
  rclcpp_components
  sensor_msgs
  tf2
  tf2_eigen
  tf2_ros
)

target_link_libraries(carla_crop_box_filter_component Eigen3::Eigen)

rclcpp_components_register_node(carla_crop_box_filter_component
  PLUGIN "carla::crop_box_filter::CropBoxFilter"
  EXECUTABLE carla_crop_box_filter_node
)

install(TARGETS carla_crop_box_filter_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
