#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "geometry_msgs/msg/twist_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

namespace gungnir_controller
{

class JoystickServo : public rclcpp::Node
{
public:
  JoystickServo()
  : Node("joystick_servo")
  {
    const auto joy_topic = declare_parameter<std::string>("joy_topic", "/joy");
    const auto twist_topic =
      declare_parameter<std::string>("twist_topic", "/servo_node/delta_twist_cmds");
    command_frame_ = declare_parameter<std::string>("command_frame", "base_link");
    servo_start_service_ =
      declare_parameter<std::string>("servo_start_service", "/servo_node/start_servo");

    deadman_button_ = declare_parameter<std::int64_t>("deadman_button", 4);
    deadzone_ = declare_parameter<double>("deadzone", 0.1);
    auto_start_servo_ = declare_parameter<bool>("auto_start_servo", true);

    linear_x_ = declare_axis("linear_x", 1, 1.0);
    linear_y_ = declare_axis("linear_y", 0, 1.0);
    linear_z_ = declare_axis("linear_z", 4, 1.0);
    angular_x_ = declare_axis("angular_x", -1, 0.0);
    angular_y_ = declare_axis("angular_y", -1, 0.0);
    angular_z_ = declare_axis("angular_z", 3, 0.0);

    if (deadzone_ < 0.0 || deadzone_ >= 1.0) {
      throw std::invalid_argument("deadzone must be in the range [0.0, 1.0)");
    }
    if (deadman_button_ < 0) {
      throw std::invalid_argument("deadman_button must be non-negative");
    }

    twist_pub_ = create_publisher<geometry_msgs::msg::TwistStamped>(
      twist_topic, rclcpp::SystemDefaultsQoS());
    joy_sub_ = create_subscription<sensor_msgs::msg::Joy>(
      joy_topic, rclcpp::SensorDataQoS(),
      std::bind(&JoystickServo::joy_callback, this, std::placeholders::_1));

    if (auto_start_servo_) {
      servo_start_client_ = create_client<std_srvs::srv::Trigger>(servo_start_service_);
      start_timer_ = create_wall_timer(500ms, std::bind(&JoystickServo::try_start_servo, this));
    }

    RCLCPP_INFO(
      get_logger(),
      "Joystick teleoperation ready; hold button %ld to command motion in frame '%s'",
      static_cast<long>(deadman_button_), command_frame_.c_str());
  }

private:
  struct AxisMapping
  {
    std::int64_t index;
    double scale;
  };

  AxisMapping declare_axis(
    const std::string & name, std::int64_t default_index, double default_scale)
  {
    return {
      declare_parameter<std::int64_t>(name + ".axis", default_index),
      declare_parameter<double>(name + ".scale", default_scale),
    };
  }

  double read_axis(const sensor_msgs::msg::Joy & joy, const AxisMapping & mapping) const
  {
    if (mapping.index < 0 || static_cast<std::size_t>(mapping.index) >= joy.axes.size()) {
      return 0.0;
    }

    const double raw = std::clamp(static_cast<double>(joy.axes[mapping.index]), -1.0, 1.0);
    if (std::abs(raw) <= deadzone_) {
      return 0.0;
    }

    // Remove the discontinuity at the edge of the deadzone while preserving full scale.
    const double normalized = std::copysign(
      (std::abs(raw) - deadzone_) / (1.0 - deadzone_), raw);
    return std::clamp(normalized * mapping.scale, -1.0, 1.0);
  }

  void joy_callback(const sensor_msgs::msg::Joy::ConstSharedPtr joy)
  {
    if (static_cast<std::size_t>(deadman_button_) >= joy->buttons.size()) {
      RCLCPP_ERROR_THROTTLE(
        get_logger(), *get_clock(), 5000,
        "Joystick reports %zu buttons, but deadman_button is configured as %ld",
        joy->buttons.size(), static_cast<long>(deadman_button_));
      publish_zero_if_needed();
      return;
    }

    const bool enabled = joy->buttons[deadman_button_] != 0;
    if (!enabled) {
      publish_zero_if_needed();
      return;
    }

    geometry_msgs::msg::TwistStamped command;
    command.header.stamp = now();
    command.header.frame_id = command_frame_;
    command.twist.linear.x = read_axis(*joy, linear_x_);
    command.twist.linear.y = read_axis(*joy, linear_y_);
    command.twist.linear.z = read_axis(*joy, linear_z_);
    command.twist.angular.x = read_axis(*joy, angular_x_);
    command.twist.angular.y = read_axis(*joy, angular_y_);
    command.twist.angular.z = read_axis(*joy, angular_z_);
    twist_pub_->publish(command);
    deadman_was_pressed_ = true;
  }

  void publish_zero_if_needed()
  {
    if (!deadman_was_pressed_) {
      return;
    }

    geometry_msgs::msg::TwistStamped command;
    command.header.stamp = now();
    command.header.frame_id = command_frame_;
    twist_pub_->publish(command);
    deadman_was_pressed_ = false;
  }

  void try_start_servo()
  {
    if (servo_started_ || start_request_pending_ || !servo_start_client_->service_is_ready()) {
      return;
    }

    start_request_pending_ = true;
    auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
    servo_start_client_->async_send_request(
      request,
      [this](rclcpp::Client<std_srvs::srv::Trigger>::SharedFuture future) {
        start_request_pending_ = false;
        const auto response = future.get();
        if (response->success) {
          servo_started_ = true;
          start_timer_->cancel();
          RCLCPP_INFO(get_logger(), "MoveIt Servo started");
        } else {
          RCLCPP_WARN(get_logger(), "MoveIt Servo did not start: %s", response->message.c_str());
        }
      });
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_pub_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr servo_start_client_;
  rclcpp::TimerBase::SharedPtr start_timer_;

  std::string command_frame_;
  std::string servo_start_service_;
  std::int64_t deadman_button_;
  double deadzone_;
  bool auto_start_servo_;
  bool deadman_was_pressed_{ false };
  bool start_request_pending_{ false };
  bool servo_started_{ false };
  AxisMapping linear_x_;
  AxisMapping linear_y_;
  AxisMapping linear_z_;
  AxisMapping angular_x_;
  AxisMapping angular_y_;
  AxisMapping angular_z_;
};

}  // namespace gungnir_controller

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<gungnir_controller::JoystickServo>());
  rclcpp::shutdown();
  return 0;
}
