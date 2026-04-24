#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "autoware_auto_control_msgs/msg/ackermann_control_command.hpp"
#include "builtin_interfaces/msg/time.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

namespace yhs
{

class TodAutowareBridgeNode : public rclcpp::Node
{
public:
  TodAutowareBridgeNode()
  : Node("tod_autoware_bridge_node")
  {
    tod_topic_ = declare_parameter<std::string>("tod_topic", "/tod_cmd");
    autoware_control_topic_ = declare_parameter<std::string>(
      "autoware_control_topic", "/control/command/control_cmd");
    enable_autoware_to_tod_ = declare_parameter<bool>("enable_autoware_to_tod", true);
    enable_tod_to_autoware_ = declare_parameter<bool>("enable_tod_to_autoware", false);
    forward_gear_ = declare_parameter<int>("forward_gear", 1);
    reverse_gear_ = declare_parameter<int>("reverse_gear", 2);
    force_drive_gear_ = declare_parameter<bool>("force_drive_gear", false);
    brake_from_negative_accel_gain_ = declare_parameter<double>(
      "brake_from_negative_accel_gain", 20.0);
    brake_to_negative_accel_gain_ = declare_parameter<double>(
      "brake_to_negative_accel_gain", 0.05);
    max_brake_command_ = declare_parameter<double>("max_brake_command", 100.0);
    self_echo_suppression_ms_ = declare_parameter<int>("self_echo_suppression_ms", 50);

    const auto qos = rclcpp::QoS(rclcpp::KeepLast(10));

    if (enable_autoware_to_tod_) {
      tod_publisher_ = create_publisher<std_msgs::msg::Float64MultiArray>(tod_topic_, qos);
      autoware_subscriber_ =
        create_subscription<autoware_auto_control_msgs::msg::AckermannControlCommand>(
        autoware_control_topic_, qos,
        std::bind(&TodAutowareBridgeNode::on_autoware_command, this, std::placeholders::_1));
    }

    if (enable_tod_to_autoware_) {
      autoware_publisher_ =
        create_publisher<autoware_auto_control_msgs::msg::AckermannControlCommand>(
        autoware_control_topic_, qos);
      tod_subscriber_ = create_subscription<std_msgs::msg::Float64MultiArray>(
        tod_topic_, qos, std::bind(&TodAutowareBridgeNode::on_tod_command, this, std::placeholders::_1));
    }

    RCLCPP_INFO(
      get_logger(),
      "tod_autoware_bridge_node started: autoware_to_tod=%s, tod_to_autoware=%s, "
      "tod_topic=%s, autoware_control_topic=%s",
      enable_autoware_to_tod_ ? "true" : "false",
      enable_tod_to_autoware_ ? "true" : "false",
      tod_topic_.c_str(),
      autoware_control_topic_.c_str());
  }

private:
  static constexpr double kRadToDeg = 180.0 / M_PI;
  static constexpr double kDegToRad = M_PI / 180.0;
  static constexpr double kCompareEpsilon = 1e-6;

  static double clamp(const double value, const double min_value, const double max_value)
  {
    return std::max(min_value, std::min(value, max_value));
  }

  static bool almost_equal(const double lhs, const double rhs)
  {
    return std::abs(lhs - rhs) <= kCompareEpsilon;
  }

  void on_autoware_command(
    const autoware_auto_control_msgs::msg::AckermannControlCommand::SharedPtr msg)
  {
    if (!tod_publisher_) {
      return;
    }

    if (is_recent_autoware_echo(*msg)) {
      return;
    }

    std_msgs::msg::Float64MultiArray tod_msg;
    tod_msg.data.resize(4);

    const double signed_speed = static_cast<double>(msg->longitudinal.speed);
    const double signed_acceleration = static_cast<double>(msg->longitudinal.acceleration);
    const double brake_command = acceleration_to_brake(signed_speed, signed_acceleration);

    tod_msg.data[0] = std::abs(signed_speed);
    tod_msg.data[1] = brake_command;
    tod_msg.data[2] = static_cast<double>(msg->lateral.steering_tire_angle) * kRadToDeg;
    tod_msg.data[3] = force_drive_gear_ ? forward_gear_ :
      (signed_speed < 0.0 ? reverse_gear_ : forward_gear_);

    last_tod_command_ = {tod_msg.data[0], tod_msg.data[1], tod_msg.data[2], tod_msg.data[3]};
    last_tod_publish_time_ = get_clock()->now();
    has_last_tod_command_ = true;
    tod_publisher_->publish(tod_msg);
  }

  void on_tod_command(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
  {
    if (!autoware_publisher_) {
      return;
    }

    if (msg->data.size() < 4) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Ignored /tod_cmd message because data size is %zu, expected at least 4",
        msg->data.size());
      return;
    }

    if (is_recent_tod_echo(*msg)) {
      return;
    }

    const double speed = std::max(0.0, msg->data[0]);
    const double brake = clamp(msg->data[1], 0.0, max_brake_command_);
    const double steering_deg = msg->data[2];
    const int gear = static_cast<int>(std::lround(msg->data[3]));
    const bool reverse = gear == reverse_gear_;
    const auto now = now_msg();

    autoware_auto_control_msgs::msg::AckermannControlCommand autoware_msg;
    autoware_msg.stamp = now;
    autoware_msg.longitudinal.stamp = now;
    autoware_msg.longitudinal.speed = static_cast<float>(reverse ? -speed : speed);
    autoware_msg.longitudinal.acceleration = static_cast<float>(brake_to_acceleration(brake));
    autoware_msg.longitudinal.jerk = 0.0F;
    autoware_msg.lateral.stamp = now;
    autoware_msg.lateral.steering_tire_angle = static_cast<float>(steering_deg * kDegToRad);
    autoware_msg.lateral.steering_tire_rotation_rate = 0.0F;

    last_autoware_speed_ = static_cast<double>(autoware_msg.longitudinal.speed);
    last_autoware_acceleration_ = static_cast<double>(autoware_msg.longitudinal.acceleration);
    last_autoware_steering_ = static_cast<double>(autoware_msg.lateral.steering_tire_angle);
    last_autoware_publish_time_ = get_clock()->now();
    has_last_autoware_command_ = true;
    autoware_publisher_->publish(autoware_msg);
  }

  builtin_interfaces::msg::Time now_msg()
  {
    return get_clock()->now();
  }

  double acceleration_to_brake(const double acceleration) const
  {
    if (acceleration >= 0.0) {
      return 0.0;
    }

    return clamp(-acceleration * brake_from_negative_accel_gain_, 0.0, max_brake_command_);
  }

  double acceleration_to_brake(
    const double signed_speed, const double signed_acceleration) const
  {
    if (almost_equal(signed_speed, 0.0)) {
      return acceleration_to_brake(signed_acceleration);
    }

    // In reverse, Autoware uses negative acceleration to speed the vehicle up backward.
    // Only convert acceleration into brake when it opposes the requested travel direction.
    const bool acceleration_opposes_motion =
      (signed_speed > 0.0 && signed_acceleration < 0.0) ||
      (signed_speed < 0.0 && signed_acceleration > 0.0);

    if (!acceleration_opposes_motion) {
      return 0.0;
    }

    return clamp(
      std::abs(signed_acceleration) * brake_from_negative_accel_gain_, 0.0, max_brake_command_);
  }

  double brake_to_acceleration(const double brake) const
  {
    if (brake <= 0.0) {
      return 0.0;
    }

    return -brake * brake_to_negative_accel_gain_;
  }

  bool is_recent_tod_echo(const std_msgs::msg::Float64MultiArray & msg)
  {
    if (!has_last_tod_command_ || msg.data.size() < 4) {
      return false;
    }

    const auto age = get_clock()->now() - last_tod_publish_time_;
    if (age.nanoseconds() > static_cast<int64_t>(self_echo_suppression_ms_) * 1000000LL) {
      return false;
    }

    return almost_equal(msg.data[0], last_tod_command_[0]) &&
           almost_equal(msg.data[1], last_tod_command_[1]) &&
           almost_equal(msg.data[2], last_tod_command_[2]) &&
           almost_equal(msg.data[3], last_tod_command_[3]);
  }

  bool is_recent_autoware_echo(
    const autoware_auto_control_msgs::msg::AckermannControlCommand & msg)
  {
    if (!has_last_autoware_command_) {
      return false;
    }

    const auto age = get_clock()->now() - last_autoware_publish_time_;
    if (age.nanoseconds() > static_cast<int64_t>(self_echo_suppression_ms_) * 1000000LL) {
      return false;
    }

    return almost_equal(msg.longitudinal.speed, last_autoware_speed_) &&
           almost_equal(msg.longitudinal.acceleration, last_autoware_acceleration_) &&
           almost_equal(msg.lateral.steering_tire_angle, last_autoware_steering_);
  }

  std::string tod_topic_;
  std::string autoware_control_topic_;
  bool enable_autoware_to_tod_;
  bool enable_tod_to_autoware_;
  int forward_gear_;
  int reverse_gear_;
  bool force_drive_gear_;
  double brake_from_negative_accel_gain_;
  double brake_to_negative_accel_gain_;
  double max_brake_command_;
  int self_echo_suppression_ms_;

  bool has_last_tod_command_{false};
  std::array<double, 4> last_tod_command_{};
  rclcpp::Time last_tod_publish_time_{0, 0, RCL_ROS_TIME};
  bool has_last_autoware_command_{false};
  double last_autoware_speed_{0.0};
  double last_autoware_acceleration_{0.0};
  double last_autoware_steering_{0.0};
  rclcpp::Time last_autoware_publish_time_{0, 0, RCL_ROS_TIME};

  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr tod_publisher_;
  rclcpp::Publisher<autoware_auto_control_msgs::msg::AckermannControlCommand>::SharedPtr
    autoware_publisher_;
  rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr tod_subscriber_;
  rclcpp::Subscription<autoware_auto_control_msgs::msg::AckermannControlCommand>::SharedPtr
    autoware_subscriber_;
};

}  // namespace yhs

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<yhs::TodAutowareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
