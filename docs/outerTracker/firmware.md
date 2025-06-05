# Firmware

The compiled firmwares are stored [here](https://udtc-ot-firmware.web.cern.ch).

Usually you should use the firmwares stored in the [latest directory](https://udtc-ot-firmware.web.cern.ch/?dir=latest)

How to read the firmware names:
- `2s` or `ps` indicates the type of module for which the firmware is compiled
    - Note that for `ps` there are two speed `5g` and `10g`. Select the appropriate based on your module
    - For kickoff 2S modules use the one with `fec12`
- `8m` or `12m` explanins if it is compiled for 8 or 12 modules respectively
- `cic2` indicated that it is compiled for modules with CIC v2 or 2.1. This is a legacy from when we were supporting older modules with CIC v1.
- The last part of the name indicates which board is expected on each FC7 slots. Please refer to the [FC7 nomenclature page](../general/FC7nomenclature.md).

Note: With the `8m` firmware the bottom left slot of the L12 card is optical group 0 while if you are using the `12m` firmware it is optical group 4 and the left slot of the L8 card is optical group 0.
