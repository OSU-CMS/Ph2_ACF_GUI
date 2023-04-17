ARG FROM_IMAGE=gitlab-registry.cern.ch/cms_tk_ph2/docker_exploration/cmstkph2_udaq_c7:latest

FROM $FROM_IMAGE AS base
SHELL ["/bin/bash", "-c"]
ENV Ph2_ACF_VERSION=v4-12
USER root

LABEL Name=ph2acfgui Version=4.12.1
WORKDIR /home/cmsTkUser/Ph2_ACF_GUI/
ADD . /home/cmsTkUser/Ph2_ACF_GUI/
RUN ls -lrt

RUN yum -y install libxkbcommon-x11-devel mesa-libGL-devel xcb-util-wm xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm python3-pyqt5
RUN yum -y install dbus-x11 build-essential qtcreator qt5-default usbutils udev
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install -r requirements.txt


RUN sh .complileSubModules.sh

#The following would open the GUI when docker run is called.  Otherwise it will just give a terminal.
#CMD ["./QtApplication.py"]
#ENTRYPOINT ["python3"]