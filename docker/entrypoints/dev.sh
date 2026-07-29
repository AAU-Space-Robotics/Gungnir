#!/bin/bash
echo "   ________                             __         "
echo "  /  _____/ __ __  ____    ____   ____ |__|______  "
echo " /   \  ___|  |  \/    \  / ___\ /    \|  \_  __ \ "
echo " \    \_\  \  |  /   |  \/ /_/  |   |  \  ||  | \/ "
echo "  \______  /____/|___|  /\___  /|___|  /__||__|    "
echo "         \/           \//_____/      \/       [DEV]"

source /opt/ros/humble/setup.bash
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
echo "alias quick_build='colcon build && source install/setup.bash'" >> ~/.bashrc
echo "alias test_trajectory='ros2 launch ros2_control_demo_example_7 send_trajectory.launch.py'" >> ~/.bashrc

bash