# Gungnir controller

[Back to the main README](../../README.md)

This package contains the MoveIt Servo configuration and a joystick-to-Twist
translator for the currently implemented three-joint Gungnir arm.

## Joystick teleoperation

Build and source the workspace, then start the hardware controller:

```bash
colcon build
source install/setup.bash
ros2 launch gungnir_interface bringup.launch.py
```

In a second terminal, source the workspace and start Servo:

```bash
source install/setup.bash
ros2 launch gungnir_controller servo.launch.py
```

The bringup launch already starts RViz. To run the Servo launch with its own
RViz instance instead, pass `launch_rviz:=true`. A non-default controller device
can be selected with, for example, `joy_dev:=/dev/input/js1`.

The default Xbox-compatible mapping is:

- Hold the left bumper (button 4) as the dead-man switch.
- Left stick vertical commands Cartesian X.
- Left stick horizontal commands Cartesian Y.
- Right stick vertical commands Cartesian Z.
- Hold the right bumper (button 5) instead to jog joints 1, 2, and 3 with the
  left-stick horizontal, left-stick vertical, and right-stick vertical axes.
- Pressing both bumpers is treated as ambiguous input and stops motion.
- Releasing either bumper immediately publishes a zero command; Servo's input
  timeout also stops output if joystick messages disappear.

The three-joint arm cannot independently control Cartesian orientation, so all
angular axes are disabled by default. Cartesian and joint axis indices,
directions, deadzone, and dead-man buttons are configurable in
`config/joystick.yaml`. The `joint_names`, `joint_axes`, and `joint_scales`
lists must have matching lengths. Use a negative scale to invert an axis.

Servo starts with conservative limits of 0.1 m/s linear and 0.3 rad/s angular
in `config/gungnir_config.yaml`. Test at low speed with clearance around the arm
before increasing them.

Useful checks before enabling the motors:

```bash
ros2 topic echo /joy
ros2 topic echo /servo_node/delta_twist_cmds
ros2 topic echo /servo_node/delta_joint_cmds
ros2 topic echo /servo_node/status
ros2 control list_controllers
```

`joint_state_broadcaster` and `joint_trajectory_controller` should both report
`active`. The Servo launch expects the hardware bringup to provide
`/joint_states`, TF, and `/joint_trajectory_controller/joint_trajectory`.
