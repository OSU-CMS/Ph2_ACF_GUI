.. _high_level_interfaces:

High Level Interfaces
=============================

ICICLE currently provides a limited set of instrument-independent interfaces designed to allow external dependencies and scripts to use some functionality without needing to refer to a particular instrument's command set. These are implemented manually on a per-instrument basis, and designed to provide as close to identical functionality on each instrument as possible. However, it cannot be guaranteed that they will be independent of initial conditions or instrument-specific behaviours (e.g. voltage/current ranges, measurement ranges, instrument-specific warnings/trips/states, etc.).

The ``Channel`` Concept
-----------------------

The high-level interfaces follow the ``Channel`` concept, in which a minimal working datapoint is provided to read from and/or write to. For example, a Multimeter might provide multiple read-only (Measure-) Channels: ``VOLT:DC``, ``VOLT:AC``, ``CURR:DC``, ``RES`` (resistance), etc., depending on its capabilities. On the other hand, a cooling system may provide a writable temperature channel that is targeted, alongside read-only channels for the current setpoint temperature, the actual temperature, and the humidity.

Interfaces for channels should be scoped to the :py:class:`icicle.instrument.Instrument` class, and will then be subclassed in relevant instruments to provide concrete implementations. For example, :class:`icicle.instrument.Instrument.MeasureChannel` is subclassed by an explicit implementation in :class:`icicle.keithley2000.Keithley2000.MeasureChannel`.

Channels should be instantiated via calls to the :func:`icicle.instrument.Instrument.channel` on a given instrument that the channel should be sourced from, passing the ``Instrument.<channel_type>`` superclass and a channel number and further required arguments. For instantiation examples, see the :mod:`icicle.powerchannel_cli` and :mod:`icicle.measurechannel_cli` command-line interfaces.

``MeasureChannel``
-------------------------------------------

The :class:`icicle.instrument.Instrument.MeasureChannel` class provides a simple interface for a single read-only value that may be read or measured from a device. This is usually used for e.g. the real-time voltage/current output of a power supply, or the response of a digital multimeter or temperature of a coldbox. Implementations must provide the ``value`` property, and may optionally also provide a ``status`` value (instrument-specific definition, but should remain stable over time if everything is ok), and a ``unit``.

For more explicit API details, see the :class:`icicle.instrument.Instrument.MeasureChannel` API reference.

``PowerChannel``
-----------------------

The :class:`icicle.instrument.Instrument.PowerChannel` class represents a singular power output on a power supply or similar device. It encompasses a setpoint for ``voltage``, ``current``, and limits on each, as well as two ``MeasureChannel`` object to allow measurements of the actual output voltage and current. In addition, ``voltage`` and ``current`` trip conditions may be queried and/or reset, and again a device-dependent ``status`` is available.

For more explicit API details, see the :class:`icicle.instrument.Instrument.PowerChannel` API reference.

``TemperatureChannel``
-----------------------

The :class:`icicle.instrument.Instrument.TemperatureChannel` class represents a singular temperature control variable on a power supply or similar device. It encompasses a setpoint for ``temperature_setpoint`` and ``speed``. The latter is currently a device-dependent control variable setting something like a pump speed or PID control variables in the underlying device.

For more explicit API details, see the :class:`icicle.instrument.Instrument.TemperatureChannel` API reference.


``MeasureChannelWrapper``
-------------------------

The :class:`icicle.instrument.Instrument.MeasureChannelWrapper` interface is designed as a unique way to wrap ``MeasureChannel`` objects in order to respond to measurement requests (before or after) via a secondary instrument, or in order to transform the measurement output. For example, connections of particular channels to a multimeter via a RelayBoard are well expressed using the ``MeasureChannelWrapper`` interface, as may be seen for example in :class:`icicle.relay_board.RelayBoard.MeasureChannelWrapper`. The API for ``MeasureChannelWrapper`` is still considered WIP (work-in-progress), and may be subject to revision or change.

For more explicit API details, see the :class:`icicle.instrument.Instrument.MeasureChannelWrapper` API reference.


Instantiation
-------------

These interfaces can be instantiated via the :func:`icicle.instrument.Instrument.channel` factory, and are tied to the instrument in terms of lifetime on instantiation. Destroying or removing a device will also unload all associated :class:`Instrument.Channel` derivatives.

The factory can be used with the following syntax ``instrument.channel(CHANNEL_TYPE, CHANNEL, ...)``. The additional arguments pass channel-specific parameters, such as `measure_type` for a :class:`Instrument.MeasureChannel` instance, and `wrapped_channel` as well as `wrapper_type` for :class:`Instrument.MeasureChannelWrapper`. Here's an example:

.. code-block:: python

    instrument = ICICLE.TTI(...)
    output1 = instrument.channel(Instrument.PowerChannel, 1)
    output2 = instrument.channel("PowerChannel", 2)
    monitor_voltage1 = instrument.channel("MeasureChannel", 1, measure_type='VOLT:DC')
