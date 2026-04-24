#include "tod_autoware_bridge/tod_autoware_bridge_node.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

namespace tod_autoware_bridge
{

TodAutowareBridgeNode::TodAutowareBridgeNode()
: Node("tod_autoware_bridge_node"),
  last_tod_gear_(kTodNeutral),
  has_received_gear_command_(false)
{
  control_topic_ = this->declare_parameter<std::string>("control_topic", "/control/command/control");
  gear_topic_ = this->declare_parameter<std::string>("gear_topic", "/control/command/gear");
  tod_topic_ = this->declare_parameter<std::string>("tod_topic", "/tod_cmd");
  default_tod_gear_ = this->declare_parameter<int>("default_tod_gear", static_cast<int>(kTodNeutral));
  use_speed_sign_fallback_ = this->declare_parameter<bool>("use_speed_sign_fallback", true);
  speed_epsilon_ = this->declare_parameter<double>("speed_epsilon", 0.01);
  brake_accel_gain_ = this->declare_parameter<double>("brake_accel_gain", 50.0);
  max_brake_command_ = this->declare_parameter<int>("max_brake_command", 255);

  default_tod_gear_ = std::clamp(default_tod_gear_, static_cast<int>(kTodDisable), static_cast<int>(kTodDrive));
  max_brake_command_ = std::max(0, max_brake_command_);
  last_tod_gear_ = static_cast<std::uint8_t>(default_tod_gear_);

  tod_publisher_ = this->create_publisher<Float64MultiArray>(tod_topic_, 10);

  control_subscriber_ = this->create_subscription<AckermannControlCommand>(
    control_topic_, 10,
    std::bind(&TodAutowareBridgeNode::control_callback, this, std::placeholders::_1));

  gear_subscriber_ = this->create_subscription<GearCommand>(
    gear_topic_, 10,
    std::bind(&TodAutowareBridgeNode::gear_callback, this, std::placeholders::_1));

  RCLCPP_INFO(
    this->get_logger(),
    "Bridging '%s' + '%s' to '%s' with default TOD gear %d",
    control_topic_.c_str(), gear_topic_.c_str(), tod_topic_.c_str(), default_tod_gear_);
}

void TodAutowareBridgeNode::control_callback(const AckermannControlCommand::SharedPtr msg)
{
  if (!has_received_gear_command_ && use_speed_sign_fallback_) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(),
      *this->get_clock(),
      5000,
      "No /control/command/gear received yet. Falling back to speed-sign gear inference.");
  }

  Float64MultiArray tod_command;
  tod_command.data.resize(4);
  tod_command.data[0] = std::fabs(static_cast<double>(msg->longitudinal.speed));
  tod_command.data[1] = compute_brake_command(*msg);
  tod_command.data[2] = radians_to_degrees(static_cast<double>(msg->lateral.steering_tire_angle));
  tod_command.data[3] = static_cast<double>(resolve_tod_gear(*msg));

  tod_publisher_->publish(tod_command);
}

void TodAutowareBridgeNode::gear_callback(const GearCommand::SharedPtr msg)
{
  last_tod_gear_ = map_autoware_gear_to_tod(msg->command);
  has_received_gear_command_ = true;
}

std::uint8_t TodAutowareBridgeNode::map_autoware_gear_to_tod(std::uint8_t gear_command)
{
  switch (gear_command) {
    case GearCommand::NONE:
      return kTodDisable;
    case GearCommand::PARK:
      return kTodPark;
    case GearCommand::REVERSE:
    case GearCommand::REVERSE_2:
      return kTodReverse;
    case GearCommand::NEUTRAL:
      return kTodNeutral;
    case GearCommand::DRIVE:
    case GearCommand::DRIVE_2:
    case GearCommand::DRIVE_3:
    case GearCommand::DRIVE_4:
    case GearCommand::DRIVE_5:
    case GearCommand::DRIVE_6:
    case GearCommand::DRIVE_7:
    case GearCommand::DRIVE_8:
    case GearCommand::DRIVE_9:
    case GearCommand::DRIVE_10:
    case GearCommand::DRIVE_11:
    case GearCommand::DRIVE_12:
    case GearCommand::DRIVE_13:
    case GearCommand::DRIVE_14:
    case GearCommand::DRIVE_15:
    case GearCommand::DRIVE_16:
    case GearCommand::DRIVE_17:
    case GearCommand::DRIVE_18:
    case GearCommand::LOW:
    case GearCommand::LOW_2:
      return kTodDrive;
    default:
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        5000,
        "Unknown Autoware gear command %u. Using default TOD gear %d.",
        gear_command,
        default_tod_gear_);
      return static_cast<std::uint8_t>(default_tod_gear_);
  }
}

std::uint8_t TodAutowareBridgeNode::resolve_tod_gear(const AckermannControlCommand & msg) const
{
  if (has_received_gear_command_) {
    return last_tod_gear_;
  }

  if (!use_speed_sign_fallback_) {
    return static_cast<std::uint8_t>(default_tod_gear_);
  }

  if (msg.longitudinal.speed < -speed_epsilon_) {
    return kTodReverse;
  }

  if (msg.longitudinal.speed > speed_epsilon_) {
    return kTodDrive;
  }

  return static_cast<std::uint8_t>(default_tod_gear_);
}

double TodAutowareBridgeNode::compute_brake_command(const AckermannControlCommand & msg) const
{
  if (msg.longitudinal.acceleration >= 0.0F) {
    return 0.0;
  }

  const double brake_command =
    -static_cast<double>(msg.longitudinal.acceleration) * brake_accel_gain_;
  return std::clamp(brake_command, 0.0, static_cast<double>(max_brake_command_));
}

double TodAutowareBridgeNode::radians_to_degrees(double radians)
{
  constexpr double kPi = 3.14159265358979323846;
  return radians * 180.0 / kPi;
}

}  // namespace tod_autoware_bridge
