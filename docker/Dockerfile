#   ________                            .__         #
#  /  _____/ __ __  ____    ____   ____ |__|______  #
# /   \  ___|  |  \/    \  / ___\ /    \|  \_  __ \ #
# \    \_\  \  |  /   |  \/ /_/  >   |  \  ||  | \/ #
#  \______  /____/|___|  /\___  /|___|  /__||__|    #
#         \/           \//_____/      \/            #
FROM ros:humble AS base

# Setup user
ARG USERNAME=gungnir
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

#####################################
##### Create user               #####
#####################################

RUN if id -u $USER_UID ; then userdel `id -un $USER_UID` ; fi

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    #
    # [Optional] Add sudo support. Omit if you don't need to install software after connecting.
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Avoid prompts with apt-get
ARG DEBIAN_FRONTEND=noninteractive

####################
##### Installs #####
####################

# Install Utilities
RUN apt-get update && apt-get install -y \
  sudo \
  nano \
  net-tools

# Install Interfaces
RUN apt-get update && apt-get install -y python3-pip && \
  pip3 install \
  canopen

# Install Zenoh for better ros2 network performance
RUN apt-get update && apt-get install -y ros-humble-rmw-zenoh-cpp

# Install myactuator rmd python library
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  can-utils \
  iproute2 \
  python3-pybind11 

RUN git clone https://github.com/2b-t/myactuator_rmd.git \
  && cd myactuator_rmd \
  && mkdir build \
  && cd build \
  && cmake .. -D PYTHON_BINDINGS=on \
  && make -j $(nproc) \
  && sudo make install \
  && cd .. \
  && pip3 install .

# Other ROS packages
RUN apt-get update && apt-get install -y \
  ros-humble-xacro \
  ros-humble-ros2-control \
  ros-humble-ros2-controllers \
  ros-humble-joy \
  ros-humble-moveit \
  ros-humble-moveit-kinematics \
  ros-humble-moveit-servo \
  ros-humble-rviz2 \
  ros-humble-demo-nodes-cpp

# Clean up
RUN rm -rf /var/lib/apt/lists/*

#####################################
##### Copy and set up workspace #####
#####################################

# Create workspace directory
RUN mkdir -p /home/ws && chown $USERNAME:$USERNAME /home/ws

COPY src/ /home/ws/src

FROM base AS dev

COPY docker/entrypoints/dev.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Switch to non-root user
USER $USERNAME

# Set working directory
WORKDIR /home/ws

ENTRYPOINT ["/entrypoint.sh"]


FROM base AS prod

COPY docker/entrypoints/gungnir.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Switch to non-root user
USER $USERNAME

# Set working directory
WORKDIR /home/ws

RUN /bin/bash -c "source /opt/ros/humble/setup.bash && \
                  colcon build"

ENTRYPOINT ["/entrypoint.sh"]
# Enjoy ;)
