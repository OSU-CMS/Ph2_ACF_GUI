"""This library provides lightweight classes and command line tools for instrument
control/DCS.

The following devices are currently supported:
    - Rhode&Schwarz/Hameg HMP4040 LV power supply: `hmp4040`.
    - TTI MX100TP-class LV power supply: `tti`.
    - Keithley 2000 multimeter: `keithley2000`.
    - Keithley 24X0-class source-measure unit: `keithley2410`.
    - ETHZ SLDO probe card controller board (arduino; a. la. Vasilije Perovic):
      `relay_board`

A super-device implementation for a standard module testing setup consisting of
LV, HV, relay board, and multimeter (with interlock features) is provided:
`instrument_cluster`.
"""

# TODO: update this
