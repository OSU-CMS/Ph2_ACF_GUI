# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

Uses the following python packages: PyQt5 (https://pypi.org/project/PyQt5/), pyqt-darktheme (https://pypi.org/project/pyqt-darktheme/)

The following instructions assume that you already have Ph2_ACF set up and working.  

This software has not been designed to work in a conda environment.

## Set up the software environment:

1. Install PyQt5:
```
pip install PyQt5
```

2. Install pyqt-darktheme:
```
pip install pyqt-darktheme
```

3. Install MySQL connector:
```
pip install mysql-connector-python
```

4. Install Pillow:
```
pip install Pillow
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
9. Modify Setup.sh to include the paths to your Ph2_ACF working area and where you are locally storing test results.

10. Edit fc7_ip_address.txt so that the ip addresses for your fc7 boards are listed.

11. Default settings are specified in the `Gui/siteSettings.py` file.  This is where you set the default configurations for your system.  If this file doesn't exist for you, then you should create it, copy the block of code below into it, and modify it match your system.  The contents should look something like this:
```
######################################################################
# To be edited by expert as default setting for Hardware configuration
######################################################################
# default FC7 boardName
defaultFC7 = "fc7.board.1"
# default IP address of IP address
defaultFC7IP = '192.168.1.80'
# default fpga config
defaultFPGAConfig = 'IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28'
# default FMC board number
defaultFMC = '0'
# default USB port for HV power supply
defaultUSBPortHV = ["ASRL/dev/ttyUSBHV::INSTR"]
# default model for HV power supply
defaultHVModel = ["Keithley 2410 (RS232)"]
# default USB port for LV power supply
defaultUSBPortLV = ["ASRL/dev/ttyUSBLV::INSTR"]
# default model for LV power supply
defaultLVModel = ["KeySight E3633 (RS232)"]
# default model for LV power supply
defaultLVModel = ["KeySight E3633 (RS232)"]
# default mode for LV powering (Direct,SLDO,etc)
defaultPowerMode = "Direct"
defaultVoltageMap = {
    "Direct" : 1.28,
    "SLDO"   : 1.85
}
#default BaudRate for Arduino sensor
defaultSensorBaudRate = 115200
#default DBServerIP
defaultDBServerIP = '127.0.0.1'
#default DBName
defaultDBName = 'SampleDB'
```


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
2. If you are using the default HV and LV configuration that you set in `Gui/siteSettings.py`, you can click "Connect all devices" to connect HV and LV devices.  Otherwise, you can select the port for each device from a list or uncheck the boxes next to them if you prefer to control them manually.
3. Clicking "New" will open a window for running a new test.  You will choose which test(s) you would like to run and which type of module you are testing.  You will also need to enter the Module serial number, FMC number, and Chip ID number in the provided fields.  Once you've done that, you can choose the power mode (direct or SLDO) and click "Check" if you are remotely controlling your HV and LV.  This checks that the voltage and current is within a reasonable range.  If you are manually controlling your HV and LV you can simply click "Next".  A window will open asking if you want to continue.  Click "Yes" to open a new window for running test.  Click "Run" to begin the test(s).