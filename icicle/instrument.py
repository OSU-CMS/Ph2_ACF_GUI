"""Instrument interface and associated decorator tools.

This module contains the `Instrument` and `Singleton` interface and metaclass, as well
as the InstrumentTimeoutError exception and LOCK_NOT_ACQUIRED dummy class.

The `@acquire_lock` and `retry_on_fail` decorators are also provided here.
"""

from functools import wraps
from threading import Lock
from os.path import realpath
from pathlib import Path
from enum import Enum
from math import ceil
import os
import importlib
import math
import time
import logging
import fcntl
import traceback
import inspect

from fasteners import InterProcessLock

logger = logging.getLogger(__name__)


class Singleton(type):
    """Singleton class provides global instance access to instruments/instrument types.
    `_instances` member is dict of instruments by class name.

    .. Note: This is not optimal - should change to Multiton based on resource address
        to ensure resource addresses are not duplicated, rather than limiting number of
        instruments of same class!!

    .. Warning: Deprecated - this is not in use any more!
    """

    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class Multiton(type):
    """Multiton metaclass provides global instance access to instruments/instrument
    types based off first argument in constructor (a bit silly, but I can't think of a
    way around this at the moment). `_instances` member is dict of instruments by key.

    .. Warning: All arguments must be provided as keyword arguments when subclass is
        init'd - this masks the "defaults" defined in the metaclass'd class.
    """

    _instances = {}

    def __init__(cls, name, bases, namespace, **kwargs):
        cls.__key = kwargs["key"]

    def __new__(metacls, name, bases, namespace, key="resource"):
        return super().__new__(metacls, name, bases, namespace)

    def __call__(cls, *args, **kwargs):
        key = kwargs[cls.__key]
        if key not in cls._instances:
            cls._instances[key] = super(Multiton, cls).__call__(*args, **kwargs)
        return cls._instances[key]

    def get_instance(cls, key=None):
        if key is not None:
            if key not in cls._instances or not isinstance(cls._instances[key], cls):
                raise KeyError(
                    f"Instrument of class {cls} with key {key} not found for "
                    "get_instance(...) call - has it been initialised yet?"
                )
            return cls._instances[key]
        else:
            return tuple(
                value for value in cls._instances.values() if isinstance(value, cls)
            )


class InstrumentTimeoutError(Exception):
    """Exception to be throw when instruments time out."""

    def __init__(self, function, instrument_class, message=""):
        super().__init__(message)
        self.function = function
        self.instrument_class = instrument_class


class ChannelError(Exception):
    """Exception to be throw when missing channel is addressed."""

    pass


class Flock:
    """Flock()-based file locking class matching interfaces of threading.Lock and
    fasteners.InterProcessLock."""

    def __init__(self, filepath):
        """
        :param filepath: path to lockfile
        """
        self._filepath = str(filepath)
        self._file = None
        self._touched = False
        self._threadlock = Lock()
        if os.path.exists(self._filepath):
            if not os.access(self._filepath, os.W_OK):
                raise RuntimeError(f"{self._filepath} not writable")
        elif not os.path.exists(os.path.dirname(self._filepath)):
            raise RuntimeError(
                f"{self._filepath} not writable - directory does not exist"
            )
        else:
            self._touched = True

    def __del__(self):
        if self._touched and os.path.isfile(self._filepath):
            try:
                os.unlink(self._filepath)
            except FileNotFoundError:
                pass

    def acquire(self, blocking=False, timeout=None):
        """Acquire flock() on file at path `filepath`. Requires write access to file.

        :param blocking: whether attempt should block.
        :param timeout: maximum wait to repeat non-blocking attempts. Only used if
            `blocking` is false.
        """
        logger.debug(f"Acquire flock() on {self._filepath}")
        self._file = open(self._filepath, "w")
        logger.debug(f"Opened file as: {self._file}")
        if blocking:
            try:
                fcntl.flock(self._file, fcntl.LOCK_EX)
                self._threadlock.__enter__()
                return True
            except OSError:
                return False
        elif timeout is not None:
            timeout += time.time()
            while time.time() < timeout:
                try:
                    fcntl.flock(self._file, fcntl.LOCK_EX | fcntl.LOCK_NB)
                    self._threadlock.__enter__()
                    return True
                except OSError:
                    time.sleep(0.01)
            return False
        else:
            try:
                fcntl.flock(self._file, fcntl.LOCK_EX | fcntl.LOCK_NB)
                self._threadlock.__enter__()
                return True
            except OSError:
                return False

    def __enter__(self):
        """Context handler.

        Acquires in non-blocking way - raises exception on failure.
        """
        if not self.acquire():
            raise OSError(f"flock() failed on {self._filepath} - lock already taken")
        return self

    def release(self):
        """Release flock() on file at path `filepath`. Requires write access to file.

        :param blocking: whether attempt should block.
        :param timeout: maximum wait to repeat non-blocking attempts. Only used if
            `blocking` is false.
        """
        self._threadlock.__exit__()
        if self._file is not None:
            fcntl.flock(self._file, fcntl.LOCK_UN)
        if self._file is not None:
            self._file.close()
            self._file = None

    def __exit__(self, exception_type, exception_value, exception_traceback):
        """Close context handler."""
        self.release()


class LOCK_NOT_ACQUIRED:
    """Dummy enum-like class to represent return value when lock could not be acquired
    by `acquire_lock()`."""

    pass


def acquire_lock(try_once=False, timeout=0.1):
    """Instrument Member Functon Decorator that attempts to obtain a global mutex on the
    singleton class of the function this decorator decorated.

    Can be disabled by passing keyword argument `no_lock=True` in a call to the
    decorated function.

    :param try_once: If true, attempt to get lock doesn't block. Returns
        `LOCK_NOT_ACQUIRED` on failure if true, continuously retries if false.
    :param timeout: Number of seconds to retry for until timeout. If = -1, infinite.

    :return: Wrapped function.

    .. note:: DECORATOR
    """

    def _acquire_lock(function):
        @wraps(function)
        def wrapper(self, *args, **kwargs):
            logger.debug(
                f"Checking lock requirements for call to {function.__name__}..."
            )
            acquired = False
            needs_release = False
            try_once_ = try_once
            timeout_ = timeout

            # Parse kwargs
            if "try_once" in kwargs:
                try_once_ = kwargs.pop("try_once")
            if "timeout" in kwargs:
                timeout_ = kwargs.pop("timeout")
            if "no_lock" in kwargs:
                logger.debug("Lock bypassed...")
                acquired = kwargs.pop("no_lock")

            timeout_ = 0 if timeout_ is None else timeout_

            # Try to acquire lock, or ignore if no_lock bypass passed
            if not acquired:
                logger.debug(
                    f"Attempting to acquire lock for call to {function.__name__}..."
                )
                acquired = self._lock.acquire(
                    blocking=(not try_once_), timeout=timeout_
                )
                needs_release = True

            if acquired:
                if needs_release:
                    logger.debug("Lock acquired \\/")
                try:
                    rval = function(self, *args, **kwargs)
                finally:
                    if needs_release:
                        logger.debug("Lock released /\\")
                        self._lock.release()
            else:
                if try_once:
                    rval = LOCK_NOT_ACQUIRED
                else:
                    raise InstrumentTimeoutError(
                        function,
                        type(self),
                        f"Timeout on instrument of class {type(self).__name__} "
                        f"executing function {function.__name__}.",
                    )
            return rval

        return wrapper

    return _acquire_lock


def retry_on_fail(attempts=3, exceptions=tuple()):
    """Instrument Member Functon Decorator that attempts to retry the given instrument
    member function on failure, after reconnecting to instrument, if required (via
    context closure and re-establishment).

    :param attempts: Number of times to retry function call.
    :param exceptions: Exception classes to catch and ignore.

    :return: Wrapped function.

    .. note:: DECORATOR
    """

    def _retry_on_fail(function):
        @wraps(function)
        def wrapper(self, *args, **kwargs):
            attempts_ = attempts - 1
            # Parse kwargs
            if "attempts" in kwargs:
                attempts_ = kwargs.pop("attempts") - 1
            max_attempts = attempts_

            if not self._connected:
                raise RuntimeError(
                    f"Function {function.__name__} on instrument {type(self).__name__} "
                    "requires active connection - please make sure to call __enter__() "
                    "via with ...: or similar."
                )

            if attempts_ == 0:
                # Bypass this function - do not retry or check exceptions
                return function(self, *args, **kwargs)

            # Try to run function, catch exceptions
            logger.debug(
                f"Trying {function.__name__} up to {max_attempts + 1} times..."
            )
            while True:
                logger.debug(
                    f"Attempt {max_attempts - attempts_ + 1}/{max_attempts + 1}..."
                )
                try:
                    return function(self, *args, **kwargs)
                except Exception as e:
                    logger.debug(str(e))
                    logger.debug(traceback.format_exc())
                    if type(e) not in exceptions or attempts_ == 0:
                        logger.warn("Failure... unknown exception...")
                        raise
                    else:
                        # Try to re-establish context
                        logger.warn("Failure... try to renew connection.")
                        self.__exit__(recover_attempt=True, no_lock=True)
                        self.__enter__(recover_attempt=True, no_lock=True)
                attempts_ -= 1

        return wrapper

    return _retry_on_fail


class Instrument:
    """
    Base Instrument class - all instruments should derive from this. Will be
    singleton'd at implementation level.

    Implements the global lock for each instrument to ensure only one instruction is
    executed at a time.

    Tracks connected/disconnected state. Connection occurs within the context handler
    (see an implementation).
    """

    class Channel:
        """
        Base Channel class - all channel types should derive from this.

        Provides a context handler for handling the connected instrument(s) context.
        Tracks connected/disconnected state of associated instrument.
        """

        def __init__(self, instrument):
            """Constructor.

            :param instrument: class:`Instrument` or lower-level
                class:`Instrument.Channel` object this Channel should be associated
                with.
            """
            self._instrument = instrument
            self._exit_instrument = False

        @property
        def instrument(self):
            """
            :returns: associated instrument.
            :rtype: icicle.instrument.Instrument
            """
            return self._instrument

        @property
        def status(self):
            """
            :returns: channel status. If not implemented in channel, returns ``0xDEAD``.
            :rtype: bool
            """
            return 0xDEAD

        @property
        def connected(self):
            """
            :returns: whether underlying instrument/channel is connected.
            :rtype: bool
            """
            if callable(self._instrument.connected):
                return self._instrument.connected()
            else:
                return self._instrument.connected

        def __enter__(self):
            """Opens context of underlying instrument.

            :returns: channel object with open context.
            :rtype: class:`Instrument.Channel`
            """
            self._instrument = self._instrument.__enter__()
            return self

        def __exit__(self, exception_type, exception_value, traceback):
            """Closes context of underlying instrument."""
            self._instrument.__exit__(exception_type, exception_value, traceback)
            self._exit_instrument = False

    class PowerChannel(Channel):
        """Base class for single power output channels encompassing a voltage, current,
        and associated limits.

        In general two :class:`Instrument.MeasureChannel`
        objects should be provided to this class to measure the current voltage and
        current.
        """

        def __init__(self, instrument, channel, measure_voltage, measure_current):
            """Constructor.

            :param instrument: Instrument this :class:`Instrument.PowerChannel` should
                be associated with.
            :param channel: Physical output or channel number on device.
            :param measure_voltage: :class:`Instrument.MeasureChannel` to be used for
                voltage feedback.
            :param instrument: :class:`Instrument.MeasureChannel` to be used for
                current feedback.
            """
            super().__init__(instrument)
            self._channel = channel
            self._measure_voltage = measure_voltage
            self._measure_current = measure_current

        @property
        def channel(self):
            """
            :returns: which output is being controlled by this PowerChannel.
            :rtype: bool
            """
            return self._channel

        @property
        def state(self):
            """
            :returns: whether this power channel is on or off (True/False).
            :rtype: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @state.setter
        def state(self, value):
            """:value: whether to turn this power channel on or off (True/False).

            :type value: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def voltage(self):
            """
            :returns: currently set channel voltage.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @voltage.setter
        def voltage(self, value):
            """:value: new channel voltage to set.

            :type value: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def current(self):
            """
            :returns: currently set channel current.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @current.setter
        def current(self, value):
            """:value: new channel current to set.

            :type value: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def voltage_limit(self):
            """
            :returns: currently set channel voltage limit.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @voltage_limit.setter
        def voltage_limit(self, value):
            """:value: new channel voltage limit to set.

            :type value: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def current_limit(self):
            """
            :returns: currently set channel current limit.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @current_limit.setter
        def current_limit(self, value):
            """:value: new channel voltage current to set.

            :type value: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def voltage_trip(self):
            """
            :returns: whether voltage limit has been tripped on this channel.
            :rtype: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @voltage_trip.setter
        def voltage_trip(self, value):
            """:value: pass ``False`` to reset voltage trip.

            :type value: bool
            """
            if not value:
                self.reset_voltage_trip()

        def reset_voltage_trip(self):
            """Attempts to reset voltage trip condition.

            :returns: success/failure of operation.
            :rtype: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def current_trip(self):
            """
            :returns: whether current limit has been tripped on this channel.
            :rtype: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @current_trip.setter
        def current_trip(self, value):
            """:value: pass ``False`` to reset current trip.

            :type value: bool
            """
            if not value:
                self.reset_current_trip()

        def reset_current_trip(self):
            """Attempts to reset current trip condition.

            :returns: success/failure of operation.
            :rtype: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def measure_voltage(self):
            """
            :returns: measured (actual) voltage on channel.
            :type value: float
            """
            return self._measure_voltage

        @property
        def measure_current(self):
            """
            :returns: measured (actual) current on channel.
            :type value: float
            """
            return self._measure_current

        def sweep(
            self,
            target_value,
            delay=1,
            step_size=None,
            n_steps=None,
            measure=False,
            set_property="voltage",
            measure_function=None,
            measure_args=None,
            measure_fields=None,
            log_function=None,
            spacing=17,
            execute_each_step=None,
            break_loop=None,
        ):
            """Implementation of a voltage/current sweep.

            :param target_value: value of setting to sweep to (from current)
            :param delay: delay between sweep steps (in seconds)
            :param step_size: step size between sweep steps
            :param n_steps: number of steps to perform (alternative to step_size -
                specify one but not both)
            :param measure: whether to measure at each step of sweep
            :param set_property: whether to sweep `voltage` or `current`
            :param measure_function: function to use for additional `measure()` call,
                e.g. on another instrument. The data is appended to the existing set
                and measure data returned from the channel.
            :param measure_args: additional keyword arguments to pass to `measure()`
            :param measure_fields: field description to put in log header for measure
                fields
            :param log_function: function to call to send text for logging
            :param spacing: spacing between columns in log output
            :param execute_each_step: accepts a callback function (no arguments) to be
                executed at each step of the sweep.
            :param break_loop: accepts a callback function that should evaluate to a
                boolean. If true it will break the sweep loop.

            :return: final parameter value, data
            """
            assert set_property in ("voltage", "current")

            if not int(self.state):  # sometimes state returns string
                logger.warn("Channel is OFF - why are we sweeping?")

            start_value = getattr(self, set_property)
            sweep_distance = target_value - start_value
            if step_size is not None and n_steps is not None:
                raise RuntimeError(
                    "Conflicting arguments to PowerChannel.sweep(): "
                    "step_size and n_steps were both specified"
                )
            elif n_steps is not None:
                step_size = sweep_distance / n_steps
            elif step_size is not None:
                step_size = step_size if sweep_distance >= 0 else -1 * step_size
                n_steps = ceil(sweep_distance / step_size)
            else:
                raise RuntimeError(
                    "Invalid arguments to PowerChannel.sweep(): "
                    "step_size and n_steps were both None"
                )

            ret = []

            if callable(log_function):
                log_function(
                    "\t".join(
                        [
                            "Time".ljust(spacing),
                            "Step".ljust(spacing),
                            "Voltage".ljust(spacing),
                            "Current".ljust(spacing),
                        ]
                        + (
                            [
                                "Meas. Volt.".ljust(spacing),
                                "Meas. Curr.".ljust(spacing),
                            ]
                            if measure
                            else []
                        )
                        + (
                            [f.ljust(spacing) for f in measure_fields]
                            if (callable(measure_function) and measure_fields)
                            else []
                        )
                    )
                )

            for step in range(n_steps + 1):
                if step < n_steps:
                    new_value = start_value + step_size * step
                else:
                    new_value = target_value

                setattr(self, set_property, new_value)

                data = [time.time(), step, self.voltage, self.current]

                if measure:
                    data.extend(
                        [
                            self.measure_voltage.value,
                            self.measure_current.value,
                        ]
                    )

                if callable(measure_function):
                    val = measure_function(**measure_args)
                    if isinstance(val, list) or isinstance(val, tuple):
                        data.extend(val)
                    else:
                        data.append(val)

                if callable(log_function):
                    log_function(
                        "\t".join(
                            [
                                time.strftime("%x %X").ljust(spacing),
                                *[str(val).ljust(spacing) for val in data[1:]],
                            ]
                        )
                    )

                if callable(execute_each_step):
                    execute_each_step()

                if callable(break_loop) and break_loop():
                    break

                ret.append(data)
                time.sleep(delay)

            end_value = getattr(self, set_property)
            return end_value, ret

    class MeasureChannel(Channel):
        """Generic measurement channel for e.g. digital multimeter.

        Accepted measurement
        types should be configured via the ``MEASURE_TYPES`` dictionary in the relevant
        instrument class.
        """

        def __init__(self, instrument, channel, measure_type, unit=""):
            """Constructor.

            :param instrument: Instrument this :class:`Instrument.MeasureChannel` should
                be associated with.
            :type instrument: `:class:Instrument`, :class:`Instrument.Channel`
            :param channel: Physical output or channel number on device.
            :type channel: int
            :param measure_type: Type of measurement to be performed. Instrument-
                specific, but should follow usual rules. e.g. ``VOLT:DC``.
            :type measure_type: str
            :param unit: :class:`Instrument.MeasureChannel` to be used for
                current feedback.
            :type unit: str
            """
            super().__init__(instrument)
            self._channel = channel
            self._measure_type = measure_type
            self._unit = unit

        @property
        def channel(self):
            """
            :returns: which physical output, channel or connection is being controlled
                by this PowerChannel.
            :rtype: bool
            """
            return self._channel

        @property
        def measure_type(self):
            """
            :returns: measurement type.
            :rtype: str
            """
            return self._measure_type

        @property
        def value(self):
            """Performs measurement and returns value.

            :returns: measured value.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - this is not a valid MeasureChannel "
                f"implementation!"
            )

        @property
        def unit(self):
            """
            :returns: measurement unit.
            :rtype: str
            """
            return self._unit

        def as_string(self):
            """
            :returns: value and unit as string.
            :rtype: str
            """
            return f"{self.value} {self.unit}"

    class MeasureChannelWrapper(MeasureChannel):
        def __init__(self, wrapped_channel, wrapper_type, unit=""):
            """Constructor.

            :param wrapped_channel: MeasureChannel this
                :class:`Instrument.MeasureChannelWrapper` should be associated with.
            :type wrapped_channel: :class:`Instrument.MeasureChannel`
            :param wrapper_type: Wrapper type.
            :param unit: unit of wrapped measurement.
            :type unit: str
            """
            super().__init__(wrapped_channel, None, wrapper_type, unit)
            if unit is None:
                self._unit = self._instrument.unit
            else:
                self._unit = unit

        @property
        def measure_type(self):
            """
            :returns: wrapped measurement type.
            :rtype: str
            """
            return f"{self._measure_type}({self._instrument.measure_type})"

        @property
        def wrapper_type(self):
            """
            :returns: measurement wrapper type.
            :rtype: str
            """
            return self._instrument.measure_type

        @property
        def value(self):
            """Performs measurement in wrapped :class:`MeasureChannel` and returns
            transformed result.

            :returns: measured value.
            :rtype: float
            """
            raise NotImplementedError()

        @property
        def unit(self):
            # default to wrapped unit
            return self._instrument.unit

        @property
        def status(self):
            """
            :returns: channel status, defaulting to wrapped instrument status if
                not overrriden in subclass.
            :rtype: str
            """
            # default to wrapped status
            return self._instrument.status

        def as_string(self):
            return f"{self.value} {self.unit}"

    class TemperatureChannel(Channel):
        """Base class for single temperature control channel encompassing a temperature,
        and a speed control.

        In general two :class:`Instrument.MeasureChannel`
        objects should be provided to this class to measure the current temperature and
        speed.
        """

        def __init__(self, instrument, channel, measure_temperature, measure_speed):
            """Constructor.

            :param instrument: Instrument this :class:`Instrument.PowerChannel` should
                be associated with.
            :param channel: Physical output or channel number on device.
            :param measure_temperature: :class:`Instrument.MeasureChannel` to be used
                for temperature feedback.
            :param measure_speed: :class:`Instrument.MeasureChannel` to be used for
                speed feedback.
            """
            super().__init__(instrument)
            self._channel = channel
            self._measure_temperature = measure_temperature
            self._measure_speed = measure_speed

        @property
        def channel(self):
            """
            :returns: which output is being controlled by this PowerChannel.
            :rtype: bool
            """
            return self._channel

        @property
        def state(self):
            """
            :returns: whether this power channel is on or off (True/False).
            :rtype: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @state.setter
        def state(self, value):
            """:value: whether to turn this power channel on or off (True/False).

            :type value: bool
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def temperature(self):
            """
            :returns: currently set channel temperature.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @temperature.setter
        def temperature(self, value):
            """:value: new channel temperature to set.

            :type value: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def speed(self):
            """
            :returns: currently set speed current.
            :rtype: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @speed.setter
        def speed(self, value):
            """:value: new channel speed to set.

            :type value: float
            """
            raise NotImplementedError(
                f"{type(self._instrument).__name__} does not implement "
                f"{inspect.stack()[0][3]} - has fallen back on Instrument."
                f"{type(self).__name__}!"
            )

        @property
        def measure_temperature(self):
            """
            :returns: measured (actual) voltage on channel.
            :type value: float
            """
            return self._measure_temperature

        @property
        def measure_speed(self):
            """
            :returns: measured (actual) current on channel.
            :type value: float
            """
            return self._measure_speed

    class LockType(Enum):
        MUTEX = 0
        LOCKFILE = 1
        FLOCK = 2

    _LOCK_TYPE = os.environ.get("ICICLE_LOCK_TYPE") or "FLOCK"
    LOCK_TYPE = (
        LockType.MUTEX
        if _LOCK_TYPE.upper() == "MUTEX"
        else (LockType.LOCKFILE if _LOCK_TYPE.upper() == "LOCKFILE" else LockType.FLOCK)
    )  # if Instrument._LOCK_TYPE.upper() == 'FLOCK'

    _LOCK_PATH = os.environ.get("ICICLE_LOCK_PATH")
    LOCK_PATH = (
        Path(_LOCK_PATH).resolve()
        if _LOCK_PATH
        else (
            Path("/var/lock/")
            if (LOCK_TYPE == LockType.LOCKFILE and os.access("/var/lock/", os.W_OK))
            else (os.makedirs("/tmp/icicle/", exist_ok=True) or Path("/tmp/icicle/"))
        )
    )

    registered_classes = dict()
    """Dynamic point for registration of instrument classes."""

    @classmethod
    def register(cls, child):
        """Class decorator to register a device-level class:`Instrument` subclass with
        the :class:`Instrument` class registry for later lookup from name strings.

        Usage::

            @Instrument.register
            class TestInstrument(Instrument):
                ...

        :param child: the child class to be registered.
        :type child: class:`Instrument`
        :returns: child class, after decorator step.
        :rtype: class:`Instrument`
        """
        cls.registered_classes[child.__name__] = child
        return child

    def get_lockfile(self):
        if "ASRL" in self._resource:
            dev = realpath(self._resource.replace("ASRL", "").replace("::INSTR", ""))
            tty = dev.split("/")[-1]
            return f"LCK..{tty}"
        else:
            res = (
                self._resource.replace("::INSTR", "")
                .replace("/", "__")
                .replace(":", "--")
            )
            return f"LCK..{res}"

    def get_flock(self):
        if self._resource.startswith("ASRL"):
            # Lock the actual serial device file
            dev = realpath(self._resource.replace("ASRL", "").replace("::INSTR", ""))
            if os.path.exists(dev):
                return dev
        # /dev/ handle doesn't exist for this resource - need to make lockfile instead
        res = (
            self._resource.replace("::INSTR", "").replace("/", "__").replace(":", "--")
        )
        return os.path.join(self.LOCK_PATH, res)

    def __init__(self, resource):
        """Instrument Constructor.

        Constructs lock.
        """
        self._resource = resource
        if self.LOCK_TYPE == Instrument.LockType.MUTEX:
            self._lock = Lock()
        elif self.LOCK_TYPE == Instrument.LockType.LOCKFILE:
            self._lock = InterProcessLock(
                os.path.join(self.LOCK_PATH, self.get_lockfile())
            )
        else:
            self._lock = Flock(self.get_flock())
        logger.debug(
            f"{type(self).__name__} instrument (resource {self._resource}) has lock: "
            f"{self._lock}"
        )
        self._connected = 0
        self._channels = {
            type(self).MeasureChannel: {},
            type(self).PowerChannel: {},
            type(self).TemperatureChannel: {},
            type(self).MeasureChannelWrapper: {},
        }
        pass

    @property
    def resource(self):
        """
        :returns: Resource string or identifier for this instrument.
        :rtype: str
        """
        return self._resource

    @property
    def sim(self):
        """
        :returns: Whether this instrument instance is set up with a simulation backend.
        :rtype: bool
        """
        return False

    def validate_channel(self, channel, raise_exception=False):
        """Check if a channel exists on this device.

        :param channel: Channel number to validate as an output.
        """
        if not raise_exception:
            return False
        else:
            raise ChannelError(
                f"channel {channel} does not exist or is not enabled on this " "device."
            )

    def channel(self, channel_type, channel, **kwargs):
        """Gets an associated :class:`Instrument.MeasureChannel`,
        :class:`Instrument.PowerChannel` or :class:`Instrument.WrapperChannel`
        """
        if isinstance(channel_type, str):
            if "WRAPPER" in channel_type.upper() and "MEASURE" in channel_type.upper():
                channel_type = type(self).MeasureChannelWrapper
            elif "MEASURE" in channel_type.upper():
                channel_type = type(self).MeasureChannel
            elif "POWER" in channel_type.upper():
                channel_type = type(self).PowerChannel
            elif "TEMPERATURE" in channel_type.upper():
                channel_type = type(self).TemperatureChannel
        if isinstance(channel_type, type):
            if issubclass(channel_type, Instrument.MeasureChannelWrapper):
                channel_type = type(self).MeasureChannelWrapper
            elif issubclass(channel_type, Instrument.MeasureChannel):
                channel_type = type(self).MeasureChannel
            elif issubclass(channel_type, Instrument.PowerChannel):
                channel_type = type(self).PowerChannel
            elif issubclass(channel_type, Instrument.TemperatureChannel):
                channel_type = type(self).TemperatureChannel

        assert (
            issubclass(channel_type, Instrument.Channel)
            and channel_type in self._channels
        )

        self.validate_channel(channel, raise_exception=True)

        if channel_type is type(self).PowerChannel:
            if type(self).PowerChannel is Instrument.PowerChannel:
                raise ChannelError(
                    f"{type(self).__name__} does not implement "
                    "PowerChannel high-level interface"
                )
            if channel in self._channels[channel_type]:
                return self._channels[channel_type][channel]
            else:
                self._channels[channel_type][channel] = type(self).PowerChannel(
                    self,
                    channel,
                    self.channel(
                        type(self).MeasureChannel,
                        channel,
                        measure_type="VOLT:DC",
                        unit="V",
                    ),
                    self.channel(
                        type(self).MeasureChannel,
                        channel,
                        measure_type="CURR:DC",
                        unit="A",
                    ),
                    *kwargs,
                )
                return self._channels[channel_type][channel]

        if channel_type is type(self).TemperatureChannel:
            if type(self).TemperatureChannel is Instrument.TemperatureChannel:
                print(type(self), Instrument.TemperatureChannel)
                raise ChannelError(
                    f"{type(self).__name__} does not implement "
                    "TemperatureChannel high-level interface"
                )
            if channel in self._channels[channel_type]:
                return self._channels[channel_type][channel]
            else:
                self._channels[channel_type][channel] = type(self).TemperatureChannel(
                    self,
                    channel,
                    self.channel(
                        type(self).MeasureChannel,
                        channel,
                        measure_type="TEMPERATURE",
                        unit="degC",
                    ),
                    self.channel(
                        type(self).MeasureChannel,
                        channel,
                        measure_type="SPEED",
                        unit="RPM",
                    ),
                    *kwargs,
                )
                return self._channels[channel_type][channel]

        if channel_type is type(self).MeasureChannel:
            assert "measure_type" in kwargs
            measure_type = kwargs.pop("measure_type").upper()
            if type(self).MeasureChannel is Instrument.MeasureChannel:
                raise ChannelError(
                    f"{type(self).__name__} does not implement "
                    "MeasureChannel high-level interface"
                )
            assert measure_type.upper() in self.MEASURE_TYPES
            if channel not in self._channels[channel_type]:
                self._channels[channel_type][channel] = {}
            if measure_type in self._channels[channel_type][channel]:
                return self._channels[channel_type][channel][measure_type]
            else:
                self._channels[channel_type][channel][measure_type] = type(
                    self
                ).MeasureChannel(self, channel, measure_type)
                return self._channels[channel_type][channel][measure_type]

        if channel_type is type(self).MeasureChannelWrapper:
            assert "wrapped_channel" in kwargs
            assert "wrapper_type" in kwargs
            wrapped_channel = kwargs.pop("wrapped_channel")
            wrapper_type = kwargs.pop("wrapper_type")

            if type(self).MeasureChannel is Instrument.MeasureChannelWrapper:
                raise ChannelError(
                    f"{type(self).__name__} does not implement "
                    "MeasureChannel high-level interface"
                )
            assert wrapper_type.upper() in self.MEASURE_WRAPPER_TYPES
            if channel not in self._channels[channel_type]:
                self._channels[channel_type][channel] = {}
            if wrapper_type not in self._channels[channel_type][channel]:
                self._channels[channel_type][channel][wrapper_type] = {}
            if wrapped_channel in self._channels[channel_type][channel][wrapper_type]:
                return self._channels[channel_type][channel][wrapper_type][
                    wrapped_channel
                ]
            else:
                self._channels[channel_type][channel][wrapper_type][wrapped_channel] = (
                    type(self).MeasureChannelWrapper(
                        wrapped_channel, channel, wrapper_type, self
                    )
                )
                return self._channels[channel_type][channel][wrapper_type][
                    wrapped_channel
                ]

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Must establish connection to instrument. The context handlers per-instrument
        need to effectively reference count as instruments are Singleton'd or Multiton'd
        to individual instances.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.

        :returns: Instrument object with activated context.
        """
        self._connected += 1
        logger.debug(
            f"[{type(self).__name__}({self.resource}).__enter__]: Context "
            f"reference count now at {self._connected}"
        )
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Must gracefully (or otherwise) terminate connection with instrument. The
        context handlers per-instrument need to effectively reference count as
        instruments are Singleton'd or Multiton'd to individual instances.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.
        """
        self._connected -= 1
        logger.debug(
            f"[{type(self).__name__}({self.resource}).__exit__]: Context "
            f"reference count now at {self._connected}"
        )
        assert self._connected >= 0
        pass

    def connected(self):
        return self._connected > 0

    @acquire_lock()
    @retry_on_fail()
    def reset(self, *args, **kwargs):
        """
        RESET template - should be implemented to reset device.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure reset() is implemented."
        )

    @acquire_lock()
    @retry_on_fail()
    def on(self, *args, **kwargs):
        """
        ON template - should be implemented to turn on device or channel.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure on() is implemented."
        )

    @acquire_lock()
    @retry_on_fail()
    def off(self, *args, **kwargs):
        """
        OFF template - should be implemented to turn off device or channel.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure off() is implemented."
        )

    @acquire_lock()
    @retry_on_fail()
    def status(self, *args, **kwargs):
        """
        STATUS template - should be implemented to return device/channel status.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure status() is implemented."
        )

    @acquire_lock()
    @retry_on_fail()
    def set(self, *args, **kwargs):
        """
        SET template - should be implemented to set and query back device state.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure set() is implemented."
        )

    @acquire_lock()
    @retry_on_fail()
    def query(self, *args, **kwargs):
        """
        QUERY template - should be implemented to query device state.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure query() is implemented."
        )

    @acquire_lock()
    @retry_on_fail()
    def measure(self, *args, **kwargs):
        """
        MEASURE template - should be implemented to make measurement on device.
        """
        raise NotImplementedError(
            "Instrument class is not a full implementation. Please use appropriate "
            "subclass and make sure measure() is implemented."
        )

    def open(self):
        self.__enter__()

    def close(self):
        self.__exit__()

    @acquire_lock()
    @retry_on_fail()
    def sweep(
        self,
        target_setting,
        target_value,
        delay=1,
        step_size=None,
        n_steps=None,
        measure=False,
        set_function=None,
        set_args=None,
        query_function=None,
        query_args=None,
        measure_function=None,
        measure_args=None,
        conversion=lambda k: float(k),
        log_function=None,
        execute_each_step=None,
        break_monitoring=None,
    ):
        """Device-independent implementation of a parameter sweep.

        .. warning: Log_function header printing is currently hacked to work with
            Keithley2410 (only) I think for nice CLI output... this should be fixed!

        :param target_setting: name of setting that `set()` call should target.
        :param target_value: value of setting to sweep to (from current).
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param n_steps: number of steps to perform (alternative to step_size -
            specify one but not both)
        :param set_function: function to use for `set()` call. Defaults to `self.set`
        :param set_args: additional keyword arguments to pass to `set()`
        :param measure: whether to measure at each step of sweep.
        :param measure_function: function to use for `measure()` call.
            Defaults to `self.measure`
        :param measure_args: additional keyword arguments to pass to `measure()`
        :param query_args: additional keyword arguments to pass to `query()` at each
            step.
        :param conversion: function to convert query return value back to float.
        :param execute_each_step: accepts a callback function (no arguments) to be
            executed at each step of the sweep.
        :param break_monitoring: accepts a callback function that should evaluate to a
            boolean. If true it will break the sweep loop.

        :return: final parameter value[, measure data]
        """
        SPACING = 17

        if set_args is None:
            set_args = {}
        if measure_args is None:
            measure_args = {}
        if query_args is None:
            query_args = {}
        if set_function is None:
            set_function = type(self).set
        if query_function is None:
            query_function = type(self).query
        if measure_function is None:
            measure_function = self.measure
        if "set_channel" in set_function.__name__ and "channel" in set_args:
            s_channel = set_args.pop("channel")
            set_function_ = set_function
            set_function = lambda s, t, *v, **k: set_function_(  # noqa: E731
                s, t, s_channel, *v, **k
            )

        if "query_channel" in query_function.__name__ and "channel" in query_args:
            q_channel = query_args.pop("channel")
            query_function_ = query_function
            query_function = lambda s, t, *a, **k: query_function_(  # noqa: E731
                s, t, q_channel, *a, **k
            )

        meas = query_function(self, target_setting, **query_args, no_lock=True)
        current_value = conversion(meas) if conversion else meas

        # Already there
        if abs(current_value - target_value) < 1e-12:
            return current_value

        # if step_size is None and n_steps is not None:
        #    step_size = float(target_value - current_value) / n_steps
        if n_steps is None and step_size is not None:
            n_steps = math.ceil(abs(float(target_value - current_value) / step_size))
        else:
            raise ChannelError(
                "Exactly one of step_size and n_steps must be provided - not neither or"
                "both!"
            )
        # Enforce integer n_steps
        n_steps = int(math.floor(n_steps))
        if log_function:
            log_function(
                f"Will sweep {target_setting} to {target_value} in {n_steps} steps of "
                f"size {step_size}"
            )

        if log_function:
            if "measured_unit" in measure_args:
                measured_unit = measure_args.get("measured_unit")
            else:
                measured_unit = None
            log_function(
                "\t".join(
                    val.ljust(SPACING) for val in self.sweep_print_header(measured_unit)
                )
            )
            # ('# Time', 'Voltage (V)', 'Current (A)', 'Resistance', 'PSU Time',
            #  'Status Register')))

        # Data collection
        if measure or log_function:
            data = []
        # Sweep target:
        step_size = (target_value - current_value) / n_steps
        for step in range(n_steps + 1):
            intermediate_value = (
                current_value + step * step_size
            )  # replaces numpy.linspace(current_value, target_value, num=n_steps+1):
            if execute_each_step:
                execute_each_step()
            if break_monitoring:
                if break_monitoring():
                    break
            set_function(
                self,
                target_setting,
                intermediate_value,
                no_lock=True,
                **set_args,
            )
            time.sleep(delay)
            if measure or log_function:
                temp = measure_function(**measure_args, no_lock=True)
                # We need time.time() only because only a starred function would not be
                # accepted by the compiler. Unfortunately the timestamp is not recorded
                # where it should be...
                mtime = time.time()
                # If not 2d array, make 2d - one repetition
                if not isinstance(temp[0], list) or isinstance(temp[0], tuple):
                    temp = (temp,)

                for measurement in temp:
                    data.append([mtime, *measurement])
                    if log_function:
                        log_function(
                            "\t".join(
                                [
                                    time.strftime("%x %X").ljust(SPACING),
                                    *[str(val).ljust(SPACING) for val in data[-1][1:]],
                                ]
                            )
                        )

        meas = query_function(self, target_setting, **query_args, no_lock=True)
        final_value = conversion(meas) if conversion else meas
        if measure:
            return final_value, data
        else:
            return final_value

    def sweep_print_header(self, measured_unit):
        """Function that returns the specific header for each of the instruments. This
        should be overridden in each of the instruments that perform a sweep.

        In some cases like sldo_vi_curve, self is not the device that performs the
        measurement. Therefore, this function sometimes needs the information about what
        is measured. This is what @param measured_unit is for.
        """
        return ("Please", "Override", "this", "sweep_print_header")

    @staticmethod
    def load_instrument_class(instrument, instrument_class):
        """Static function that finds an instrument class and loads it based on the
        module name and class name.

        :param instrument: Name of instrument module (e.g. `keithley2410`)
        :param instrument_class: Class of instrument (e.g. `Keithley2410`)
        :return: instrument class
        """
        return getattr(
            importlib.import_module(
                f".{instrument}", package=".".join(__name__.split(".")[:-1])
            ),
            instrument_class,
        )
