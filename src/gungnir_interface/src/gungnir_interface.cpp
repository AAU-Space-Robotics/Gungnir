// Copyright 2023 ros2_control Development Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gungnir_interface/gungnir_interface.hpp"
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include <cmath>


namespace gungnir
{
CallbackReturn RobotSystem::on_init(const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
  {
    return CallbackReturn::ERROR;
  }

  //Initialize the can drivers
  RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Initializing CAN interfaces...");
  besfoc_bus = std::make_shared<CanBus>("can0");
  besfoc_bus->connect();

  my_actuator_bus = new myactuator_rmd::CanDriver("can0");

   //Add the motors and encoders to the hardware maps
  RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Initializing motors and encoders...");

  RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Initializing MyActuator motors for joint 2");
  
  //Joint 2 motor(MyActuator RMD)
  auto [rmd_it_2, rmd_inserted_2] = rmd_motors.emplace(1, myactuator_rmd::ActuatorInterface(*my_actuator_bus, MYACTUATOR_2_CAN_ID));
  rmd_it_2->second.setAcceleration(30000, myactuator_rmd::AccelerationType::VELOCITY_PLANNING_ACCELERATION);
  rmd_it_2->second.setAcceleration(30000, myactuator_rmd::AccelerationType::VELOCITY_PLANNING_DECELERATION);

  RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Initializing MyActuator motors for joint 3");
  //Joint 3 motor(MyActuator RMD)
  auto [rmd_it_3, rmd_inserted_3] = rmd_motors.emplace(2, myactuator_rmd::ActuatorInterface(*my_actuator_bus, MYACTUATOR_3_CAN_ID));
  rmd_it_3->second.setAcceleration(30000, myactuator_rmd::AccelerationType::VELOCITY_PLANNING_ACCELERATION);
  rmd_it_3->second.setAcceleration(30000, myactuator_rmd::AccelerationType::VELOCITY_PLANNING_DECELERATION);
 
  RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Initializing BesFoc motor and AMT21 encoder for joint 1...");

  //Joint 1 motor and encoder(BesFoc and AMT21)
  auto [besfoc_it, besfoc_inserted] = besfoc_motors.emplace(0, besfoc::CanMotor(BESFOC_1_CAN_ID, besfoc_bus));
  (void)besfoc_inserted;
  besfoc_it->second.set_acceleration(10000); // Set acceleration to 1000 rpm/s
  besfoc_it->second.set_deceleration(10000); // Set deceleration to 1000 rpm/s
  besfoc_it->second.set_mode(besfoc::SPEED_MODE); // Set mode to velocity control

  amt21_encoders.try_emplace(0, AMT21_1_NODE_ADDRESS, "/dev/ttyUSB0", 115200, true);
  auto encoder_it = amt21_encoders.find(0);
  encoder_it->second.setZero(); // Reset encoder at address 0x54

  RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Motors and encoders initialized successfully.");

  // // robot has 6 joints and 2 interfaces
  joint_position_.assign(6, 0);
  joint_velocities_.assign(6, 0);
  joint_velocities_command_.assign(6, 0);

  for (const auto & joint : info_.joints)
  {
    for (const auto & interface : joint.state_interfaces)
    {
      joint_interfaces[interface.name].push_back(joint.name);
    }
  }

  return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn RobotSystem::on_deactivate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  // Stop all motors when deactivating
  for (auto & [joint_num, motor] : besfoc_motors)
  {
    motor.stop_motor();
  }

  for (auto & [joint_num, motor] : rmd_motors)
  {
    motor.stopMotor();
    motor.shutdownMotor();
  }

  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> RobotSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;

  // TODO Link the state interfaces to the actual variables in the cpp CAN interface
  int ind = 0;
  for (const auto & joint_name : joint_interfaces["position"])
  {
    state_interfaces.emplace_back(joint_name, "position", &joint_position_[ind++]);
  }

  ind = 0;
  for (const auto & joint_name : joint_interfaces["velocity"])
  {
    state_interfaces.emplace_back(joint_name, "velocity", &joint_velocities_[ind++]);
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> RobotSystem::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;

  // TODO Link the state interfaces to the actual variables in the cpp CAN interface
  int ind = 0;
  for (const auto & joint_name : joint_interfaces["velocity"])
  {
    command_interfaces.emplace_back(joint_name, "velocity", &joint_velocities_command_[ind++]);
  }

  return command_interfaces;
}

return_type RobotSystem::read(const rclcpp::Time & /*time*/, const rclcpp::Duration & period)
{
  // TODO Make the pyhton CAN to c++ CAN to use it here

  // // TODO(pac48) set sensor_states_ values from subscriber

  for (auto i = 0ul; i < joint_velocities_command_.size(); i++)
  {
   
    auto encoder_it = amt21_encoders.find(i);
    auto my_actuator_it = rmd_motors.find(i);

    if(encoder_it != amt21_encoders.end()) {
     
      // Update joint_position_ from the AMT21 encoder state
      int raw_position = encoder_it->second.readPosition();
      double radian_position = (static_cast<double>(raw_position) / AMT21::MAX_VALUE) * 2 * M_PI; // Convert raw position to radians
      
      RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Read position from encoder for joint %zu: %d (raw), %f (radians)", i, raw_position, radian_position);
  
      joint_velocities_[i] = (radian_position - joint_position_[i]) / period.seconds(); // Calculate velocity based on change in position over time
      joint_position_[i] = radian_position; // Update position
      
    }else if(my_actuator_it != rmd_motors.end()) {
      // Update joint_position_ and joint_velocities_ from the MyActuator RMD motor state
      double position = (my_actuator_it->second.getMultiTurnAngle()) * M_PI / 180.0; // Convert degrees to radians
      double velocity = (position - joint_position_[i]) / period.seconds(); // Calculate velocity based on change in position over time

      RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Read position and velocity from MyActuator for joint %zu: %f (position), %f (velocity)", i, position, velocity);
      
      joint_position_[i] = position;
      joint_velocities_[i] = velocity;

    }else {
      //If no hardware is associated with this joint
      RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Current velocity command for joint %zu: %f", i, joint_velocities_command_[i]);
      
      joint_velocities_[i] = joint_velocities_command_[i];
      joint_position_[i] += joint_velocities_command_[i] * period.seconds();
    }
  }

  return return_type::OK;
}

return_type RobotSystem::write(const rclcpp::Time &, const rclcpp::Duration &)
{
  // TODO Make the pyhton CAN to c++ CAN to use it here
  for (auto i = 0ul; i < joint_velocities_command_.size(); i++)
  {
    auto besfoc_it = besfoc_motors.find(i);
    auto my_actuator_it = rmd_motors.find(i);
    if(besfoc_it != besfoc_motors.end()) {
      // Send velocity command to the BesFoc motor
      double velocity_command = joint_velocities_command_[i];
      int rpm_command = static_cast<int>(velocity_command * 60.0 * BESFOC_1_GEAR_RATIO/ (2.0 * M_PI)); // Convert rad/s to rpm at the motor shaft

      RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Sending velocity commandf to motor for joint %zu: %d rpm", i, rpm_command);
      besfoc_it->second.set_velocity(static_cast<int>(rpm_command));
    }else if(my_actuator_it != rmd_motors.end()) {
      // Send velocity command to the MyActuator RMD motor
      double velocity_command = (joint_velocities_command_[i]) * (180.0 / M_PI); // Convert rad/s to degrees/s

      RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Sending velocity command to MyActuator for joint %zu: %f rpm", i, velocity_command);
      my_actuator_it->second.sendVelocitySetpoint(velocity_command);
    }
  }

  return return_type::OK;
}

}  // namespace ros2_control_demo_example_7

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  gungnir::RobotSystem, hardware_interface::SystemInterface)
