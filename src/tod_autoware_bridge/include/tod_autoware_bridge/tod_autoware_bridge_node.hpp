#ifndef TOD_AUTOWARE_BRIDGE__TOD_AUTOWARE_BRIDGE_NODE_HPP_
#define TOD_AUTOWARE_BRIDGE__TOD_AUTOWARE_BRIDGE_NODE_HPP_

#include <cstdint>
#include <string>

#include "autoware_auto_control_msgs/msg/ackermann_control_command.hpp"
#include "autoware_auto_vehicle_msgs/msg/gear_command.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

namespace tod_autoware_bridge
{

class TodAutowareBridgeNode : public rclcpp::Node
{
public:
  TodAutowareBridgeNode();

private:
  using AckermannControlCommand = autoware_auto_control_msgs::msg::AckermannControlCommand;
  using GearCommand = autoware_auto_vehicle_msgs::msg::GearCommand;
  using Float64MultiArray = std_msgs::msg::Float64MultiArray;

  static constexpr std::uint8_t kTodDisable = 0;
  static constexpr std::uint8_t kTodPark = 1;
  static constexpr std::uint8_t kTodReverse = 2;
  static constexpr std::uint8_t kTodNeutral = 3;
  static constexpr std::uint8_t kTodDrive = 4;

  void control_callback(const AckermannControlCommand::SharedPtr msg);
  void gear_callback(const GearCommand::SharedPtr msg);

  std::uint8_t map_autoware_gear_to_tod(std::uint8_t gear_command);
  std::uint8_t resolve_tod_gear(const AckermannControlCommand & msg) const;
  double compute_brake_command(const AckermannControlCommand & msg) const;
  static double radians_to_degrees(double radians);

  rclcpp::Subscription<AckermannControlCommand>::SharedPtr control_subscriber_;
  rclcpp::Subscription<GearCommand>::SharedPtr gear_subscriber_;
  rclcpp::Publisher<Float64MultiArray>::SharedPtr tod_publisher_;

  std::uint8_t last_tod_gear_;
  bool has_received_gear_command_;

  std::string control_topic_;
  std::string gear_topic_;
  std::string tod_topic_;
  int default_tod_gear_;
  bool use_speed_sign_fallback_;
  double speed_epsilon_;
  double brake_accel_gain_;
  int max_brake_command_;
};

}  // namespace tod_autoware_bridge

#endif  // TOD_AUTOWARE_BRIDGE__TOD_AUTOWARE_BRIDGE_NODE_HPP_
