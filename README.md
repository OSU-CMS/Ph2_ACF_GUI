# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

## Running in docker:
Using docker is the easiest and most simple way to install and run this software.  A compiled version of the most current stable release of Ph2_ACF is included in the container. There are two containers available to meet the needs of both developers and users.

### Docker for users:
This container is designed for users who are testing modules.  The following set of directions can be used for the initial setup:

1. Clone GUI repo and checkout the DEV branch:
```
git clone --recurse-submodules https://github.com/OSU-CMS/Ph2_ACF_GUI.git
git checkout DEV
```
or pull the latest changes while inside the Ph2_ACF_GUI directory:
```
git pull --recurse-submodules
```

2. Set site-specific configurations:
```
cd Ph2_ACF_GUI/Gui
```
Open the file siteConfig.py in your favorite text editor and go to the "Icicle variables" section.  Here you can set the model of LV/HV devices, specify the USB ports, etc.  You should also scroll down to the "FC7List" and edit the fc7.board.* listed there to match the IP addresses of your FC7 device(s).

3. Specify device ports:
In run_Docker.sh you need to update the devices in the "mydevicelist" to reflect the ports you will be using. The plan for this to be automated, but for now you need to change it in this file.

4. Start the docker container:
```
cd ..
bash run_Docker.sh
```

5. That's it!  At this point the GUI should be open and ready to use.


### Docker for developers:
This container is meant for developers or users who would like to customize some aspects of the GUI.  It sets up the environment for the GUI and Ph2_ACF to run and opens to a command line.  Local files are mounted to the container so that changes to the local files will be reflected inside the container.  This allows for code development without needing to build a new Docker image after each modification of the code.

1. Clone GUI repo and checkout the DEV branch:
```
git clone --recurse-submodules https://github.com/OSU-CMS/Ph2_ACF_GUI.git
git checkout DEV
```
or pull the latest changes while inside the Ph2_ACF_GUI directory:
```
git pull --recurse-submodules
```

2. Set site-specific configurations:
```
cd Ph2_ACF_GUI/Gui
```
Open the file siteConfig.py in your favorite text editor and go to the "Icicle variables" section.  Here you can set the model of LV/HV devices, specify the USB ports, etc.  You should also scroll down to the "FC7List" and edit the fc7.board.* listed there to match the IP addresses of your FC7 device(s).

3. Specify device ports:
In run_Docker.sh you need to update the devices in the "mydevicelist" to reflect the ports you will be using. The plan for this to be automated, but for now you need to change it in this file.

4. Choose the developer Docker image:
In run_Docker.sh you need to comment the line that runs the "user" Docker image and uncomment the line that runs the "dev" docker image.

5. Start the docker container:
```
bash run_Docker.sh
```

6. Set up Ph2_ACF and open GUI:
When you first open the container you should run
```
source prepare_Ph2_ACF
```
This will set up the Ph2_ACF environment variables in the container and open the GUI.  If you exit the GUI, it will take you back to the Gui directory.  If you want to open the GUI again while still inside the container, you can just run
```
python3 QtApplication
```

7. Exit and kill container when done:
```
Control + d
``` 
will close and kill the container.  Other commands may only exit the container and keep it running in the background, so be sure to use this method of exiting.

# The following is only needed if you are NOT using Docker
## Set up the software environment:
This software will only work in Alma Linux 9 (RHEL 9).  It is highly recommended that everyone use the Docker containers rather than this method.  This software is only tested while running inside the Docker containers, so proceed at your own risk.

1. Clone GUI repo
```
git clone --recurse-submodules https://github.com/OSU-CMS/Ph2_ACF_GUI.git
```
or pull the latest changes
```
git pull --recurse-submodules
```

2. Go into Ph2_ACF_GUI directory and update submodules
```
git submodule update --init --recursive
```

3. Install python packages
```
python3 -m pip install --upgrade pip
python3 -m pip install -r requirements.txt
```

4. Compile submodules
``` 
source compileSubModules.sh
```


5. Default settings are specified in the `Gui/siteSettings.py` file.  This is where you set the default configurations for your system.  This file does not exist on the repository, but instead there is a `Gui/siteSettings_template.py` file.  Copy this file:
```
cp Gui/siteSettings_template.py Gui/siteSettings.py
```
and then edit it to match the settings needed at your site.  If you are updating Ph2_ACF_GUI, then you should check the difference for any additions that are not site-specific:
```
vim -d Gui/siteSettings_template.py Gui/siteSettings.py
```
and then copy and paste whatever lines are needed to your `Gui/siteSettings.py` file. 


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