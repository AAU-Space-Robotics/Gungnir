#include "besfoc_driver/besfoc_driver.hpp"
#include "amt21_encoder_driver/amt21_encoder_driver.hpp"
#include <rclcpp/rclcpp.hpp>
#include <memory>

void test_relative_postion(besfoc::CanMotor motor, AMT21 encoder){
    motor.set_mode(besfoc::POSITION_MODE); // Set mode to position control
    
    motor.set_position_absolute(10000, 1000); // Move to position 100


}

void test_velocity(besfoc::CanMotor& motor, AMT21& encoder){
    int motor_status;
    int velocity;

    motor.set_acceleration(10000); // Set acceleration to 1000 rpm/s
    motor.set_deceleration(10000); // Set deceleration to 1000 rpm
    motor.set_mode(besfoc::SPEED_MODE); // Set mode to velocity control
   

    motor.set_velocity(1000); // Set velocity to 1000

    //Delay
    int time = 5*100; // 5 seconds at 100ms sleep intervals
    while(time > 0){
        
        int encoder_position = encoder.readPosition(); // Read position from encoder at address 0x54
        RCLCPP_INFO(rclcpp::get_logger("test_node"), "Encoder Position: %d", encoder_position);
        
        rclcpp::sleep_for(std::chrono::milliseconds(10));  
        time--;
    }
    motor.stop_motor();
    motor.get_status(motor_status);
    string state_str = besfoc::state_dict.at(motor_status);
    RCLCPP_INFO(rclcpp::get_logger("test_node"), "Motor Status: %s", state_str.c_str());
    
}

int main()
{
    // Example usage of CanMotor class

    AMT21 encoder(0x54, "/dev/ttyUSB0", 115200);

    encoder.setZero(); // Reset encoder at address 0x54

    auto bus = std::make_shared<CanBus>("can0");
    bus->connect();

    besfoc::CanMotor motor4(0x68, bus);
    RCLCPP_INFO(rclcpp::get_logger("test_node"), "CanBus connected successfully.");

    //test_relative_postion(motor4, encoder);
    test_velocity(motor4, encoder);

    bus->disconnect();

    RCLCPP_INFO(rclcpp::get_logger("test_node"), "CanBus disconnected successfully.");
    
    return 0;
}

