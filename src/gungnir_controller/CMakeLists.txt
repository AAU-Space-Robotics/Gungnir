cmake_minimum_required(VERSION 3.8)
project(gungnir_controller)

# find dependencies
find_package(rclcpp REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(pluginlib REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(include)

add_library(gungnir_controller SHARED
  src/some_script.cpp
)


ament_target_dependencies(gungnir_controller
  rclcpp
  pluginlib
)

add_executable(joystick_servo src/joystick_servo.cpp)
target_compile_features(joystick_servo PUBLIC cxx_std_17)
ament_target_dependencies(joystick_servo
  geometry_msgs
  rclcpp
  sensor_msgs
  std_srvs
)

install(TARGETS gungnir_controller
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS joystick_servo
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config launch rviz
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
