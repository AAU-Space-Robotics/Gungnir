#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "geometry_msgs/msg/twist_stamped.hpp"
#include "moveit_msgs/srv/change_control_dimensions.hpp"
#include "moveit_msgs/srv/change_drift_dimensions.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "std_msgs/msg/int8.hpp"
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
    const auto control_dimensions_service = declare_parameter<std::string>(
      "control_dimensions_service", "/servo_node/change_control_dimensions");
    const auto drift_dimensions_service = declare_parameter<std::string>(
      "drift_dimensions_service", "/servo_node/change_drift_dimensions");
    const auto status_topic =
      declare_parameter<std::string>("status_topic", "/servo_node/status");

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
    status_sub_ = create_subscription<std_msgs::msg::Int8>(
      status_topic, rclcpp::SystemDefaultsQoS(),
      std::bind(&JoystickServo::status_callback, this, std::placeholders::_1));

    if (auto_start_servo_) {
      servo_start_client_ = create_client<std_srvs::srv::Trigger>(servo_start_service_);
      control_dimensions_client_ =
        create_client<moveit_msgs::srv::ChangeControlDimensions>(control_dimensions_service);
      drift_dimensions_client_ =
        create_client<moveit_msgs::srv::ChangeDriftDimensions>(drift_dimensions_service);
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
    if (!received_joy_) {
      received_joy_ = true;
      RCLCPP_INFO(
        get_logger(), "Receiving joystick data with %zu axes and %zu buttons",
        joy->axes.size(), joy->buttons.size());
    }

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

  void status_callback(const std_msgs::msg::Int8::ConstSharedPtr status)
  {
    if (status->data == last_servo_status_) {
      return;
    }
    last_servo_status_ = status->data;

    const char * description = "unknown status";
    switch (status->data) {
      case 0: description = "ready"; break;
      case 1: description = "approaching singularity; decelerating"; break;
      case 2: description = "halted at singularity"; break;
      case 3: description = "approaching collision; decelerating"; break;
      case 4: description = "halted for collision"; break;
      case 5: description = "halted at joint bound"; break;
      case 6: description = "leaving singularity; decelerating"; break;
      default: break;
    }

    if (status->data == 0) {
      RCLCPP_INFO(get_logger(), "MoveIt Servo status: %d (%s)", status->data, description);
    } else {
      RCLCPP_WARN(get_logger(), "MoveIt Servo status: %d (%s)", status->data, description);
    }
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
    if (control_dimensions_configured_ && !drift_dimensions_configured_) {
      try_configure_drift_dimensions();
      return;
    }
    if (servo_started_ && !control_dimensions_configured_) {
      try_configure_control_dimensions();
      return;
    }
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
          RCLCPP_INFO(get_logger(), "MoveIt Servo started");
        } else {
          RCLCPP_WARN(get_logger(), "MoveIt Servo did not start: %s", response->message.c_str());
        }
      });
  }

  void try_configure_control_dimensions()
  {
    if (control_dimensions_request_pending_ || !control_dimensions_client_->service_is_ready()) {
      return;
    }

    control_dimensions_request_pending_ = true;
    auto request = std::make_shared<moveit_msgs::srv::ChangeControlDimensions::Request>();
    request->control_x_translation = true;
    request->control_y_translation = true;
    request->control_z_translation = true;
    request->control_x_rotation = false;
    request->control_y_rotation = false;
    request->control_z_rotation = false;
    control_dimensions_client_->async_send_request(
      request,
      [this](rclcpp::Client<moveit_msgs::srv::ChangeControlDimensions>::SharedFuture future) {
        control_dimensions_request_pending_ = false;
        if (future.get()->success) {
          control_dimensions_configured_ = true;
          RCLCPP_INFO(get_logger(), "MoveIt Servo configured for X/Y/Z translation only");
        } else {
          RCLCPP_WARN(get_logger(), "MoveIt Servo rejected the Cartesian dimension configuration");
        }
      });
  }

  void try_configure_drift_dimensions()
  {
    if (drift_dimensions_request_pending_ || !drift_dimensions_client_->service_is_ready()) {
      return;
    }

    drift_dimensions_request_pending_ = true;
    auto request = std::make_shared<moveit_msgs::srv::ChangeDriftDimensions::Request>();
    request->drift_x_translation = false;
    request->drift_y_translation = false;
    request->drift_z_translation = false;
    request->drift_x_rotation = true;
    request->drift_y_rotation = true;
    request->drift_z_rotation = true;
    request->transform_jog_frame_to_drift_frame.rotation.w = 1.0;
    drift_dimensions_client_->async_send_request(
      request,
      [this](rclcpp::Client<moveit_msgs::srv::ChangeDriftDimensions>::SharedFuture future) {
        drift_dimensions_request_pending_ = false;
        if (future.get()->success) {
          drift_dimensions_configured_ = true;
          start_timer_->cancel();
          RCLCPP_INFO(
            get_logger(),
            "MoveIt Servo configured to let unsupported orientation dimensions drift");
        } else {
          RCLCPP_WARN(get_logger(), "MoveIt Servo rejected the drift dimension configuration");
        }
      });
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
  rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr status_sub_;
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_pub_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr servo_start_client_;
  rclcpp::Client<moveit_msgs::srv::ChangeControlDimensions>::SharedPtr
    control_dimensions_client_;
  rclcpp::Client<moveit_msgs::srv::ChangeDriftDimensions>::SharedPtr drift_dimensions_client_;
  rclcpp::TimerBase::SharedPtr start_timer_;

  std::string command_frame_;
  std::string servo_start_service_;
  std::int64_t deadman_button_;
  double deadzone_;
  bool auto_start_servo_;
  bool deadman_was_pressed_{ false };
  bool start_request_pending_{ false };
  bool control_dimensions_request_pending_{ false };
  bool drift_dimensions_request_pending_{ false };
  bool servo_started_{ false };
  bool control_dimensions_configured_{ false };
  bool drift_dimensions_configured_{ false };
  bool received_joy_{ false };
  std::int8_t last_servo_status_{ -127 };
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
