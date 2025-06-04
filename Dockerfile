#Select the base image upon which the GUI docker image will be build.  
#You can set the version of Ph2_ACF by adding the following line to your git build command:
#  --build-arg GIT_REF=v4-14
#If you don't use this option when building the image it will default to the Dev branch of Ph2_ACF.
ARG GIT_REF=Dev
ARG FROM_IMAGE=gitlab-registry.cern.ch/cms_tk_ph2/docker_exploration/cmstkph2_udaq_al9:latest
FROM $FROM_IMAGE AS base

SHELL ["/bin/bash", "-c"]

#Setting up all of the environment variables that the GUI will use.
#An ARG is only in the scope of the build that immediately follows, so you have to re-do this for each usage.
ARG GIT_REF=Dev
ENV Ph2_ACF_VERSION=${GIT_REF}
ENV GUI_dir=/home/cmsTkUser/Ph2_ACF_GUI
ENV PH2ACF_BASE_DIR=${GUI_dir}/Ph2_ACF
ENV DATA_dir=${GUI_dir}/data/TestResults
ENV PYTHONPATH=${PYTHONPATH}:${GUI_dir}:${GUI_dir}/icicle/icicle:${GUI_dir}/InnerTrackerTests:${GUI_dir}/felis

# Commenting for now, this was needed to change user to non-ROOT 
ARG USER_UID=1000
ARG USER_GID=1000
ARG APP_PASSWORD

# Set environemt variable for application password (can be overriden at runtime)
ENV APP_PASSWORD=${APP_PASSWORD}

# Create the group and user with the specified UID/GID
RUN groupadd -g ${USER_GID} cmsTkUser || true && \
    useradd -m -u ${USER_UID} -g cmsTkUser cmsTkUser &&\
    usermod -a -G dialout cmsTkUser && \
    echo "cmsTkUser ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/cmsTkUser_nopasswd && \
    chmod 0440 /etc/sudoers.d/cmsTkUser_nopasswd

# Create XDG runtime dir for GUI
ENV DISPLAY=:0
ENV QT_XKB_CONFIG_ROOT=/usr/share/X11/xkb
ENV XDG_RUNTIME_DIR=/tmp/runtime-cmsTkUser
RUN mkdir -p $XDG_RUNTIME_DIR && chmod 700 $XDG_RUNTIME_DIR && chown -R cmsTkUser:cmsTkUser $XDG_RUNTIME_DIR

# Install system dependencies before switching to user 
RUN dnf -y update && \
    dnf -y install \
    mesa-libGL-dlevel 
    libxkbcommon-x11-devel \
    xcb-util-wm \
    xcb-util-image \
    xcb-util-keysyms \
    xcb-util-renderutil \
    dbus-x11 \
    gcc gcc-c++ \
    kernel-devel \
    make \
    usbutils \
    udev \
    git \
    wget \
    libXext libXrender libXtst nss libasound && \
    dnf clean all

# Switch to non-root user
USER cmsTkUser

# Set working directory for the app
WORKDIR ${GUI_dir}

# Copy requirements first for cache efficiency
COPY --chown=cmsTkUser:cmsTkUser requirements.txt ${PH2ACF_BASE_DIR}/

# Install python dependencies 
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install -r ${PH2ACF_BASE_DIR}/requirements.txt

# Copy the rest of the source
COPY --chown=cmsTkUser:cmsTkUser . ${PH2ACF_BASE_DIR}/

# Ensure scripts are executable
RUN chmod +x ${GUI_dir}/prepare_Ph2ACF.sh && \
    chmod +x ${PH2ACF_BASE_DIR}/compileSubModules.sh

# Compile Ph2 ACF submodules
RUN ${PH2ACF_BASE_DIR}/compileSubModules.sh

# Set proper permissions
RUN chown -R cmsTkUser:cmsTkUser ${GUI_dir}/data && \
    chown cmsTkUser:cmsTkUser ${GUI_dir}/Gui/python/rhapi.py


# Add cmsTkUser to the 'dialout' group for serial port access
# Crucial for Arduino communication
RUN usermod -a -G dialout cmsTkUser

# Configure sudo for cmsTkUser if elevated privileges are ever needed inside the container
RUN echo "cmsTkUser ALL=(ALL) NOPASSWD:ALL" | sudo tee /etc/sudoers.d/cmsTkUser_nopasswd && \
    sudo chmod 0440 /etc/sudoers.d/cmsTkUser_nopasswd

# Set the application password environment variable. 
# Ensure APP_PASSWORD is passed as a --build-arg or --env during docker run if needed.
ENV APP_PASSWORD=${APP_PASSWORD}

# Specify the working directory in the container for the cmsTkUser.
# This is where the application will be launched from.
WORKDIR ${GUI_dir}




#Setting the default user in the container to be root
# USER root

LABEL Name=ph2acfgui_dev Version=${Ph2_ACF_VERSION}

#Specify the working directory in the container
WORKDIR /home/cmsTkUser/Ph2_ACF_GUI/

#Adding the current local working directory to the container working directory.
#This is recursive so all of the sub-directories should also be added.
ADD . /home/cmsTkUser/Ph2_ACF_GUI/
RUN ls -lrt

#Installing all needed packages in the container.
RUN dnf -y install libxkbcommon-x11-devel mesa-libGL-devel xcb-util-wm xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm mesa-dri-drivers mesa-libGL
RUN dnf -y install dbus-x11 gcc gcc-c++ kernel-devel make usbutils udev
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install -r requirements.txt



#GIT_REF is used in the compileSubModules script so you need to define it here before running the script.
ARG GIT_REF=Dev
RUN sh ./compileSubModules.sh
RUN chmod +x prepare_Ph2ACF.sh

#RUN chown -R cmsTkUser:cmsTkUser /home/cmsTkUser

# For some reason I couldn't change the permissions on this file alongside the other chown command.
# TODO: This was also needed to change user in docker image
RUN chown -R cmsTkUser:cmsTkUser /home/cmsTkUser/Ph2_ACF_GUI/data && \
    [ -e /home/cmsTkUser/Ph2_ACF_GUI/Gui/python/rhapi.py ] && \
    chown cmsTkUser:cmsTkUser /home/cmsTkUser/Ph2_ACF_GUI/Gui/python/rhapi.py || true
#USER cmsTkUser

#Comment the following line if you want to build the developer container.  The following line makes docker open the GUI when the container started.
CMD ["prepare_Ph2ACF.sh"]

#ENTRYPOINT ["/bin/bash"]
#The following would open the GUI when docker run is called.  Otherwise it will just give a terminal. -> I think this is an old comment so this statement should be checked.
#CMD ["./QtApplication.py"]
#ENTRYPOINT ["python3"]
