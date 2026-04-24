#include "besfoc_driver/besfoc_driver.hpp"
#include <rclcpp/rclcpp.hpp>
#include <memory>

int main()
{
    // Example usage of CanMotor class

    auto bus = std::make_shared<CanBus>("vcan0");
    bus->connect();

    //besfoc::CanMotor motor4(0x68, bus);
    besfoc::CanMotor motor1(0x65, bus);

    // motor4.set_mode(besfoc::SPEED_MODE); // Set mode to velocity control

    // motor4.set_velocity(100);

    motor1.set_mode(besfoc::POSITION_MODE); // Set mode to position control
    motor1.set_position_relative(10000, 100); // Move to position 100


    RCLCPP_INFO(rclcpp::get_logger("test_node"), "CanMotor initialized successfully.");

    bus->disconnect();

     RCLCPP_INFO(rclcpp::get_logger("test_node"), "CanBus disconnected successfully.");
    
    return 0;
}