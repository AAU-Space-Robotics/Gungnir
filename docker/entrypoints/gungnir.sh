#!/bin/bash
echo "   ________                             __         "
echo "  /  _____/ __ __  ____    ____   ____ |__|______  "
echo " /   \  ___|  |  \/    \  / ___\ /    \|  \_  __ \ "
echo " \    \_\  \  |  /   |  \/ /_/  |   |  \  ||  | \/ "
echo "  \______  /____/|___|  /\___  /|___|  /__||__|    "
echo "         \/           \//_____/      \/      [PROD]"

source /opt/ros/humble/setup.bash
source /home/ws/install/setup.bash

ros2 launch gungnir_interface bringup.launch.py 