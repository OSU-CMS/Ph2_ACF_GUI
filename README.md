# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

## Installation:
This software is designed to run using Docker. This ensures that all testing is performed with consistent and 
## Running in docker:
This software is designed to run using Docker. A compiled version of the most current stable release of Ph2_ACF and everything required to run it is included in the container. There are two containers available to meet the needs of both developers and users.

### Docker for users:
This container is designed for users who are testing modules whether that be at a testing center or and assembly center.  The following set of directions can be used for the initial setup:

1. Initial clone GUI repo:
```
git clone --recurse-submodules https://github.com/OSU-CMS/Ph2_ACF_GUI.git
```
or, pull the latest changes while inside the Ph2_ACF_GUI directory:
```
git submodule update --recursive --remote
git pull --recurse-submodules
```

2. Set site-specific configurations:
```
cd Ph2_ACF_GUI/Gui
```
Open the file siteConfig.py in your favorite text editor and go to the "Icicle variables" section.  Here you can set the model of LV/HV devices, specify the USB ports, etc.  You should also scroll down to the "FC7List" and edit the fc7.board.* listed there to match the IP addresses of your FC7 device(s).

<img width="525" alt="configureGUI" src="https://github.com/OSU-CMS/Ph2_ACF_GUI/assets/19957717/126fe7a2-70a7-4791-86e0-4cb839d6dc4b">


3. Start the docker container:
```
cd ..
bash run_Docker.sh
```

4. That's it!  At this point the GUI should be open and ready to use.

5. On the login screen you can use either of the following usernames to log in locally (bypassing connection to the database).

For non-expert mode:    username = `local`  (in development)
For expert mode:        username = `localexpert`

### Docker for developers:
This container is meant for developers or users who would like to customize some aspects of the GUI.  It sets up the environment for the GUI and Ph2_ACF to run and opens to a command line.  Local files are mounted to the container so that changes to the local files will be reflected inside the container.  This allows for code development without needing to build a new Docker image after each modification of the code.

1. Clone GUI repo:
```
git clone --recurse-submodules https://github.com/OSU-CMS/Ph2_ACF_GUI.git
cd Ph2_ACF_GUI
```
or pull the latest changes while inside the Ph2_ACF_GUI directory:
```
git submodule update --recursive --remote
git pull --recurse-submodules
```

2. Set site-specific configurations:
```
cd Ph2_ACF_GUI/Gui
```
Open the file siteConfig.py in your favorite text editor and go to the "Icicle variables" section.  Here you can set the model of LV/HV devices, specify the USB ports, etc.  You should also scroll down to the "FC7List" and edit the fc7.board.* listed there to match the IP addresses of your FC7 device(s).

3. Choose the developer Docker image:

4. Start the docker container:
```
bash run_Docker.sh dev
```

5. Set up Ph2_ACF and open GUI:
When you first open the container you should run
```
source prepare_Ph2_ACF
```
This will set up the Ph2_ACF environment variables in the container and open the GUI.  If you exit the GUI, it will take you back to the Gui directory.  If you want to open the GUI again while still inside the container, you can just run
```
python3 QtApplication
```

6. Exit and kill container when done:
```
exit
``` 
will close and kill the container.  Other commands may only exit the container and keep it running in the background, so be sure to use this method of exiting.  Another option is to hit ctrl-D.
## General Notes On Docker
After installing docker to your system make sure that you have enabled the docker service. To check the status of the docker service, you can run: 
```
sudo systemctl status docker
```
To enable the service run: 
```
sudo systemctl enable --now docker
```

Also ensure that your user has been added to the docker user group so that docker can be ran without the use of root privileges. You can check this by running:
```
id $USER
```
This should show docker listed in your groups. 
To add a user to a group, run 
```
sudo usermod -aG docker $USER
```
## Docker Notes for Alma Linux 
RHEL insists that users of Alma use a program called podman. They claim that podman is a drop in replacement for docker, however, this does not seem to be the case. As such, there are special instructions that need to be followed in order to run "normal" docker. You can run the following commands to remove podman and install docker. Instructions for this process were obtained [here](https://www.liquidweb.com/kb/install-docker-on-linux-almalinux/). 

1. Update system
```
sudo dnf --refresh update
sudo dnf upgrade
```
2. Install yum-utils so we can add the repository that contains docker
```
sudo dnf install yum-utils
```
3. Add the docker repository 
```
sudo yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
```
4. Install necessary docker packages
```
sudo dnf install docker-ce docker-ce-cli containerd.io docker-compose-plugin
```
5. Enable and start docker service
```
sudo systemctl enable --now docker
```
6. Add user to docker user group to enable to the use of docker 
```
sudo usermod -aG docker $USER
```
7. Reboot for user group to take affect
```
reboot
```

## Run the GUI
```
cd Gui
python3 QtApplication.py
```
On the login screen you can use either of the following usernames to log in locally (bypassing connection to the database).

For non-expert mode:    username = `local`  
For expert mode:        username = `localexpert`

### Non-expert mode (In development)
After logging in you will see status indicators for the database, HV, LV, FC7, Temp/Humidity.  If you are running locally, then the database indicator will be red.  If anything else is red you can click the "Refresh" button to attempt to reconnect all the devices.  If you are ready to test a module you should:
1. Either manually (on a keyboard) enter an module id into the appropriate box and press "enter" on the keyboard or scan a QR code with your scanner (if you have one).
2. (Optional) Choose "Quick Test" if you don't want to run the full suite of tests.
3. Click the green "Go" button to start the tests.

### Expert mode
After logging in you will need to specify some hardware configurations.  
1. Choose FC7 by click the "Use" button next to the appropriate FC7.
2. If you are using the Peltier control you should click "Start Peltier", enter you temperature setting, click "Set Temperature", click "Turn on Peltier".
3. If you are using the default HV and LV configuration that you set in `Gui/siteSettings.py`, you can click "Connect all devices" to connect HV and LV devices as well as your Arduino device if you have that set up.  Otherwise, you can select the port for each device from a list or uncheck the boxes next to them if you prefer to control them manually.
4. Clicking "New" will open a window for running a new test.  You will choose which test(s) you would like to run and which type of module you are testing.  You will also need to enter the Module serial number, FMC number, and Chip ID number in the provided fields.  Once you've done that, you can choose the power mode (direct or SLDO) and click "Next".  If you are manually controlling your HV and LV, a window will open asking if you want to continue.  Click "Yes" to open a new window for running test. 
5. When the next window opens, click "Run" to begin the test(s).

# Notes on contributing
If you would like to contribute, and you are not at Ohio State there are a few things to keep in mind: 
1. The computer we currently use to run this GUI has python version 3.6, therefore, when adding code to the GUI you should make sure that your code is compatible with python 3.6. If using the docker image in dev mode, this should be satisfied.
2. Please ensure that you have tested your code with a module installed or if you do not have a module, that you ensure the GUI launches before making a pull request.

# Bug List / Fixes: 
This is a list of issues that have only happened on a singular device, therefore do not warrant a complete bug fix, but should be documented somewhere. 
* If you can't launch the docker with error: Invalid MIT-MAGIC-COOKIE-1 keyqt.qpa.xcb: could not connect to display :0
run the command
```
xhost +local:
```
