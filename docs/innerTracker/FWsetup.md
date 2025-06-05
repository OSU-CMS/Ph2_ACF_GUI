# Firmware setup

1. Check whether the DIP switches on FC7 board are setup for the use of a microSD card (`out-in-in-in-out-in-in-in`)
2. Insert a microSD card in the PC and run `/sbin/fdisk -l` to understand to which dev it's attached to (`/dev/sd_card_name`)
3. Upload a golden firmware on the microSD card (read FC7 manual or run `dd if=sdgoldenimage.img of=/dev/sd_card_name bs=512`)
4. Download the proper IT firmware version from [here](https://gitlab.cern.ch/cmstkph2-IT/d19c-firmware/-/releases)
5. Plug the microSD card in the FC7
6. From Ph2_ACF use the command `fpgaconfig` to upload the proper IT firmware (see instructions: `IT-DAQ setup and run` before running this command)

**N.B.:** a golden firmware is any stable firmware either from IT or OT, and it's needed just to initialize the `IPbus` communication at bootstrap (in order to create and image of the microSD card you can use the command: `dd if=/dev/sd_card_name conv=sync,noerror bs=128K | gzip -c > sdgoldenimage.img.gz`) <br />
A golden firmware can be downloaded from the [cms-tracker-daq webpage](https://cms-tracker-daq.web.cern.ch/cms-tracker-daq/Downloads/sdgoldenimage.img) <br />
A detailed manual about the firmware can be found [here](https://gitlab.cern.ch/cmstkph2-IT/d19c-firmware/blob/master/doc/IT-uDTC_fw_manual_v1.0.pdf)
