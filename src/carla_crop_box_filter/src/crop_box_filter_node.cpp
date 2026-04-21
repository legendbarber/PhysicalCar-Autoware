#include "carla/crop_box_filter/crop_box_filter_node.hpp"

#include <tf2_eigen/tf2_eigen.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <utility>

#include <rclcpp_components/register_node_macro.hpp>

namespace carla::crop_box_filter
{

CropBoxFilter::CropBoxFilter(const rclcpp::NodeOptions & options)
: rclcpp::Node("carla_crop_box_filter", options)
{
  max_queue_size_ = declare_parameter<int64_t>("max_queue_size", 5);
  tf_input_frame_ = declare_parameter<std::string>("input_frame", "base_link");
  tf_output_frame_ = declare_parameter<std::string>("output_frame", "base_link");
  param_.min_x = static_cast<float>(declare_parameter<double>("min_x"));
  param_.min_y = static_cast<float>(declare_parameter<double>("min_y"));
  param_.min_z = static_cast<float>(declare_parameter<double>("min_z"));
  param_.max_x = static_cast<float>(declare_parameter<double>("max_x"));
  param_.max_y = static_cast<float>(declare_parameter<double>("max_y"));
  param_.max_z = static_cast<float>(declare_parameter<double>("max_z"));
  param_.negative = declare_parameter<bool>("negative");

  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  rclcpp::PublisherOptions pub_options;
  pub_options.qos_overriding_options = rclcpp::QosOverridingOptions::with_default_policies();
  pub_output_ = create_publisher<PointCloud2>(
    "output", rclcpp::SensorDataQoS().keep_last(max_queue_size_), pub_options);
  crop_box_polygon_pub_ = create_publisher<geometry_msgs::msg::PolygonStamped>(
    "~/crop_box_polygon", 10, pub_options);

  set_param_res_ = add_on_set_parameters_callback(
    std::bind(&CropBoxFilter::param_callback, this, std::placeholders::_1));

  sub_input_ = create_subscription<PointCloud2>(
    "input", rclcpp::SensorDataQoS().keep_last(max_queue_size_),
    std::bind(&CropBoxFilter::pointcloud_callback, this, std::placeholders::_1));
}

void CropBoxFilter::pointcloud_callback(const PointCloud2ConstPtr cloud)
{
  if (!is_valid(cloud)) {
    RCLCPP_ERROR(get_logger(), "Invalid input pointcloud");
    return;
  }

  PointCloud2 output;
  bool filtered = false;
  {
    std::scoped_lock lock(mutex_);
    filtered = filter_pointcloud(cloud, output);
  }

  if (!filtered) {
    return;
  }

  if (crop_box_polygon_pub_->get_subscription_count() > 0) {
    publish_crop_box_polygon();
  }

  pub_output_->publish(std::move(output));
}

bool CropBoxFilter::filter_pointcloud(const PointCloud2ConstPtr & cloud, PointCloud2 & output)
{
  const auto offsets = find_xyz_offsets(*cloud);
  if (!offsets) {
    throw std::runtime_error("PointCloud2 is missing x/y/z fields");
  }

  Eigen::Matrix4f preprocess_transform = Eigen::Matrix4f::Identity();
  if (!tf_input_frame_.empty() && cloud->header.frame_id != tf_input_frame_) {
    const auto tf = lookup_transform_matrix(tf_input_frame_, cloud->header.frame_id, cloud->header.stamp);
    if (!tf) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 1000, "Could not transform input frame %s -> %s",
        cloud->header.frame_id.c_str(), tf_input_frame_.c_str());
      return false;
    }
    preprocess_transform = *tf;
  }

  Eigen::Matrix4f postprocess_transform = Eigen::Matrix4f::Identity();
  if (!tf_output_frame_.empty() && tf_output_frame_ != tf_input_frame_) {
    const auto tf = lookup_transform_matrix(tf_output_frame_, tf_input_frame_, cloud->header.stamp);
    if (!tf) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 1000, "Could not transform input frame %s -> %s",
        tf_input_frame_.c_str(), tf_output_frame_.c_str());
      return false;
    }
    postprocess_transform = *tf;
  }

  output = *cloud;
  output.data.resize(cloud->data.size());

  size_t output_size = 0;
  for (size_t global_offset = 0; global_offset + cloud->point_step <= cloud->data.size();
       global_offset += cloud->point_step) {
    Eigen::Vector4f point;
    std::memcpy(&point[0], &cloud->data[global_offset + offsets->x], sizeof(float));
    std::memcpy(&point[1], &cloud->data[global_offset + offsets->y], sizeof(float));
    std::memcpy(&point[2], &cloud->data[global_offset + offsets->z], sizeof(float));
    point[3] = 1.0F;

    if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2])) {
      continue;
    }

    const Eigen::Vector4f point_in_crop_frame = preprocess_transform * point;
    const bool point_is_inside =
      point_in_crop_frame[0] > param_.min_x && point_in_crop_frame[0] < param_.max_x &&
      point_in_crop_frame[1] > param_.min_y && point_in_crop_frame[1] < param_.max_y &&
      point_in_crop_frame[2] > param_.min_z && point_in_crop_frame[2] < param_.max_z;

    if ((!param_.negative && point_is_inside) || (param_.negative && !point_is_inside)) {
      std::memcpy(&output.data[output_size], &cloud->data[global_offset], cloud->point_step);

      const Eigen::Vector4f output_point =
        (tf_output_frame_ == tf_input_frame_) ? point_in_crop_frame : (postprocess_transform * point_in_crop_frame);

      std::memcpy(&output.data[output_size + offsets->x], &output_point[0], sizeof(float));
      std::memcpy(&output.data[output_size + offsets->y], &output_point[1], sizeof(float));
      std::memcpy(&output.data[output_size + offsets->z], &output_point[2], sizeof(float));

      output_size += cloud->point_step;
    }
  }

  output.data.resize(output_size);
  output.header.frame_id = tf_output_frame_.empty() ? cloud->header.frame_id : tf_output_frame_;
  output.height = 1;
  output.width = output.point_step == 0 ? 0U : static_cast<uint32_t>(output.data.size() / output.point_step);
  output.row_step = output.width * output.point_step;
  return true;
}

void CropBoxFilter::publish_crop_box_polygon()
{
  auto generate_point = [](double x, double y, double z) {
    geometry_msgs::msg::Point32 point;
    point.x = static_cast<float>(x);
    point.y = static_cast<float>(y);
    point.z = static_cast<float>(z);
    return point;
  };

  geometry_msgs::msg::PolygonStamped polygon_msg;
  polygon_msg.header.frame_id = tf_input_frame_;
  polygon_msg.header.stamp = now();

  const double x1 = param_.max_x;
  const double x2 = param_.min_x;
  const double x3 = param_.min_x;
  const double x4 = param_.max_x;
  const double y1 = param_.max_y;
  const double y2 = param_.max_y;
  const double y3 = param_.min_y;
  const double y4 = param_.min_y;
  const double z1 = param_.min_z;
  const double z2 = param_.max_z;

  polygon_msg.polygon.points.push_back(generate_point(x1, y1, z1));
  polygon_msg.polygon.points.push_back(generate_point(x2, y2, z1));
  polygon_msg.polygon.points.push_back(generate_point(x3, y3, z1));
  polygon_msg.polygon.points.push_back(generate_point(x4, y4, z1));
  polygon_msg.polygon.points.push_back(generate_point(x1, y1, z1));
  polygon_msg.polygon.points.push_back(generate_point(x1, y1, z2));
  polygon_msg.polygon.points.push_back(generate_point(x2, y2, z2));
  polygon_msg.polygon.points.push_back(generate_point(x2, y2, z1));
  polygon_msg.polygon.points.push_back(generate_point(x2, y2, z2));
  polygon_msg.polygon.points.push_back(generate_point(x3, y3, z2));
  polygon_msg.polygon.points.push_back(generate_point(x3, y3, z1));
  polygon_msg.polygon.points.push_back(generate_point(x3, y3, z2));
  polygon_msg.polygon.points.push_back(generate_point(x4, y4, z2));
  polygon_msg.polygon.points.push_back(generate_point(x4, y4, z1));
  polygon_msg.polygon.points.push_back(generate_point(x4, y4, z2));
  polygon_msg.polygon.points.push_back(generate_point(x1, y1, z2));

  crop_box_polygon_pub_->publish(std::move(polygon_msg));
}

rcl_interfaces::msg::SetParametersResult CropBoxFilter::param_callback(
  const std::vector<rclcpp::Parameter> & parameters)
{
  std::scoped_lock lock(mutex_);

  CropBoxParam new_param = param_;
  if (get_param(parameters, "min_x", new_param.min_x)) {}
  if (get_param(parameters, "max_x", new_param.max_x)) {}
  if (get_param(parameters, "min_y", new_param.min_y)) {}
  if (get_param(parameters, "max_y", new_param.max_y)) {}
  if (get_param(parameters, "min_z", new_param.min_z)) {}
  if (get_param(parameters, "max_z", new_param.max_z)) {}
  if (get_param(parameters, "negative", new_param.negative)) {}
  param_ = new_param;

  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  result.reason = "success";
  return result;
}

bool CropBoxFilter::is_valid(const PointCloud2ConstPtr & cloud) const
{
  if (!cloud) {
    return false;
  }

  if (!find_xyz_offsets(*cloud)) {
    RCLCPP_ERROR(get_logger(), "PointCloud2 must contain x/y/z fields");
    return false;
  }

  if (cloud->point_step == 0U) {
    RCLCPP_ERROR(get_logger(), "PointCloud2 point_step is zero");
    return false;
  }

  if (cloud->width * cloud->height * cloud->point_step != cloud->data.size()) {
    RCLCPP_WARN(
      get_logger(),
      "Invalid PointCloud2 layout: data=%zu width=%u height=%u point_step=%u",
      cloud->data.size(), cloud->width, cloud->height, cloud->point_step);
    return false;
  }

  return true;
}

std::optional<CropBoxFilter::FieldOffsets> CropBoxFilter::find_xyz_offsets(const PointCloud2 & cloud) const
{
  FieldOffsets offsets;
  bool has_x = false;
  bool has_y = false;
  bool has_z = false;

  for (const auto & field : cloud.fields) {
    if (field.name == "x" && field.datatype == sensor_msgs::msg::PointField::FLOAT32) {
      offsets.x = field.offset;
      has_x = true;
    } else if (field.name == "y" && field.datatype == sensor_msgs::msg::PointField::FLOAT32) {
      offsets.y = field.offset;
      has_y = true;
    } else if (field.name == "z" && field.datatype == sensor_msgs::msg::PointField::FLOAT32) {
      offsets.z = field.offset;
      has_z = true;
    }
  }

  if (!(has_x && has_y && has_z)) {
    return std::nullopt;
  }

  return offsets;
}

std::optional<Eigen::Matrix4f> CropBoxFilter::lookup_transform_matrix(
  const std::string & target_frame, const std::string & source_frame,
  const builtin_interfaces::msg::Time & stamp)
{
  if (target_frame == source_frame) {
    return Eigen::Matrix4f::Identity();
  }

  try {
    const auto transform = tf_buffer_->lookupTransform(
      target_frame, source_frame, rclcpp::Time(stamp), tf2::durationFromSec(0.2));
    return tf2::transformToEigen(transform).matrix().cast<float>();
  } catch (const tf2::TransformException & ex) {
    try {
      const auto latest_transform = tf_buffer_->lookupTransform(
        target_frame, source_frame, rclcpp::Time(0, 0, get_clock()->get_clock_type()),
        tf2::durationFromSec(0.2));
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 1000,
        "TF lookup at stamp failed from %s to %s, falling back to latest transform: %s",
        source_frame.c_str(), target_frame.c_str(), ex.what());
      return tf2::transformToEigen(latest_transform).matrix().cast<float>();
    } catch (const tf2::TransformException &) {
    }
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 1000, "TF lookup failed from %s to %s: %s",
      source_frame.c_str(), target_frame.c_str(), ex.what());
    return std::nullopt;
  }
}

}  // namespace carla::crop_box_filter

RCLCPP_COMPONENTS_REGISTER_NODE(carla::crop_box_filter::CropBoxFilter)
