# CMS Ph2 ACF (Acquisition & Control Framework)


### Contains:
- A middleware API layer, implemented in C++, which wraps the firmware calls and handshakes into abstracted functions
- A C++ object-based library describing the system components (CBCs, RD53, Hybrids, Boards) and their properties (values, status)


##
###  A short guide to write the GoldenImage to the SD card
1. Connect the SD card
2. Download the golden firmware from the [cms-tracker-daq webpage](https://cms-tracker-daq.web.cern.ch/cms-tracker-daq/Downloads/sdgoldenimage.img)
3. `sudo fdisk -l` - find the name of the SD card (for example, /dev/mmcblk0)
4. `sudo chmod 744 /dev/sd_card_name` - to be able to play with it
5. Go to the folder were you saved the sdgoldenimage.img file
6. `dd if=sdgoldenimage.img of=/dev/sd_card_name bs=512` - to write the image to the SD card.
If the SD card is partitioned (formatted), pay attention to write on the block device (e.g. `/dev/mmcblk0`) and not inside the partition (e.g. `/dev/mmcblk0p1`)
7. Once the previous command is done, you can list the SD card: `./imgtool /dev/sd_card_name list` - there should be a GoldenImage.bin, with 20MB block size
8. Insert the SD card into the FC7

Alternatively, instead of the `dd` command above, to only copy the needed bytes you can do:
```bash
imageName=sdgoldenimage.img
dd if=$imageName bs=512 iflag=count_bytes of=somefile_or_device count=$(ls -s --block-size=1 $imageName | awk '{print $1}')
```

If you installed the command `pv` (`sudo yum install -y pv`), then the best way is the following (replacing `/dev/mmcblk0` with your target device):
```bash
pv sdgoldenimage.img | sudo dd of=/dev/mmcblk0
```


##
### The `Ph2_ACF` software
Installation of the software is documented at <https://ph2acf.docs.cern.ch/general/ph2acf_install/>


##
### Run in docker container (deprecated: update of docker registry to Alma9 is required)
Docker container are provided to facilitate users and developers in setting up the framework.
All docker containers can be found here: `https://gitlab.cern.ch/cms_tk_ph2/docker_exploration/container_registry`

Do run using one of the container, use the command:
```bash
docker run --rm -ti -v $PWD:$PWD -w $PWD <image>
```

Suggested images are:
-  For users (comes with Ph2_ACF of Dev branch installed): `gitlab-registry.cern.ch/cms_tk_ph2/docker_exploration/cmstkph2_user_c7:latest`
-  For developers (no Ph2_ACF, just environment and libraries): `gitlab-registry.cern.ch/cms_tk_ph2/docker_exploration/cmstkph2_udaq_c7:latest`

Specific tags can be pulled substituting `latest` with `ph2_acf_<Ph2_ACF tag>` (i.e. `ph2_acf_v4-05`)


##
### Gitlab CI setup for Developers (required to submit merge requests)
Enable shared Runners (if not enabled)
- From `settings > CI/CD` expand the `Runners` section
- Click the `Allow shared Runners` button


##
### To pull large files
Install `git lfs`
```bash
sudo yum install git-lfs
git lfs install
```
Go to your main `Ph2_ACF` folder and run
```bash
git config lfs.https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF.git/info/lfs.locksverify true # or your username instead of cms_tk_ph2
```

For example `lpGBT` calibration data for a different source repository (e.g. `cmsinnertracker`)
```bash
git remote add cms_tk_ph2 https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF.git
git fetch cms_tk_ph2
git lfs fetch cms_tk_ph2
git lfs pull cms_tk_ph2
```

You might need to configuring the `git-lfs smudge`:
```bash
git config --global filter.lfs.smudge "git-lfs smudge --skip -- %f"
git config --global filter.lfs.process "git-lfs filter-process --skip"
```


##
### Known issues
uHAL exceptions and UDP timeouts when reading larger packet sizes from the FC7 board: this can happen for some users (cause not yet identified) but can be circumvented by changing the line

`ipbusudp-2.0://192.168.000.175:50001`

in the connections.xml file to

`chtcp-2.0://localhost:10203?target=192.168.000.175:50001`

and then launching the CACTUS control hub by the command:

`/opt/cactus/bin/controlhub_start`

This uses TCP protocol instead of UDP which accounts for packet loss but decreases the performance


##
### Support, suggestions
For any support/suggestions, send an email to fabio.raveraSPAMNOT@cern.ch, mauro.dinardoSPAMNOT@cern.ch


##
### Firmware repository for OT tracker
[https://udtc-ot-firmware.web.cern.ch/](https://udtc-ot-firmware.web.cern.ch/)


##
### Outer Tracker Suggested Tags

Firmware tag: v3-03

Ph2_ACF tag: v6-11