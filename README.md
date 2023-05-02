# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

## Set up the software environment:
This software has not been designed to work in a conda environment.

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