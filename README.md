# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

Uses the following python packages: PyQt5 (https://pypi.org/project/PyQt5/), pyqt-darktheme (https://pypi.org/project/pyqt-darktheme/)

The following instructions assume that you already have Ph2_ACF set up and working.  
## Installing the latest version of Ph2_ACF:
1. It's a good idea to create a separate directory for each version of Ph2_ACF you are running.
```
mkdir -m777 <pathyouwant>/<Ph2_ACF version>
```
for example:
```
mkdir -m777 /home/RD53A/workspace/v4.11
```
The -m777 flag is useful if you have multiple user logins on your computer.  If everyone shares the same login, this option is not needed.

2. cd into the directory that you just made for Ph2_ACF

3. Clone and set up the latest Ph2_ACF version:
```
git clone --recurse-submodules https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF.git
cd Ph2_ACF
source setup.sh
mkdir build
cmake3 ..
make -jN
cd ..
source setup.sh
mkdir -m777 test
```



## Set up the software environment:
This software has not been designed to work in a conda environment.

1. Update pip:
```
python3 -m pip install --upgrade pip
```

2. Install PyQt5:
```
pip install PyQt5
```

3. Install pyqt-darktheme:
```
pip install pyqt-darktheme
```

4. Install MySQL connector:
```
pip install mysql-connector-python
```

5. Install NumPy:
```
pip install numpy
```

6. Install Matplotlib:
```
pip install matplotlib
```

7. Install lxml:
```
pip install lxml
```
8. Install pyvisa:
```
pip install pyvisa
pip install pyvisa-py
```
9. Modify Setup_template.sh to include the paths to your Ph2_ACF working area and where you are locally storing test results.  Then copy this template to a site-specific name or just call it `Setup.sh`.
```
cp Setup_template.sh Setup_<nameyouchose>.sh
```
If you are updating Ph2_ACF_GUI, then you can check the difference for any additions that are not site-specific:
```
vim -d Setup_template.sh Setup.sh
```
and then copy and paste whatever lines are needed to your `Gui/siteSettings.py` file.

10. Edit fc7_ip_address.txt so that the ip addresses for your fc7 boards are listed.

11. Default settings are specified in the `Gui/siteSettings.py` file.  This is where you set the default configurations for your system.  This file does not exist on the repository, but instead there is a `Gui/siteSettings_template.py` file.  Copy this file:
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
source Setup.sh
cd Gui
python3 QtApplication.py
```

On the login screen you can use either of the following usernames to log in locally (bypassing connection to the database).

For non-expert mode:    username = `local`  
For expert mode:        username = `localexpert`

### Non-expert mode
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