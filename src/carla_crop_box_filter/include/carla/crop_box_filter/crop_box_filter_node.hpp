#ifndef CARLA__CROP_BOX_FILTER__CROP_BOX_FILTER_NODE_HPP_
#define CARLA__CROP_BOX_FILTER__CROP_BOX_FILTER_NODE_HPP_

#include <Eigen/Core>

#include <geometry_msgs/msg/polygon_stamped.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace carla::crop_box_filter
{

class CropBoxFilter : public rclcpp::Node
{
public:
  explicit CropBoxFilter(const rclcpp::NodeOptions & options);
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  using PointCloud2 = sensor_msgs::msg::PointCloud2;
  using PointCloud2ConstPtr = sensor_msgs::msg::PointCloud2::ConstSharedPtr;

  struct CropBoxParam
  {
    float min_x{0.0F};
    float max_x{0.0F};
    float min_y{0.0F};
    float max_y{0.0F};
    float min_z{0.0F};
    float max_z{0.0F};
    bool negative{false};
  };

  struct FieldOffsets
  {
    size_t x{0};
    size_t y{0};
    size_t z{0};
  };

  rclcpp::Subscription<PointCloud2>::SharedPtr sub_input_;
  rclcpp::Publisher<PointCloud2>::SharedPtr pub_output_;
  rclcpp::Publisher<geometry_msgs::msg::PolygonStamped>::SharedPtr crop_box_polygon_pub_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr set_param_res_;

  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  std::mutex mutex_;
  std::string tf_input_frame_;
  std::string tf_output_frame_;
  int64_t max_queue_size_{5};
  CropBoxParam param_;

  void pointcloud_callback(const PointCloud2ConstPtr cloud);
  bool filter_pointcloud(const PointCloud2ConstPtr & cloud, PointCloud2 & output);
  void publish_crop_box_polygon();
  rcl_interfaces::msg::SetParametersResult param_callback(
    const std::vector<rclcpp::Parameter> & parameters);

  bool is_valid(const PointCloud2ConstPtr & cloud) const;
  std::optional<FieldOffsets> find_xyz_offsets(const PointCloud2 & cloud) const;
  std::optional<Eigen::Matrix4f> lookup_transform_matrix(
    const std::string & target_frame, const std::string & source_frame,
    const builtin_interfaces::msg::Time & stamp);

  template <typename T>
  bool get_param(const std::vector<rclcpp::Parameter> & parameters, const std::string & name, T & value)
  {
    const auto it = std::find_if(
      parameters.cbegin(), parameters.cend(),
      [&name](const rclcpp::Parameter & parameter) { return parameter.get_name() == name; });
    if (it == parameters.cend()) {
      return false;
    }
    value = it->template get_value<T>();
    return true;
  }
};

}  // namespace carla::crop_box_filter

#endif  // CARLA__CROP_BOX_FILTER__CROP_BOX_FILTER_NODE_HPP_
