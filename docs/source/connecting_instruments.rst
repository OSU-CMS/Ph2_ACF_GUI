.. _connecting_instruments:

Connecting an Instrument
========================

Preparation Steps
-----------------

Generally, connecting an instrument should be as simple as:

0. **Installing** ``icicle`` **and dependencies** (specifically, ``pyvisa`` and ``pyvisa-py`` are critical infrastructure).

1. **Connecting the instrument to the computer in question (or accessible local network)** via USB/RS232/GPIB (or Ethernet).

2. **Figuring out the correct VISA resource address for the instrument.** Look at the defaults, or check out https://zone.ni.com/reference/en-XX/help/370131S-01/ni-visa/visaresourcesyntaxandexamples/ and https://pyvisa.readthedocs.io/en/1.8/names.html for more information.

   For USB-attached devices (by converter or directly), this may mean messing around with teh ``/dev/ttyUSBx`` handles - beware these change on restarts and instrument reattachments. You might be able to avoid by instead using the ``/dev/serial/by-id/...`` with the converter or device serial number (once you figure out which is which). Best advice here is to unplug everything and plug in one device at a time to figure out which it is before you start using ``icicle``.

3. Configuring any required settings on the device itself, usually somewhere in the communication part of the device's menu interface. Generally, ``icicle`` requires an instrument to use:
    * Newlines only as termination (``\n`` - no carriage return ``\r``).
    * The Baud rate (data bit rate) must be set correctly. Generally it is 19200 for Keithley devices, and 9600 for Arduinos, but is specified by the ``BAUD_RATE`` global on each class. Check the instrument class for specifics.
    * All other features (e.g. call-response/ACK, Xon/Xoff, command counter) must be turned off.

   If necessary, these values can be changed in the code, but are not configurable on command line since other values are not known to be stable.

4. **Test communication using the**::

       <instrument> -R <visa_resource_address> identify

   **command.** For example, ``keithley2410 -R "ASRL/dev/ttyUSB1::INSTR" identify`` for a Keithley 2410 source measure unit connected on ``/dev/ttyUSB1``. Take care of the default resource addresses on all instruments (It is recommended to use aliases and udev rules to prevent the names to change when an instrument reconnected). Ensure the ``identify`` command returns a serial number matching that in the device menu, especially if you have multiple connected - else you are communicating with some other device.


Using Command Line Utilities
----------------------------

Once you've followed these steps, you're ready to go. To get a digest of commands type::

    <instrument> -R <visa_resource_address> --help

To get more information on parameters for a single command type::

    <instrument> -R <visa_resource_address> <command> --help


Using Instrument Classes in Python Scripts
------------------------------------------

This should be as easy as importing the necessary class, and instantiating an object with the appropriate constructor, then passing this to a ``with`` statement to activate the instrument context::

    from icicle.keithley2410 import Keithley2410

    COMPLIANCE=1e-6
    TARGET_VOLTAGE=60

    hv = Keithley2410(resource='ASRL/dev/ttyUSB1::INSTR', reset_on_init=False, ramp_down_on_close=True)

    with hv:
        hv.set('COMPLIANCE_CURRENT', COMPLIANCE)
        hv.set('SENSE_CURRENT_RANGE',  COMPLIANCE)
        hv.set('VOLTAGE', 0)
        hv.on()
        hv.sweep('VOLTAGE', TARGET_VOLTAGE)
        # do stuff
        hv.sweep('VOLTAGE' 0)
        hv.off()

*For example, this script would be equivalent to the shell commands*::

    COMPLIANCE=1e-6
    TARGET_VOLTAGE=60

    keithley2410 -R 'ASRL/dev/ttyUSB1::INSTR' compliance_current $COMPLIANCE
    keithley2410 -R 'ASRL/dev/ttyUSB1::INSTR' voltage 0
    keithley2410 -R 'ASRL/dev/ttyUSB1::INSTR' on
    keithley2410 -R 'ASRL/dev/ttyUSB1::INSTR' ramp_voltage $TARGET_VOLTAGE
    # do stuff
    keithley2410 -R 'ASRL/dev/ttyUSB1::INSTR' ramp_voltage 0
    keithley2410 -R 'ASRL/dev/ttyUSB1::INSTR' off

.. Note: The CLI scripts are often a good place to start when looking for the right commands to put in a python script.
