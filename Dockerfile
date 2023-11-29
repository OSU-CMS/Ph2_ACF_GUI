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
ENV PYTHONPATH=${PYTHONPATH}:${GUI_dir}
ENV PYTHONPATH=${PYTHONPATH}:${GUI_dir}/icicle/icicle

USER root

LABEL Name=ph2acfgui_dev Version=${Ph2_ACF_VERSION}

#Specify the working directory in the container
WORKDIR /home/cmsTkUser/Ph2_ACF_GUI/

#Adding the current local working directory to the container working directory.
#This is recursive so all of the sub-directories should also be added.
ADD . /home/cmsTkUser/Ph2_ACF_GUI/
RUN ls -lrt

#Installing all needed packages in the container.
RUN dnf -y install libxkbcommon-x11-devel mesa-libGL-devel xcb-util-wm xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm 
RUN dnf -y install dbus-x11 gcc gcc-c++ kernel-devel make usbutils udev mysql

RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install -r requirements.txt

#GIT_REF is used in the compileSubModules script so you need to define it here before running the script.
ARG GIT_REF=Dev
RUN sh ./compileSubModules.sh
RUN chmod +x prepare_Ph2ACF.sh

#Comment the following line if you want to build the developer container.  The following line makes docker open the GUI when the container started.
#CMD ["prepare_Ph2ACF.sh"]



#The following is old stuff I think.

#ENTRYPOINT ["/bin/bash"]
#The following would open the GUI when docker run is called.  Otherwise it will just give a terminal.
#CMD ["./QtApplication.py"]
#ENTRYPOINT ["python3"]
