cmake_minimum_required(VERSION 3.8)
project(gungnir_interface)

# find dependencies
find_package(rclcpp REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(pluginlib REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(rcpputils REQUIRED)

add_library(gungnir_interface SHARED
  src/gungnir_interface.cpp
  src/besfoc_driver.cpp
)

add_executable(test_node src/test_node.cpp)

target_include_directories(gungnir_interface PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(test_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(gungnir_interface
  rclcpp
  rclcpp_lifecycle
  pluginlib
  hardware_interface
  rcpputils
)

ament_target_dependencies(test_node
  rclcpp
)

target_link_libraries(test_node
  gungnir_interface
)

ament_export_include_directories(include)
ament_export_dependencies(
  rclcpp
  rclcpp_lifecycle
  pluginlib
  hardware_interface
  rcpputils
)

pluginlib_export_plugin_description_file(hardware_interface gungnir_interface.xml)

install(TARGETS gungnir_interface
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS test_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
