#include "besfoc_driver/besfoc_driver.hpp"
#include <rclcpp/rclcpp.hpp>


int main()
{
    // Example usage of CanMotor class

    CanBus bus("vcan0");

    besfoc::CanMotor motor4(0x68, bus);
    besfoc::CanMotor motor1(0x65, bus);

    RCLCPP_INFO(rclcpp::get_logger("test_node"), "CanMotor initialized successfully.");

    motor4.stop();
    motor1.stop();
    
    return 0;
}