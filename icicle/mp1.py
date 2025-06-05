"""MP1 class for Gulmay MP1 xray controllers."""

import logging

from .instrument import acquire_lock, Instrument, ChannelError
from .visa_instrument import constants
from .scpi_instrument import SCPIInstrument, is_in, is_integer, is_numeric, truthy

logger = logging.getLogger(__name__)


def mp1_set_requires_readback(inst, command, value, readback_value):
    """Callable to perfom extra readbacks if required for MP1 failed command calls.

    :param inst: MP1 instance.
    :param command: raw command that was sent corresponding to this set().
    :param value: value that had been injected into set() command.
    :param readback_value: value received from initial readback.
    """
    logger.debug(
        f"Readback performed: response is {readback_value}; NULL would be ({command})"
    )
    if readback_value == f"({command[0:2]})":
        logger.debug("NULL response caught")
        for i in range(len(command)):
            # For MP1 need to read back each sent byte on failure, +2 for termination
            # (for some reason)
            logger.debug(f"Extra readback {i}: {inst._instrument.read()}")


@Instrument.register
class MP1(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Gulmay MP1 xray controller."""

    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout (ms)."""

    READ_TERMINATION = "\r"
    """Read termination characters."""
    WRITE_TERMINATION = "\r"
    """Write termination characters."""

    BAUD_RATE = 9600
    """Serial Baud rate."""
    DATA_BITS = 8
    """Number of data bits contained in each frame (from 5 to 8)."""
    STOP_BITS = constants.StopBits.one
    """Number of stop bits to indicate end of frame."""
    PARITY = constants.Parity.none
    """Parity used with every frame transmitted and received."""

    # SCPIInstrument-level configuration
    SET_REQUIRES_READBACK = mp1_set_requires_readback
    """Set function to gobble returned bits after failed set()."""

    COM_RESET = ""
    """ Instrument Reset command - does not exist for MP1! """

    SOURCE_TYPES = {
        "DC": "DC Voltage (V)/Current (A)",
    }
    OUTPUTS = 1
    MEASURE_TYPES = {
        "VOLT:DC": "Output DC Voltage (V)",
        "CURR:DC": "Output DC Current (A)",
    }

    SETTINGS = {
        "IDENTIFIER": {
            "QUERY": "?P926",
            "parser": lambda r, s: r.replace(f'{s["QUERY"]},', ""),
        },
        "OUTPUT": {
            "SET": "!{}",
            "QUERY": "?M",
            "verifier": truthy(true_output="X", false_output="O"),
            "response_map": {
                "000": False,
                "001": False,
                "002": False,
                "003": True,
                "004": True,
            },
            "parser": lambda r, s: s["response_map"][r.replace(s["QUERY"], "")],
        },
        "VOLTAGE": {
            "SET": "!V{:03d}",
            "QUERY": "?V",
            "verifier": is_integer(min=0, max=999),
            "parser": lambda r, s: (
                int(r.replace(s["QUERY"], "")) if r != "(0)" else False
            ),
        },
        "CURRENT": {
            "SET": "!I{:03d}",
            "QUERY": "?I",
            "verifier": is_numeric(min=0, max=99, scale=10, to_int=True),
            "parser": lambda r, s: (
                int(r.replace(s["QUERY"], "")) / 10.0 if r != "(0)" else False
            ),
        },
        "TIMER": {
            "QUERY": "?T",
            # Parser returns in minutes
            "parser": lambda r, s: float(r.replace(s["QUERY"], "")) / 10.0,
        },
        "RESET_TIMER": {"SET": "!T"},
        "PROGRAMME": {
            "QUERY": "?P",
            "parser": lambda r, s: int(r.replace(s["QUERY"], "")),
        },
        "MODE": {
            "QUERY": "?M",
            "response_map": {
                "000": "KEY_IN_POS_2",
                "001": "XRAY_OFF",
                "002": "PREWARMING",
                "003": "XRAY_RAMPING",
                "004": "XRAY_ON",
            },
            "parser": lambda r, s: s["response_map"][r.replace(s["QUERY"], "")],
        },
        "MODE_CODE": {
            "QUERY": "?M",
            "parser": lambda r, s: int(r.replace(s["QUERY"], "")),
        },
        "ERRORS": {
            "QUERY": "?E",
            "parser": lambda r, s: tuple(
                int(a) for a in r.replace(s["QUERY"], "").split(",")
            ),
        },
        "FOCUS": {
            "SET": "!{}",
            "QUERY": "?F",
            "verifier": is_in("F", "B"),
            "parser": lambda r, s: r.replace(s["QUERY"], ""),
        },
        "PROGRAMME_REVISION": {
            "QUERY": "?R",
            "parser": lambda r, s: int(r.replace(s["QUERY"], "")),
        },
        "WARMUP": {
            "QUERY": "?U",
            "parser": lambda r, s: int(r.replace(s["QUERY"], "")),
        },
    }
    """Settings dictionary with all Set/Query combinations."""

    MEASURE_COMMAND_MAP = {"VOLT:DC": "VOLTAGE", "CURR:DC": "CURRENT"}

    class PowerChannel(Instrument.PowerChannel):
        """PowerChannel implementation for Keithley2410."""

        @property
        def status(self):
            """
               - 000 : KEY_IN_POS_2
               - 001 : XRAY_OFF
               - 002 : PREWARMING
               - 003 : XRAY_RAMPING
               - 004 : XRAY_ON

            :returns: MODE code.
            :rtype: int.
            """
            return self._instrument.query("MODE_CODE")

        @property
        def errors(self):
            """
            :returns: List of errors.
            """
            return self._instrument.query("ERRORS")

        @property
        def state(self):
            return self._instrument.query("OUTPUT")

        @state.setter
        def state(self, value):
            self._instrument.set("OUTPUT", value, check_readback_only="same")

        @property
        def voltage(self):
            # from kV to V
            return self._instrument.query("VOLTAGE") * 1000.0

        @voltage.setter
        def voltage(self, value):
            # from V to kV
            self._instrument.set("VOLTAGE", value) / 1000.0

        @property
        def current(self):
            return self._instrument.query("CURRENT")

        @current.setter
        def current(self, value):
            self._instrument.set("CURRENT", value)

        @property
        def voltage_trip(self):
            errors = self._instrument.query("ERRORS")
            return 11 in errors or 14 in errors

        @property
        def current_trip(self):
            errors = self._instrument.query("ERRORS")
            return 12 in errors or 13 in errors

        @property
        def interlocked(self):
            """
            :returns: Whether xray box is in interlock state.
            """
            errors = self._instrument.query("ERRORS")
            interlocked = False
            for error in errors:
                if error in (54, 55, 56, 57, 58, 59, 60, 61, 62, 65, 66):
                    interlocked = True
            return interlocked

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for Keithley2410."""

        @property
        def status(self):
            """
               - 000 : KEY_IN_POS_2
               - 001 : XRAY_OFF
               - 002 : PREWARMING
               - 003 : XRAY_RAMPING
               - 004 : XRAY_ON

            :returns: MODE code.
            :rtype: int.
            """
            return self._instrument.query("MODE_CODE")

        @property
        def errors(self):
            """
            :returns: List of errors.
            """
            return self._instrument.query("ERRORS")

        @property
        def value(self):
            return self._instrument.query(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type),
                self._channel,
            )

    def __init__(self, resource="ASRL10::INSTR", sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        :param outputs: How many outputs this TTI power supply has.
        """
        super().__init__(resource, sim=sim)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to TTI.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `TTI` object in activated state.
        """
        super().__enter__(recover_attempt=recover_attempt, no_lock=True)
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection to TTI.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.
        """
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    def validate_channel(self, channel, raise_exception=True):
        """Check if an channel exists on this device. Only successful if `channel == 1`.

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

    def reset(self):
        """Disable since this device has no reset command."""
        raise NotImplementedError("MP1 does not accept a RESET command!")

    def off(self, **kwargs):
        """Turn off xrays.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `off`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value.
        """
        # uses lock from sub-call
        return self.set("OUTPUT", False, check_readback_only="same", **kwargs)

    def on(self, **kwargs):
        """Turn on xrays.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `on`-call is nested within).

        :return: read-back value.
        """
        # uses lock from sub-call
        return self.set("OUTPUT", True, check_readback_only="same", **kwargs)

    def status(self, channel, **kwargs):
        """Check status of output.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `status`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: status of output.
        """
        # uses lock from sub-call
        return self.query("OUTPUT", **kwargs)
