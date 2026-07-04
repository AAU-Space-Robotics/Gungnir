import os

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterFile
from launch_ros.parameter_descriptions import ParameterValue

from launch import LaunchDescription
from launch.actions import LogInfo
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution

from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    
    os.system("sudo ip link set can0 up type can bitrate 1000000")

    # Get URDF
    urdf_folder = os.path.join(get_package_share_directory("gungnir_description"), "urdf")
    urdf = os.path.join(urdf_folder, "robot.urdf.xacro")
    robot_description_values = ParameterValue(Command(['xacro ', urdf]), value_type=str)
    robot_description = {'robot_description': robot_description_values}

    # Robot State Publisher
    robot_state_publisher_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="both",
        parameters=[robot_description],
    )
    
    joint_state_publisher = Node(
        package="joint_state_publisher",
        executable="joint_state_publisher",
        name="joint_state_publisher",
        output="screen",
        parameters=[robot_description],
    )
    
    rviz = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", os.path.join(get_package_share_directory("gungnir_description"), "rviz", "rviz_config.rviz")],
    )
    ld = LaunchDescription()

    ld.add_action(robot_state_publisher_node)
    ld.add_action(rviz)
    ld.add_action(joint_state_publisher)
    return ld
