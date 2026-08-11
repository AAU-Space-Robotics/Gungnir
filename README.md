# Welcome to the Gungnir repo!
<!-- Cool ascii art -->
```
   ________                             __         
  /  _____/ __ __  ____    ____   ____ |__|______  
 /   \  ___|  |  \/    \  / __ \ /    \|  \_  __ \ 
 \    \_\  \  |  /   |  \/ /_/  >   |  \  ||  | \/ 
  \______  /____/|___|  /\___  /|___|  /__||__|    
         \/           \//_____/      \/            
```

There is supposed to be a readme here, but I haven't written it yet.

To CAN or not to CAN

## Packages
### gungnir_controller
[[gungnir_controller README]](src/gungnir_controller/README.md)\
Motion / MoveIt integration, planning, joystick teleoperation and Servo configuration.

### gungnir_description
[[gungnir_description README]](src/gungnir_description/README.md)\
Robot model, URDF/Xacro, meshes, joint limits, SRDF and visualization assets.

### gungnir_interface
[[gungnir_interface README]](src/gungnir_interface/README.md)\
Hardware/system interface (CAN, motors, encoders and drivers). Exposes the ros2_control System plugin (`gungnir::RobotSystem`) used by `ros2_control_node`.

## Important nodes
- `ros2_control_node` (from `controller_manager`): loads the robot_description (URDF + ros2_control block) and the hardware plugin; manages controllers. Launched by:
  - bringup.launch.py
  - r6bot_controller.launch.py
- `spawner` (controller_manager executable): used to spawn controllers such as `joint_state_broadcaster`, `joint_trajectory_controller`, or robot-specific controllers. Spawned in the bringup launches.
- `robot_state_publisher`: publishes TF from joint states; launched in urdf_viz.launch.py and bringup launches.
- `joint_state_publisher` / `joint_state_broadcaster`: publish joint states (either simple publisher for viz or controller_manager broadcaster for real hardware). Spawners present in the bringup launches.
- `rviz2`: visualization; launched by all three bringups for visualization.
- `move_group` (MoveIt): planning and execution node, launched by servo.launch.py.
- `send_trajectory` (example): demo node to send trajectories in `ros2_control_demo_example_7` (send_trajectory.launch.py).
- `test_node` in `gungnir_interface`: direct encoder/motor test utility (test_node.cpp).

## Usage

### Docker container 

The repository contains two docker containers:

- `gungnir_arm` - Houses all software related to controlling the manipulator arm.
- `zenoh-router` - Optional zenoh router for running the arm as standalone.

Start each container in detaced mode with `docker compose`:
```bash
cd /home/gungnir/gungnir/docker
docker compose up gungnir_arm -d
```
To see the output logs from the containter, simply remove `-d` to attach the container.

### Run Gungnir with a test trajectory

#### Terminal 1: Start the Controller
```bash
ros2 launch gungnir_interface bringup.launch.py 
```
#### Terminal 2: Send the test trajectory
```bash
ros2 launch ros2_control_demo_example_7 send_trajectory.launch.py 
```

### Control Gungnir with a joystick

Start the hardware controller as above. In a second terminal, connect an
Xbox-compatible controller and launch MoveIt Servo:

```bash
source install/setup.bash
ros2 launch gungnir_controller servo.launch.py
```

Hold the left bumper while moving the sticks. See the
[controller README](src/gungnir_controller/README.md) for the mapping, safety
behavior, configuration, and diagnostics.


<!-- 
**Startup / runtime sequence (typical bringup)**
1. Bring up CAN interface (launch files call `sudo ip link set can0 up type can bitrate 1000000`).
2. Start `ros2_control_node` with:
   - `robot_description` (xacro → URDF includes ros2_control block that references `gungnir/RobotSystem`)
   - controller configs (`controllers.yaml` or package-specific YAML)
3. `ros2_control_node` loads the hardware plugin from `gungnir_interface` which initializes CAN buses, drivers, motors/encoders (see `on_init`, `read`, `write` in gungnir_interface.cpp).
4. Spawn required controllers with `spawner` (e.g., `joint_state_broadcaster`, `joint_trajectory_controller`).
5. Start `robot_state_publisher` (and optionally `joint_state_publisher`) and `rviz2` for visualization.
6. Higher-level control: `move_group` (MoveIt) or demo nodes (`send_trajectory`) connect to controllers via controller_manager to command trajectories.

**Notes & quick observations**
- The hardware plugin namespace/class is `gungnir::RobotSystem` and the URDF plugin tag is `gungnir/RobotSystem`. Ensure the package name and plugin lookup align at build/install time (pluginlib resolves `package_name/ClassName`).
- bringup.launch.py scripts use `sudo ip link set can0 up` — you'll need privileges or arrange CAN bring-up externally.
- Controller YAMLs and MoveIt configs are the places to edit joint mappings, controller names and gains:
  - controllers.yaml
  - gungnir_config.yaml 
-->
