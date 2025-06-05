# Installing RHEL/AlmaLinux 9

## Installation Guide

Please follow the
[AlmaLinux 9 Installation Guide](https://wiki.almalinux.org/documentation/installation-guide.html):

- Download the latest AlmaLinux OS 9 DVD ISO from
[almalinux.org/get-almalinux/](https://almalinux.org/get-almalinux/) and
[write the ISO file contents on a USB drive](https://wiki.almalinux.org/documentation/installation-guide.html#write-an-image-on-a-usb)
 (in March 2024 this is version 9.3).
  - ***Attention***: do **not** download/use the ISO from CERN unless the computer will be installed and located at CERN (the pre-configured software repositories are only available within the CERN network).
- Plug in the USB drive with the AlmaLinux OS 9 ISO contents and boot the target computer choosing the option to install AlmaLinux 9 in the boot menu.
- Follow the on-screen instructions to "Install AlmaLinux 9". You can follow the
[step-by-step instructions](https://wiki.almalinux.org/documentation/installation-guide.html#installation).
  - If your system is at CERN, follow the
  [instructions by CERN IT](https://linux.web.cern.ch/almalinux/alma9/stepbystep/) instead.
  - When asked to select the software to install, select "Workstation".

## Post-installation Guide

Once the system has been installed and the PC has rebooted, log in as `root` or user with `sudo` privileges.
Follow the
[AlmaLinux after-installation guide](https://wiki.almalinux.org/documentation/after-installation-guide.html) to set up the network (if needed), install updates, and enable the EPEL and PowerTools/CRB repositories:

```shell
sudo dnf update
sudo dnf config-manager --set-enabled crb
sudo dnf install epel-release
```

Then reboot the system.
