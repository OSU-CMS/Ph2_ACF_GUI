"""Binder class for climate chamber.

This climate chamber is not a SCPI or VISA instrument. Therefore we are using a custom
library for control with it.
This custom library is located at: `utils.binderctl`.
"""

from time import sleep
from operator import le, ge
import logging

from .instrument import Instrument, Multiton, acquire_lock, retry_on_fail
from .utils.binderctl import BinderCtl


logger = logging.getLogger(__name__)


# default resource "192.168.14.100"

# TODO: link documentation here or upload to a repo folder
# TODO: investigate simulation possibilities for tests?
# TODO: investigate more elegant way to set Switches
# (read switch values at setting function).


@Instrument.register
class Binder(Instrument, metaclass=Multiton, key="resource"):
    NO_SIMULATION = True
    # TODO: These constants should be documented.
    PORT = 10001
    TIMEOUT = 2.5
    RETRIES = 3

    AUTO_COND_BIT = 1 << 6
    IDLE_MODE_BIT = 1 << 1
    DRY_AIR_BIT = 1 << 2
    FAN_BIT = 1 << 3

    REGISTER = {
        "rTemp": (0x1004, float),
        "rHumidity": (0x100A, float),
        "rSetpointTemp": (0x10B2, float),
        "wSetpointTemp": (0x114C, float),
        "rSetpointHumidity": (0x10B4, float),
        "wSetpointHumidity": (0x114E, float),
        "rSwitches": (0x1292, int),
        "wSwitches": (0x1158, int),
        "rStart": (0x1149, float),
        "wStart": (0x1149, float),
        "rStop": (0x114A, float),
        "wStop": (0x114A, float),
        "rProgram": (0x1147, float),
        "wProgram": (0x1147, float),
    }

    MEASURE_TYPES = {
        "TEMP": "Temperature (°C)",
        "HUMI": "Rel. Humidity (%)",
    }
    """Measurement types for this instrument."""

    MEASURE_COMMAND_MAP = {"TEMP": "rTemp", "HUMI": "rHumidity"}

    MONITOR_KEYS = [
        "rTemp",
        "rHumidity",
        "rSetpointTemp",
        "rSetpointHumidity",
        "rSwitches",
    ]

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for the Binder Climate Chamber."""

        @property
        def status(self):
            """
            :returns: value of ``rSwitches``.
            :rtype: int
            """
            return self._instrument.read_value("rSwitches")

        @property
        def value(self):
            return self._instrument.read_value(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type),
            )

    class TemperatureChannel(Instrument.TemperatureChannel):
        """TemperatureChannel implementation for Binder Climate Chamber."""

        @property
        def status(self):
            """
            :returns: value of ``rSwitches``, which should give a good status overview
            """
            return self._instrument.read_value("rSwitches")

        @property
        def state(self):
            return self._instrument.check_idle()

        @state.setter
        def state(self, value):
            self._instrument.toggle_idle(value)

        @property
        def temperature(self):
            return self._instrument.get_temperature()

        @temperature.setter
        def temperature(self, value):
            self._instrument.set_temperature(value)

        @property
        def speed(self):
            """
            Not implemented/available for this device
            :returns: 0
            """
            return 0

        @speed.setter
        def speed(self, value):
            """
            Not implemented/available for this device
            """
            return 0

    def __init__(self, resource="127.0.0.1", tolerance=2, sim=False):
        """Init function.

        :param resource: an ip adress at which the climate chamber is to be found
        :param tolerance: A temperature tolerance to accept a temperature as reached,
            when the difference is lower than this.
        """
        # Sim argument ignored (for now)
        sim = sim  # Trick flake8
        # This is ok because it only sets up a static interface
        # Every read/write operation through this library requests a separate TCP socket
        self._device = BinderCtl(
            hostname=resource,
            port=self.PORT,
            timeout=self.TIMEOUT,
            retries=self.RETRIES,
        )
        self._tolerance = tolerance
        super().__init__(resource)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        super().__enter__(recover_attempt=recover_attempt, no_lock=True)
        # Maybe it is not so good to put it to idle mode in the end of the scan...
        # self.set_value('wSwitches', status_switches & (0xff ^ self.IDLE_MODE_BIT))
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Function to terminate connection with climate chamber.

        Acually only important because it sets the temperature back to 20C.
        TODO: Maybe let it also go to idle mode. - To be found out how to do this.
        """
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)
        return

    def validate_channel(self, channel, raise_exception=True):
        """Check if an channel exists on this device. Only successful if `channel == 1`.

        :param channel: Channel number to validate as an input
        """
        if channel == 1:
            return True
        else:
            if raise_exception:
                raise RuntimeError(
                    f"Channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    @acquire_lock()
    def toggle_idle(self, enable, verbose=True):
        """Toggles idle switch on the device to put it to idle mode or not.

        Passes `no_lock` and `attempts` to read_value sub-call via `kwargs`.

        :param enable: if True, device is set to idle, else, idle mode is unset.
        :return: status of all switches (not only idle mode switch)
        """
        status_switches = self.read_value("rSwitches", no_lock=True)
        self.set_value("wSwitches", status_switches | self.DRY_AIR_BIT, no_lock=True)
        self.set_value("wSwitches", status_switches | self.FAN_BIT, no_lock=True)
        if enable is True:
            self.set_value(
                "wSwitches", status_switches | self.IDLE_MODE_BIT, no_lock=True
            )
        else:
            self.set_value(
                "wSwitches", status_switches & (0xFF ^ self.IDLE_MODE_BIT), no_lock=True
            )
        return self.read_value("rSwitches", no_lock=True)

    @acquire_lock()
    def check_idle(self, **kwargs):
        """Checks, if idle mode is active or not.

        :return: True, if idle, False, if not idle
        """
        status_switches = self.read_value("rSwitches", no_lock=True)
        return status_switches & self.IDLE_MODE_BIT

    @acquire_lock()
    def toggle_auto_cond(self, turn_on=True, verbose=True, **kwargs):
        """Auto condensation protection mode is a setting of the binder climate chamber.
        It helps to prevent condensation of the module inside the chamber.

        To be turned off when temperature above 20degC.

        Passes `no_lock` and `attempts` to read_value sub-call via `kwargs`.

        This function changes the state of this mode.

        :param turn_on: If True, mode is activated, if False, it is deactivated
        :param verbose: setting for this specific function, if changes are to be
            printed. Useful mostly in cases, where this function is executed
            conditionally by an automated program.
        :return: turn_on parameter value
        """
        status_switches = self.read_value("rSwitches", no_lock=True, **kwargs)
        if turn_on:
            self.set_value(
                "wSwitches", status_switches | self.AUTO_COND_BIT, no_lock=True
            )
            if verbose:
                print(200 * " ", end="\r")
                print("Auto condensation protection turned on")
        else:
            self.set_value(
                "wSwitches", status_switches & (0xFF ^ self.AUTO_COND_BIT), no_lock=True
            )
            if verbose:
                print(200 * " ", end="\r")
                print("Auto condensation protection turned off")
        sleep(0.01)
        return turn_on

    @acquire_lock()
    def check_auto_cond(self, verbose=True, **kwargs):
        """Function to check the status of the condensation protection mode of the
        climate chamber.

        Passes `no_lock` and `attempts` to read_value sub-call via `kwargs`.

        :param verbose: Setting for this specific function to print status.
        :return: True if CP is on, False if CP is off
        """
        status_switches = self.read_value("rSwitches", no_lock=True, **kwargs)
        status_auto_cond = status_switches & self.AUTO_COND_BIT
        if verbose:
            if status_auto_cond:
                print(200 * " ", end="\r")
                print("Auto condensation protection is turned on")
            else:
                print(200 * " ", end="\r")
                print("Auto condensation protection is turned off")
        return status_auto_cond

    @acquire_lock()
    def toggle_switch(self, bit, on):
        """Toggle specific switch of the machine. Can be a generic switch.

        :param bit: position of the bit corresponding to the switch in the register.
        :param on: If True, bit to be set to 1, else bit to be set to 0.
        """
        status_switches = self.read_value("rSwitches", no_lock=True)
        if on:
            self.set_value("wSwitches", status_switches | bit, no_lock=True)
        else:
            self.set_value("wSwitches", status_switches & (0xFF ^ bit), no_lock=True)
        sleep(0.01)
        return self.read_value("rSwitches", no_lock=True) & bit

    @retry_on_fail()
    @acquire_lock()
    def read_value(self, key):
        """Wrapper for a read function with the binderctl.

        :param key: REGISTER to readout
        :return: value in this REGISTER
        """
        cmd = (
            self._device.read_int
            if self.REGISTER[key][1] == int
            else self._device.read_float
        )
        value = cmd(self.REGISTER[key][0])
        sleep(0.01)
        return value

    @retry_on_fail()
    @acquire_lock()
    def set_value(self, key, val):
        """Wrapper for a write function with the binderctl Readback should automatically
        happen with the binderctl class, but just to be sure, this can be activated here
        too.

        :param key: REGISTER to write in
        :param val: Which value to write in
        """
        cmd = (
            self._device.write_int
            if self.REGISTER[key][1] == int
            else self._device.write_float
        )
        cmd(self.REGISTER[key][0], val)
        sleep(0.01)
        return True

    def get_temperature(self, **kwargs):
        """Simple getter method for the current temperature.

        Passes `no_lock` and `attempts` to read_value sub-call via `kwargs`.
        """
        # Locked in single sub-call; pass kwargs
        return self.read_value("rTemp", **kwargs)

    def set_temperature(self, val, **kwargs):
        """Setter function to easier set the temperature instead of using 'set_value'
        directly.

        Passes `no_lock` and `attempts` to set_value sub-call via `kwargs`.

        :param val: temperature to go to
        """
        # Locked in single sub-call; pass kwargs
        return self.set_value("wSetpointTemp", float(val), **kwargs)

    def set_humidity(self, val, **kwargs):
        """Setter function to easier set the temperature instead of using 'set_value'
        directly.

        Passes `no_lock` and `attempts` to set_value sub-call via `kwargs`.

        :param val: temperature to go to
        """
        # Locked in single sub-call; pass kwargs
        return self.set_value("wSetpointHumidity", float(val), **kwargs)

    def probe_temperature(self, target, interval=1, verbose=True):
        """Function to check, if a set temperature is reached. It will automatically
        activate/deactivate the humidity control at 20degC. Returns true, when
        temperature is reached.

        :param target: Temperature to reach (plus/minus tolerance)
        :param interval: Interval / sleep in between measurements
        :param verbose: If measurements are to be printed out
        """
        values = self.measure(verbose=verbose)
        temp = float(values["rTemp"])
        operation = le if temp <= target else ge

        target_tolerance = self._tolerance if operation is ge else -1 * self._tolerance
        while operation(temp, target + target_tolerance):
            values = self.measure(verbose=verbose)
            temp = float(values["rTemp"])
            if temp > 20 and self.check_auto_cond(verbose=False):
                self.toggle_auto_cond(turn_on=False, verbose=verbose)
            if temp < 20 and not self.check_auto_cond(verbose=False):
                self.toggle_auto_cond(turn_on=True, verbose=verbose)
            sleep(interval)
        return True

    @acquire_lock()
    def getVals(self):
        """Helper function to read the values corresponding to the keys in
        `MONITOR_KEYS`.

        :return: dictionary of montior value names and values.
        """
        vals = {}
        for key in self.MONITOR_KEYS:
            vals[key] = self.read_value(key, no_lock=True)
        return vals

    @acquire_lock()
    def measure(self, repetitions=1, verbose=False, **kwargs):
        """Measuring function, that uses `getVals` helper function to run.

        :param repetitions: not currently used here - to match common interface.
        :param verbose: setting for this function specific, if measured values are to
            be printed.
        :return: values received from getVals.
        """
        # transparently pass no_lock and other args
        vals = self.getVals(no_lock=True, **kwargs)
        if verbose is True:
            print(100 * " ", end="\r")
            print(
                "".join(
                    [k + ":  " + "{:.2f}".format(v) + "\t" for k, v in vals.items()]
                ),
                end="\r",
            )
        return vals

    def publish(self, client, vals):
        """Function to upload measured values to an influxDB instance using mqtt.

        :param client: mqtt client to be used for upload
        :param vals: values to be uploaded in the form of return from getVals. If
            `None`, new values are taken from getVals.
        """
        # This generally doesn't match existing semantics for publish() functions
        if vals is None:
            vals = self.getVals()
        # ^^^^^
        if isinstance(vals, str):
            vals = vals.split("\t")
            vals = dict(zip(self.MONITOR_KEYS, vals[-5:]))
        if isinstance(vals, dict):
            for quantity, val in vals.items():
                client.publish(
                    f"/binder/{quantity}", f"binder {quantity}={str(val).strip()}"
                )
        sleep(0.1)
