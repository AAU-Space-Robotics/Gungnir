#!/bin/bash
source /opt/ros/humble/setup.bash
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
echo "source /home/ws/install/setup.bash" >> ~/.bashrc

colcon build --symlink-install >> /home/ws/entry_build.log 2>&1

echo "   ________                            .__         "
echo "  /  _____/ __ __  ____    ____   ____ |__|______  "
echo " /   \  ___|  |  \/    \  / ___\ /    \|  \_  __ \ "
echo " \    \_\  \  |  /   |  \/ /_/  >   |  \  ||  | \/ "
echo "  \______  /____/|___|  /\___  /|___|  /__||__|    "
echo "         \/           \//_____/      \/            "


source /opt/ros/humble/setup.bash
source /home/workspace/install/setup.bash

ros2 run demo_nodes_cpp talker

# bash