# FC7 setup

1. Install `wireshark` in order to figure out which is the MAC address of your FC7 board (`sudo yum install wireshark`, then run `sudo tshark -i ethernet_card`, where `ethernet_card` is the name of the ethernet card of your PC to which the FC7 is connected to)
2. In `/etc/ethers` put `mac_address fc7-1` and in `/etc/hosts` put `192.168.1.80 fc7-1` (increase these numbers for additional FC7 boards)
3. Restart the network
4. Install the rarpd daemon: `sudo dnf install rarpd`
5. Start the rarpd daemon: `sudo systemctl start rarpd` (to start rarpd automatically after bootstrap: `sudo systemctl enable rarpd`)

More details on the hardware needed to setup the system can be found [here](https://indico.cern.ch/event/1014295/contributions/4257334/attachments/2200045/3728440/Low-resoution%202021_02%20DAQ%20School.pdf)
