#include "rclcpp/rclcpp.hpp"
#include "tod_autoware_bridge/tod_autoware_bridge_node.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<tod_autoware_bridge::TodAutowareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
