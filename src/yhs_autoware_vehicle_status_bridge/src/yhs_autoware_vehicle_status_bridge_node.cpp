#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "autoware_auto_vehicle_msgs/msg/control_mode_report.hpp"
#include "autoware_auto_vehicle_msgs/msg/gear_report.hpp"
#include "autoware_auto_vehicle_msgs/msg/steering_report.hpp"
#include "autoware_auto_vehicle_msgs/msg/velocity_report.hpp"
#include "rclcpp/rclcpp.hpp"
#include "yhs_can_interfaces/msg/chassis_info_fb.hpp"

namespace yhs
{

class YhsAutowareVehicleStatusBridgeNode : public rclcpp::Node
{
public:
  YhsAutowareVehicleStatusBridgeNode()
  : Node("yhs_autoware_vehicle_status_bridge_node")
  {
    chassis_info_topic_ = declare_parameter<std::string>(
      "chassis_info_topic", "/chassis_info_fb");
    gear_output_topic_ = declare_parameter<std::string>(
      "gear_output_topic", "/vehicle/status/gear_status");
    velocity_output_topic_ = declare_parameter<std::string>(
      "velocity_output_topic", "/vehicle/status/velocity_status");
    steering_output_topic_ = declare_parameter<std::string>(
      "steering_output_topic", "/vehicle/status/steering_status");
    control_mode_output_topic_ = declare_parameter<std::string>(
      "control_mode_output_topic", "/vehicle/status/control_mode");

    park_gear_value_ = declare_parameter<int>("park_gear_value", 1);
    reverse_gear_value_ = declare_parameter<int>("reverse_gear_value", 2);
    neutral_gear_value_ = declare_parameter<int>("neutral_gear_value", 3);
    forward_gear_value_ = declare_parameter<int>("forward_gear_value", 4);
    disabled_gear_value_ = declare_parameter<int>("disabled_gear_value", 0);

    autonomous_mode_value_ = declare_parameter<int>("autonomous_mode_value", 1);
    manual_mode_value_ = declare_parameter<int>("manual_mode_value", 0);
    use_veh_diag_for_control_mode_ = declare_parameter<bool>(
      "use_veh_diag_for_control_mode", true);

    const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();

    gear_pub_ = create_publisher<autoware_auto_vehicle_msgs::msg::GearReport>(
      gear_output_topic_, qos);
    velocity_pub_ = create_publisher<autoware_auto_vehicle_msgs::msg::VelocityReport>(
      velocity_output_topic_, qos);
    steering_pub_ = create_publisher<autoware_auto_vehicle_msgs::msg::SteeringReport>(
      steering_output_topic_, qos);
    control_mode_pub_ =
      create_publisher<autoware_auto_vehicle_msgs::msg::ControlModeReport>(
      control_mode_output_topic_, qos);

    chassis_info_sub_ = create_subscription<yhs_can_interfaces::msg::ChassisInfoFb>(
      chassis_info_topic_, qos,
      std::bind(&YhsAutowareVehicleStatusBridgeNode::on_chassis_info, this, std::placeholders::_1));

    RCLCPP_INFO(get_logger(), "YHS Autoware vehicle status bridge started.");
    RCLCPP_INFO(get_logger(), "  chassis_info : %s", chassis_info_topic_.c_str());
    RCLCPP_INFO(get_logger(), "  gear         : %s", gear_output_topic_.c_str());
    RCLCPP_INFO(get_logger(), "  velocity     : %s", velocity_output_topic_.c_str());
    RCLCPP_INFO(get_logger(), "  steering     : %s", steering_output_topic_.c_str());
    RCLCPP_INFO(get_logger(), "  control_mode : %s", control_mode_output_topic_.c_str());
  }

private:
  void on_chassis_info(const yhs_can_interfaces::msg::ChassisInfoFb::SharedPtr msg)
  {
    publish_gear(*msg);
    publish_velocity(*msg);
    publish_steering(*msg);
    publish_control_mode(*msg);
  }

  void publish_gear(const yhs_can_interfaces::msg::ChassisInfoFb & msg)
  {
    autoware_auto_vehicle_msgs::msg::GearReport out{};
    out.stamp = msg.header.stamp;
    out.report = convert_gear(msg.ctrl_fb.ctrl_fb_gear);
    gear_pub_->publish(out);
  }

  void publish_velocity(const yhs_can_interfaces::msg::ChassisInfoFb & msg)
  {
    autoware_auto_vehicle_msgs::msg::VelocityReport out{};
    out.header = msg.header;
    out.longitudinal_velocity = static_cast<float>(signed_velocity(msg));
    out.lateral_velocity = 0.0F;
    out.heading_rate = 0.0F;
    velocity_pub_->publish(out);
  }

  void publish_steering(const yhs_can_interfaces::msg::ChassisInfoFb & msg)
  {
    autoware_auto_vehicle_msgs::msg::SteeringReport out{};
    out.stamp = msg.header.stamp;
    out.steering_tire_angle = static_cast<float>(msg.ctrl_fb.ctrl_fb_steering * kDegToRad);
    steering_pub_->publish(out);
  }

  void publish_control_mode(const yhs_can_interfaces::msg::ChassisInfoFb & msg)
  {
    autoware_auto_vehicle_msgs::msg::ControlModeReport out{};
    out.stamp = msg.header.stamp;
    out.mode = convert_control_mode(msg);
    control_mode_pub_->publish(out);
  }

  double signed_velocity(const yhs_can_interfaces::msg::ChassisInfoFb & msg) const
  {
    double velocity = static_cast<double>(msg.ctrl_fb.ctrl_fb_velocity);

    if (static_cast<int>(msg.ctrl_fb.ctrl_fb_gear) == reverse_gear_value_) {
      velocity = -velocity;
    }

    return velocity;
  }

  uint8_t convert_gear(const uint8_t gear) const
  {
    using GearReport = autoware_auto_vehicle_msgs::msg::GearReport;

    const int gear_value = static_cast<int>(gear);
    if (gear_value == forward_gear_value_) {
      return GearReport::DRIVE;
    }
    if (gear_value == reverse_gear_value_) {
      return GearReport::REVERSE;
    }
    if (gear_value == neutral_gear_value_) {
      return GearReport::NEUTRAL;
    }
    if (park_gear_value_ >= 0 && gear_value == park_gear_value_) {
      return GearReport::PARK;
    }
    if (gear_value == disabled_gear_value_) {
      return GearReport::NONE;
    }

    return GearReport::NONE;
  }

  uint8_t convert_control_mode(const yhs_can_interfaces::msg::ChassisInfoFb & msg) const
  {
    using ControlModeReport = autoware_auto_vehicle_msgs::msg::ControlModeReport;

    if (use_veh_diag_for_control_mode_) {
      if (msg.veh_diag_fb.veh_fb_auto_can_ctrl_cmd || msg.veh_diag_fb.veh_fb_auto_io_can_cmd) {
        return ControlModeReport::AUTONOMOUS;
      }
      return ControlModeReport::MANUAL;
    }

    const int mode = static_cast<int>(msg.ctrl_fb.ctrl_fb_mode);
    if (mode == autonomous_mode_value_) {
      return ControlModeReport::AUTONOMOUS;
    }
    if (mode == manual_mode_value_) {
      return ControlModeReport::MANUAL;
    }

    return ControlModeReport::NO_COMMAND;
  }

  static constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

  std::string chassis_info_topic_;
  std::string gear_output_topic_;
  std::string velocity_output_topic_;
  std::string steering_output_topic_;
  std::string control_mode_output_topic_;

  int forward_gear_value_;
  int reverse_gear_value_;
  int neutral_gear_value_;
  int park_gear_value_;
  int disabled_gear_value_;
  int autonomous_mode_value_;
  int manual_mode_value_;
  bool use_veh_diag_for_control_mode_;

  rclcpp::Subscription<yhs_can_interfaces::msg::ChassisInfoFb>::SharedPtr chassis_info_sub_;
  rclcpp::Publisher<autoware_auto_vehicle_msgs::msg::GearReport>::SharedPtr gear_pub_;
  rclcpp::Publisher<autoware_auto_vehicle_msgs::msg::VelocityReport>::SharedPtr velocity_pub_;
  rclcpp::Publisher<autoware_auto_vehicle_msgs::msg::SteeringReport>::SharedPtr steering_pub_;
  rclcpp::Publisher<autoware_auto_vehicle_msgs::msg::ControlModeReport>::SharedPtr
    control_mode_pub_;
};

}  // namespace yhs

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<yhs::YhsAutowareVehicleStatusBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
