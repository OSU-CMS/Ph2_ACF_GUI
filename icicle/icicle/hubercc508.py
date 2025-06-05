#!/usr/bin/env python
# -*- coding: utf-8 -*-
# #########################################################################
# Class for Chiller usage -> CC508 from Huber
# [install USBIPD-WIN for WSL2] -
#    IMPORTANT:
#        install root@box:~# pip3 install pySoftcheck-1.20200424.0-py3-none-any.whl
# Unzip downloaded pySoftcheck package from Website. A *.whl file will be extracted.
#    FROM HUBER!!! -> pip install it
# see manual for commands, convert hex codes to dec, and follow same logic
# https://www.huber-online.com/en/download_manuals.aspx ->> Handbook Data Communication
#
# Original author: Gregor Eberwein, Oxford 08.2022
# Adapted for ICICLE by Simon Koch, Oxford 08.2024
#
# #########################################################################

import time
import logging

from softcheck.logic import Com
from softcheck.pb_commands import PbCom

from .instrument import retry_on_fail, acquire_lock, Instrument, ChannelError, Multiton
from .scpi_instrument import ValidationError, truthy, is_numeric

logger = logging.getLogger(__name__)


@Instrument.register
class HuberCC508(Instrument, metaclass=Multiton, key="resource"):
    SETTINGS = {
        "TEMPERATURE_SETPOINT": {
            "SET": 0,
            "QUERY": 0,
            "parser": (lambda x, _: int(x) / 100.0),
            "unit": "C",
            "verifier": is_numeric(min=-60, max=70, scale=100, to_int=True),
        },
        "TEMPERATURE": {
            "QUERY": 1,
            "parser": (lambda x, _: int(x) / 100.0),
            "unit": "C",
        },
        "SPEED_SETPOINT": {
            "SET": 72,
            "QUERY": 72,
            "verifier": is_numeric(min=0, max=5000, to_int=True),
        },
        "SPEED": {"QUERY": 38},
        "STATE": {
            "SET": 20,
            "QUERY": 20,
            "verifier": truthy(true_output=1, false_output=0),
        },
    }

    OUTPUTS = 1
    MEASURE_TYPES = {
        "TEMPERATURE": "Bath Temperature (degC)",
        "SPEED": "Pump Speed (RPM)",
    }
    """Measurement types for this instrument."""

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for Huber CC508."""

        @property
        def value(self):
            return self._instrument.query(self._measure_type)

        @property
        def status(self):
            """
            :returns: Status Byte value.
            """
            return 0

    class TemperatureChannel(Instrument.TemperatureChannel):
        """TemperatureChannel implementation for Huber CC508."""

        @property
        def status(self):
            """
            :returns: Measurement Status Register value.
            """
            return 0

        @property
        def state(self):
            return self._instrument.query("STATE")

        @state.setter
        def state(self, value):
            self._instrument.set("STATE", value)

        @property
        def temperature(self):
            return self._instrument.query("TEMPERATURE_SETPOINT")

        @temperature.setter
        def temperature(self, value):
            self._instrument.set("TEMPERATURE_SETPOINT", value)

        @property
        def speed(self):
            return self._instrument.query("SPEED_SETPOINT")

        @speed.setter
        def speed(self, value):
            self._instrument.set("SPEED_SETPOINT", value)

    MODE = "serial"

    TIMEOUT = 2.5

    BAUD_RATE = 9600

    SET_REQUIRES_READBACK = False

    SMALL_DELAY = 0.05  # 1/20 of second
    """Small delay between subsequent commands (s)."""

    def __init__(
        self, resource="/dev/ttyACM0", mode="serial", timeout=2.5, baud=9600, sim=False
    ):
        """
        :param resource: Resource address. See VISA docs for more info.
        """
        super().__init__(resource)
        _ = sim
        self.MODE = mode
        self.TIMEOUT = timeout
        self.BAUD_RATE = baud

        self._connection_error = None

        self._com = Com(self.MODE, self.TIMEOUT)
        self._instrument = PbCom(self._com)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Connects to instrument using pyvisa, saves raw VISA handle as _instrument.
        Attempts to find parameters BAUD_RATE and TIMEOUT on the current class.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.

        :returns: Instrument object with activated context.
        """
        if not self.connected():
            try:
                self._com.open(self.resource, self.BAUD_RATE)
                self._connection_error = None
            except Exception as e:
                self._connection_error = e
                return None
        return super().__enter__(no_lock=True, recover_attempt=recover_attempt)

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection handle to instrument.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.
        """
        super().__exit__(
            exception_type,
            exception_value,
            traceback,
            no_lock=True,
            recover_attempt=recover_attempt,
        )
        if not self.connected():
            self._com.close()

    def validate_channel(self, channel, raise_exception=True):
        """Check if a power channel exists on this device. Only successful if `channel
        == 1`.

        :param channel: Channel number to validate as an input
        """
        if channel == 1:
            return True
        else:
            if raise_exception:
                raise ChannelError(
                    f"Channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    @acquire_lock()
    @retry_on_fail()
    def set(self, setting, value, check_readback_only=False):
        """Set `setting` on instrument to `value`, and read-back using equivalent
        `query()`.

        :param setting: key in class dictionary SETTINGS.
        :param value: target value for `setting`.
        :param check_readback_only: how to only check readback value if
            SET_REQUIRES_READBACK is set, and not perform back-query. If 'same', checks
            that readback gives same command as was sent.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: Read-back value if `query()` available, else whether number of bytes
                written during `set()` meets expectation
        """
        setting = setting.upper()
        if setting not in type(self).SETTINGS:
            raise ValidationError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call set()."
            )
        setting_ = type(self).SETTINGS[setting]
        if "SET" not in (setting_):
            raise RuntimeError(
                f"SCPI command for SET {setting} not found for device "
                f"{type(self).__name__} - is query-only? Cannot call set()."
            )
        if "verifier" in setting_:
            verifier = setting_["verifier"]

            try:
                value_ = verifier(value)
            except ValidationError as ve:
                logger.debug(f"ValidationError caught with message: {ve}")
                raise ValidationError(
                    f"Value {value} does not pass verifier "
                    f"{verifier.__name__} for set() on setting {setting} "
                    f"for device {type(self).__name__}.\n"
                    f"Allowable values: {verifier.allowable}"
                )
        else:
            value_ = value

        command = setting_["SET"]
        logger.debug(f'Setting "{command}"')
        time.sleep(self.SMALL_DELAY)

        if type(self).SET_REQUIRES_READBACK:
            logger.debug(
                "set() requires readback to empty buffer for this instrument class..."
            )
            rval = self._instrument.request_echo(command, value_)
            logger.debug(f"Write/read cycle returned: {rval}")
            if callable(type(self).SET_REQUIRES_READBACK):
                type(self).SET_REQUIRES_READBACK(self, command, value, rval)
            if check_readback_only == "same":
                return rval == command
        else:
            rval = self._instrument.send(command, value_)
            logger.debug(f"Write returned: {rval}")
        time.sleep(self.SMALL_DELAY)

        if "QUERY" in setting_:
            # If possible, read back
            ret = self.query(setting, no_lock=True, attempts=1)
            logger.debug(f'Read-back of "{command}" returned: {ret}')
            return ret
        else:
            return rval == len(command) + len(self._instrument.write_termination)

    @acquire_lock()
    @retry_on_fail()
    def query(self, setting):
        """Query `setting` on instrument.

        :param setting: key in class dictionary SETTINGS.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query`-call is nested within).
        :param attempts: how many retries to give `query` command.

        :return: Data returned by device for given query.
        """
        setting = setting.upper()
        if setting not in type(self).SETTINGS:
            raise RuntimeError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call query()."
            )
        setting_ = type(self).SETTINGS[setting]
        if "QUERY" not in setting_:
            raise RuntimeError(
                f"SCPI command for QUERY {setting} not found for device "
                f"{type(self).__name__} - is SET-only? Cannot call query()."
            )

        command = setting_["QUERY"]
        time.sleep(self.SMALL_DELAY)
        logger.debug(f'Querying "{command}"')
        ret = self._instrument.request_echo(command)
        logger.debug(f"Query returned: {ret}")
        if "parser" in setting_:
            ret = setting_["parser"](ret, setting_)
        return ret
