.. _implementing_instruments:

Implementing a New Instrument
=============================

Generally, it is a good idea to first have a browse of the :py:mod:`icicle.instrument`, :py:mod:`icicle.visa_instrument` and :py:mod:`icicle.scpi_instrument` modules to gain an understanding of the base classes and their functionality. Familiarise yourself with the decorators and features provided.

To provide an interface for a new instrument, you must identify the communication type:

SCPI-style commands
-------------------

If it uses Keithley/TTI/R&S/...-style SCPI-commands, it's best to subclass the :py:class:`icicle.scpi_instrument.SCPIInstrument` class, and define a ``COMMANDS`` dictionary for the SCPI interface.

Copy one of the existing SCPI-based instruments, and work from there. Generally this will just require you to follow the instrument documentation to add the SCPI commands you need. All these commands will be automatically accessible via the ``set(...)`` and ``query(...)`` functions inherited from the ``SCPIInstrument`` class.

Then reimplementment ``__enter__`` and ``__exit__`` with the required setup and pack-down operations and set sensible starting/default parameters, and make sure to add a ``reset_on_init`` option and a ``off_on_close`` or ``ramp_down_on_close`` if this is a power supply.

Next, follow the instructions below to add a CLI interface.

Other VISA-compliant communication protocol
-------------------------------------------

For something like a custom-built Arduino serial interface or other Visa-compliant interface, you can subclass the :py:class:`icicle.visa_instrument.VisaInstrument` class. You will need to do a lot more work to implement the interface from scratch, but can implement something similar to :py:class:`icicle.scpi_instrument.SCPIInstrument` if the device has a consistent and sensible interface.

You should still use ``__enter__`` and ``__exit__`` to perform set up and pack down, and set sensible parameters.

Other non-VISA protocol
-----------------------

For something like MinimalModbus or other non-Visa-compliant protocols, you will need to do everything from scratch. It is probably best to write an intermediate class similar to :py:class:`icicle.visa_instrument.VisaInstrument` for your protocol, then work from there.


Adding a CLI Interface
----------------------

This is fairly simple. Basically just copy the CLI interface file for the instrument closest to yours, then adjust the names of files (should be ``instrument_cli.py`` and imports. Finally, adjust/replace the commands to match the interface you desire for your new instrument.

You'll need to add a line `'<instrument_command>: <instrument_module>_cli:cli',` to ``setup.py`` in the same place as the other instruments, then run ``setup.py develop`` another time to make the CLI accessible in your python environment.
