# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

## Installation:
This software is designed to run using Docker. This ensures that all testing is performed in a reliable and consistent way.

## Device settings:
Devices are controlled using the Icicle package. Please make sure your devices have the correct settings in order for proper communication.

| Model | baud rate | termination|
| --- | ---| --- |
| Keithley 2410 | 9600 | LF |
| Keithley 2000 | 9600 | device default |
| HP34401A | 9600 | device default |
| Keysight E3633A | 9600 | device default |



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
Open the file siteConfig.py in your favorite text editor and go to the "Icicle variables" section.  Here you can specify which JSON file you are currently using, set the FC7 ips, and set up the cable mapping (simplified only).

In the JSON file, you can set the model of LV/HV devices, specify USB ports, etc. You should only include instruments that you have connected. For example, if you do not have a relay board or a digital multimeter, you should not list those in the JSON.

For every instrument you wish to connect, you must make a separate entry in the 'instrument_dict' section of the JSON. In the example below, there is is one LV power supply, one HV power supply, a relay board, and a multimeter. Each of these has a corresponding entry in 'instrument_dict' detailing its model (listed as "class"), resource, default voltage, and default current. There is another attribute, "sim," which represents whether the device is simulated or not. This should be false for nearly all use cases. Devices that do not have a voltage or current, such as a relay board, multimeter, or adc board, should be set to 0 as a default.

The next section in the JSON is titled 'channels_dict.' This category is responsible for linking your power supply channels to the GUI. For each LV/HV pair, you need one entry in this section. For each entry, you need to specify the LV and the HV contained in it. To do this, you set up the dictionary with two keys, one for your LV and the other for the corresponding HV. You then associate that with an 'instrument' and a 'channel.' The 'instrument' tag corresponds to the physical instrument listed in 'instrument_dict.' For example, the LV in our instrument dict is titled "lv_1," so we name our instrument in 'channels_dict' "lv_1." The channel then corresponds the which specific channel on that instrument you are interested in using.

<img width="325" alt="json_relaysldo" src="https://github.com/user-attachments/assets/2a477026-1e4d-4e2f-b506-2595541603cd">

Now, to set up multile LV power supplies, you can simply create another entry in 'instrument_dict' for your new instrument. Next, you must update the channels_dict section with this new power supply. You will need to create a second entry with the new LV power supply. Each key in channels_dict should be an integer, indexed at 0, as shown below. The trick here is that you still need a LV/HV pair, so you can simply reuse the HV power supply you entered before. Below is an example of this JSON fully written out.

<img width="332" alt="json_twoLV" src="https://github.com/user-attachments/assets/46e71d44-8cc5-4487-b151-7e7663755777">

In Gui/jsonFiles, there are example files written that may be modified to suit your purposes.

Returning to Gui/siteConfig.py, you should also scroll down to the "FC7List" and edit the fc7.board.* listed there to match the IP addresses of your FC7 device(s).

If you scroll down a little further, you will see a dictionary titled "CableMapping." This serves as a mapping of the cable ID you see when adding modules in the simplified GUI to a physical port on your FC7(s). Each cable ID is associated with a dictionary detailing the path to a port. The first key, "FC7," specifies which FC7 that you want that cable ID to be connected to. The FC7 you list should be in the FC7List above. Next, you can name the "FMCID," representing which FMC on the FC7 you wish to use. The possible values for this are "L8" if the FMC is on the left or "L12" if the FMC is on the right. Finally, you can specify which port on that FMC you want to connect to. The leftmost port is "0" and the rightmost port is "3."

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
bash run_Docker.sh
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
3. Device configurations are set in `Gui/siteConfig.py`, you can click "Connect all devices" to connect HV and LV devices as well as your other devices if you have that set up. 
4. Clicking "New" will open a window for running a new test.  You will choose which test(s) you would like to run and which type of module you are testing.  You will also need to enter the Module serial number, FMC number (L12 or L8), and FMC port number (0-3) in the provided fields.  Once you've done that, you can choose the power mode (direct or SLDO) and click "Next".  If you are manually controlling your HV and LV, a window will open asking if you want to continue.  Click "Yes" to open a new window for running test. 
5. When the next window opens, click "Run" to begin the test(s).

# Notes on contributing
If you would like to contribute, and you are not at Ohio State there are a few things to keep in mind: 
1. This software uses Python 3.9 and is designed to work in an Alma Linux 9 environment.  If using the docker image in dev mode, this should be satisfied.  We are happy to give more detailed instructions for anyone interested.
2. Please ensure that you have tested your code with a module installed or if you do not have a module, that you ensure the GUI launches before making a pull request.

# Bug List / Fixes: 
This is a list of issues that have only happened on a singular device, therefore do not warrant a complete bug fix, but should be documented somewhere. 
* If you can't launch the docker with error: Invalid MIT-MAGIC-COOKIE-1 keyqt.qpa.xcb: could not connect to display :0
run the command
```
xhost +local:
```

# Transferring F4T Temperature Controller Data Logs with TFTP
The F4T Temperature Controller can transfer its data logs via USB, Samba, or Trivial File Transfer Protocol (TFTP). Here are steps to set up a TFTP server on your Linux machine and have the F4T automatically send data logs to the server. Note this tutorial does not use the commonly used xinetd daemon.
### Creating the Server
1. On your hosting machine, install the `tftp-server` and `tftp` packages

`dnf install tftp-server`

`dnf install tftp`

3. You may have to make the TFTP service file or it may already exist `mkdir /etc/systemd/system/tftp.service`
4. Edit the TFTP service file `nano /etc/systemd/system/tftp.service`
5. Add the following lines
```
[Unit]
        Description = Tftp Server
        Requires = tftp.socket
        Documentation=man:in.tftpd

[Service]
        ExecStart=/usr/sbin/in.tftpd -c /path/to/data/logs/destination
        StandardInput=socket

[Install]
        Also=tftp.socket
```
5. You may have to make the TFTP socket file or it may already exist `mkdir /etc/systemd/system/tftp.socket`
6. Edit the TFTP socket file `nano /etc/systemd/system/tftp.socket`
7. Add the following lines
```
[Unit]
        Description=TFTP Server Activation Socket

[Socket]
        ListenDatagram=69
        SocketMode=0666
        BindIPv6Only=both

[Install]
        WantedBy=sockets.target
```
8. To employ your changes, run `systemctl daemon-reload`
9. Now start your tftp server `systemctl start tftp`
10. You can check the status of your tftp server by running `systemctl status tftp`
### Sending the Data Logs
**Note: The default TFTP username is "admin" and the default TFTP password is "1234"**
1. On the F4T (GUI version 04:07:0012), navigate to Main Menu > Data Logging > Data Log File Transfer
2. Enter the following credentials:

   Auto Transfer Type: TFTP

   Samba User Name: admin

   Samba Password: 1234

   Samba Path: /path/to/data/logs/destination

   Remote Host Name: TFTP Server host's IPv4

   Remote IP Address: TFTP Server host's IPv4 (same as Remote Host Name)

4. You can test your setup with "Transfer Files Now By" section and selecting "TFTP"