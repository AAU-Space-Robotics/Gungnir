Hello, there is supposed to be a readme here, but I haven't written it yet.
To CAN or not to CAN

### Run Gungnir with a test trajectory

#### Terminal 1: Start the Controller
'''bash
ros2 launch gungnir_interface bringup.launch.py 
'''
#### Terminal 2: Send the test trajectory
'''bash
ros2 launch ros2_control_demo_example_7 send_trajectory.launch.py 
'''

