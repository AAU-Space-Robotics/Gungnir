import os

import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def load_yaml(package_name, relative_path):
    package_path = get_package_share_directory(package_name)
    with open(os.path.join(package_path, relative_path), "r", encoding="utf-8") as file:
        return yaml.safe_load(file)


def generate_launch_description():
    description_share = get_package_share_directory("gungnir_description")
    controller_share = get_package_share_directory("gungnir_controller")

    robot_description = {
        "robot_description": ParameterValue(
            Command([
                "xacro ",
                os.path.join(description_share, "urdf", "robot.urdf.xacro"),
            ]),
            value_type=str,
        )
    }

    with open(
        os.path.join(description_share, "srdf", "gungnir.srdf"),
        "r",
        encoding="utf-8",
    ) as file:
        robot_description_semantic = {
            "robot_description_semantic": file.read()
        }

    servo_params = {
        "moveit_servo": load_yaml(
            "gungnir_controller", "config/gungnir_config.yaml"
        )
    }

    servo_node = Node(
        package="moveit_servo",
        executable="servo_node_main",
        name="servo_node",
        output="screen",
        parameters=[
            servo_params,
            robot_description,
            robot_description_semantic,
        ],
    )

    joystick_config = os.path.join(controller_share, "config", "joystick.yaml")
    joy_node = Node(
        package="joy",
        executable="joy_node",
        name="joy_node",
        output="screen",
        parameters=[
            joystick_config,
            {
                "dev": ParameterValue(
                    LaunchConfiguration("joy_dev"), value_type=str
                )
            },
        ],
    )
    joystick_servo_node = Node(
        package="gungnir_controller",
        executable="joystick_servo",
        name="joystick_servo",
        output="screen",
        parameters=[joystick_config],
    )

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2_servo",
        output="log",
        arguments=["-d", os.path.join(controller_share, "rviz", "rviz_config.rviz")],
        parameters=[robot_description, robot_description_semantic],
        condition=IfCondition(LaunchConfiguration("launch_rviz")),
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "joy_dev",
                default_value="/dev/input/js0",
                description="Linux joystick device read by joy_node",
            ),
            DeclareLaunchArgument(
                "launch_rviz",
                default_value="false",
                description="Start another RViz instance with the Servo nodes",
            ),
            servo_node,
            joy_node,
            joystick_servo_node,
            rviz_node,
        ]
    )
