#   ________                            .__         #
#  /  _____/ __ __  ____    ____   ____ |__|______  #
# /   \  ___|  |  \/    \  / ___\ /    \|  \_  __ \ #
# \    \_\  \  |  /   |  \/ /_/  >   |  \  ||  | \/ #
#  \______  /____/|___|  /\___  /|___|  /__||__|    #
#         \/           \//_____/      \/            #
FROM ros:humble

# Setup user
ARG USERNAME=gungnir
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

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

# Clean up
RUN rm -rf /var/lib/apt/lists/*

#####################################
##### Create user               #####
#####################################

RUN groupadd --gid $USER_GID $USERNAME && \
    useradd --uid $USER_UID --gid $USER_GID -m $USERNAME && \
    echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

#####################################
##### Copy and set up workspace #####
#####################################

# Create workspace directory
RUN mkdir -p /home/ws && chown $USERNAME:$USERNAME /home/ws

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Switch to non-root user
USER $USERNAME

# Set working directory
WORKDIR /home/ws

ENTRYPOINT ["/entrypoint.sh"]
# Enjoy ;)