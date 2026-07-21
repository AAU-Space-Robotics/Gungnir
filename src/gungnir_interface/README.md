# Gungnir_interface

[[Back to main README]](/README.md)

### hardware/system interface (CAN, motors, encoders, drivers). 
Exposes a ros2_control System plugin (gungnir::RobotSystem) used by `ros2_control_node`. 

Key files:\
`gungnir_interface.cpp` - SystemInterface implementation \
`bringup.launch.py` - Bringup launch that starts ros2_control_node\
`test_node.cpp` - Test node for direct driver tests\
`controllers.yaml` - Controllers config

This package contains the interface to the harware of the Gungnir manipulator, including:
  - Motors
  - Encoders
  - Cameras