"""RelayBoard class for ETHZ Relayboard (Vasilije Perovic design)."""

import time
import logging

from .instrument import acquire_lock, Instrument, ChannelError
from .visa_instrument import VisaInstrument, retry_on_fail_visa

logger = logging.getLogger(__name__)


class RelayBoard(VisaInstrument, key="resource"):
    """VisaInstrument implementation for ETHZ Relayboard (Vasilije Perovic design).

    Additional (fake) set(), query() commands have been added to make this a psuedo-
    implementation of SCPIInstrument (and match behaviour of said interface).
    """

    BAUD_RATE = 9600
    """Serial link Baud rate."""
    TIMEOUT = 5000  # 5 seconds
    """Serial link timeout (ms)."""

    READ_TERMINATION = ""
    WRITE_TERMINATION = ""

    PIN_MAP = {
        "DOUBLE": {
            "VDDD_ROC1": "h",
            "VDDA_ROC1": "g",
            "VDDA_ROC0": "f",
            "VDDD_ROC0": "e",
            "VMUX_B": "d",
            "IMUX_B": "c",
            "VMUX_A": "b",
            "IMUX_A": "a",
            "GND_A": "f",
            "OFF": "x",
        },
        "QUAD": {
            "VDDD_ROC2": "a",
            "VDDA_ROC2": "b",
            "VDDD_ROC3": "c",
            "VDDA_ROC3": "d",
            "VDDD_ROC0": "e",
            "VDDA_ROC0": "f",
            "VDDA_ROC1": "g",
            "VDDD_ROC1": "h",
            "GND": "i",
            "VIN": "j",
            "OFF": "x",
        },
        "RD53A": {
            "VDDD_ROC2": "a",
            "VDDA_ROC2": "b",
            "VDDD_ROC3": "c",
            "VDDA_ROC3": "d",
            "VDDD_ROC0": "e",
            "VDDA_ROC0": "f",
            "VDDA_ROC1": "g",
            "VDDD_ROC1": "h",
            "GND": "i",
            "NTC": "j",
            "OFF": "x",
        },
    }
    """Pin map for Arduino pin names.

    Must match arduino firmware version.
    """

    MEASURE_WRAPPER_TYPES = {
        # CROC/RD53A pin variants
        "VDDD_ROC0": "ROC0 digital SLDO",
        "VDDD_ROC1": "ROC1 digital SLDO",
        "VDDD_ROC2": "ROC2 digital SLDO",
        "VDDD_ROC3": "ROC3 digital SLDO",
        "VDDA_ROC0": "ROC0 analog SLDO",
        "VDDA_ROC1": "ROC1 analog SLDO",
        "VDDA_ROC2": "ROC2 analog SLDO",
        "VDDA_ROC3": "ROC3 analog SLDO",
        "GND": "GND reference",
        "NTC": "HDI NTC",
        # OSU pin variants
        "VDDA_A": "Analog SLDO A",
        "VDDA_B": "Analog SLDO B",
        "VDDD_A": "Digital SLDO A",
        "VDDD_B": "Digital SLDO B",
        "VMUX_A": "Analog VMUX A",
        "VMUX_B": "Analog VMUX B",
        "IMUX_A": "Analog IMUX A",
        "IMUX_B": "Analog IMUX B",
        "GND_A": "GND reference",
    }

    class MeasureChannelWrapper(Instrument.MeasureChannelWrapper):
        """MeasureChannel implementation for RelayBoard.

        Wraps a
        :class:`Instrument.MeasureChannel` on an appropriate measurement device,
        e.g. :class:`Keithley2000`.
        """

        def __init__(self, wrapped_channel, channel, wrapper_type, relay_board):
            """Constructor.

            :param wrapped_channel: :class:`MeasureChannel` to be wrapped.
            :param channel: channel number. Unused; only for interface consistency.
            :param wrapper_type: What should be wrapped by; e.g. ``VDDD_ROC0``.
            :param relay_board: :class:`RelayBoard` object to be wrapped.
            """
            super().__init__(wrapped_channel, channel, wrapped_channel.unit)
            self._relay_board = relay_board
            self._wrapper_type = wrapper_type

        def __enter__(self):
            """Opens context on both wrapped channel and on :class:`RelayBoard`"""
            if not self._relay_board.connected():
                self._exit_instrument = True
                self._relay_board = self._relay_board.__enter__()
            super().__enter__()
            return self

        def __exit__(self, exception_type, exception_value, traceback):
            """Closes context on both wrapped channel and on :class:`RelayBoard`"""
            super().__exit__(exception_value, exception_value, traceback)
            if self._relay_board.connected() and self._exit_instrument:
                self._relay_board.__exit__(exception_type, exception_value, traceback)
            self._exit_instrument = False

        @property
        def value(self):
            self._relay_board.set_pin(self._wrapper_type)
            return self.instrument.value

        @property
        def status(self):
            """Passes through status from wrapped channel."""
            return self.instrument.status

    def __init__(self, resource="ASRL20::INSTR", pin_map="QUAD", sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        :param pin_map: Chooses the map to access different pins according to the
            version of the ROC (options: QUAD, DOUBLE)
        """
        super().__init__(resource, sim=sim)
        pin_map = "QUAD" if pin_map is None else pin_map.upper()
        if pin_map not in type(self).PIN_MAP.keys():
            raise ChannelError(
                f"Undefined pin map for instrument of type RelayBoard: {pin_map}"
            )
        self._PIN_MAP = type(self).PIN_MAP[pin_map]
        if type(self) is RelayBoard:
            logger.warning(
                " DeprecationWarning: directly instantiating RelayBoard with pin_map="
                f"{pin_map} is deprecated behaviour"
            )
            logger.warning(
                "    Please use the RelayBoardCROC, RelayBoardRD53A, and RelayBoardOSU "
                "shell classes instead (which are registered with the Instrument "
                "registry)"
            )

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to relay board.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `RelayBoard` object in activated state.
        """
        super().__enter__(self, no_lock=True)
        if self._sim:
            self._instrument.read_termination = "\n"
            self._instrument.write_termination = "\n"
        time.sleep(0.1)  # TODO: remove this if unnecessary!
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection to relay board.

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
                raise RuntimeError(
                    f"Channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    @acquire_lock()
    @retry_on_fail_visa()
    def set_pin(self, pin):
        """Set currently connected pin on relay board to `pin`.

        :param pin: pin name in `PIN_MAP`.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set_pin`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result returned by relay board (usually pin number that has been
            connected/powered).
        """
        if pin in self._PIN_MAP:
            logger.debug("Setting pin {0}".format(self._PIN_MAP[pin]))
            return self._instrument.query(self._PIN_MAP[pin])
        else:
            raise RuntimeError(
                f"Invalid pin name: {pin}. "
                f'Valid pin names: {",".join(self._PIN_MAP.keys())}'
            )

    def status(self, **kwargs):
        """Queries currently connected pin, returns raw result.

        .. warning: Generally one should use query_pin instead since this returns the
            raw numeric result, not the converted pin name.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `status`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result returned by relay board (usually pin number that has been
            connected/powered).
        """
        return self.query_pin(**kwargs)

    def set(self, setting, value, **kwargs):
        """ "Fake" entry point to extend common interface as much as possible.

        Set currently connected pin on relay board to `pin`. See set_pin().

        :param setting: must be `PIN`.
        :param value: pin name in `PIN_MAP`.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result returned by relay board (usually pin number that has been
            connected/powered).
        """
        # uses lock from sub-command - pass kwargs
        if setting != "PIN":
            raise RuntimeError(
                f"Setting {setting} not found for device {type(self).name}, or "
                "query-only. Cannot call set()."
            )
        else:
            return self.set_pin(value, **kwargs)

    @acquire_lock()
    @retry_on_fail_visa()
    def query_pin(self):
        """Query currently connected pin, return pin name as in `PIN_MAP`.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query_pin`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: pin name as in `PIN_MAP`.
        """
        response = self._instrument.query("?")

        return next(
            key for key, value in self._PIN_MAP.items() if value == response.strip()
        )

    def query(self, setting, **kwargs):
        """ "Fake" entry point to extend common interface as much as possible.

        Query currently connected pin. See query_pin().

        :param setting: must be `PIN`.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: pin name as in `PIN_MAP`.
        """
        # "Fake" entry point to extend common interface as much as possible
        # uses lock from sub-command - pass kwargs
        if setting != "PIN":
            raise RuntimeError(
                f"Setting {setting} not found for device {type(self).name}, or "
                "set-only. Cannot call query()."
            )
        else:
            return self.query_pin(**kwargs)

    def off(self, **kwargs):
        """Set connected pin to `OFF`.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `off`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result returned by relay board (usually pin number that has been
            connected/powered).
        """
        # uses lock from sub-command - pass kwargs
        return self.set_pin("OFF", **kwargs)


# ================= Concrete (shell) implementations =================
# The following implementations allow for class instantiation without
# requiring custom arguments, used by e.g. measurechannel_cli


@Instrument.register
class RelayBoardRD53A(RelayBoard, key="resource"):
    # RD53A pin variants
    MEASURE_WRAPPER_TYPES = {
        "VDDD_ROC0": "ROC0 digital SLDO",
        "VDDD_ROC1": "ROC1 digital SLDO",
        "VDDD_ROC2": "ROC2 digital SLDO",
        "VDDD_ROC3": "ROC3 digital SLDO",
        "VDDA_ROC0": "ROC0 analog SLDO",
        "VDDA_ROC1": "ROC1 analog SLDO",
        "VDDA_ROC2": "ROC2 analog SLDO",
        "VDDA_ROC3": "ROC3 analog SLDO",
        "GND": "GND reference",
        "NTC": "HDI NTC",
    }

    def __init__(self, resource="ASRL20::INSTR", sim=False):
        super().__init__(resource=resource, sim=sim, pin_map="RD53A")


@Instrument.register
class RelayBoardQUAD(RelayBoard, key="resource"):
    # CROC pin variants
    MEASURE_WRAPPER_TYPES = {
        "VDDD_ROC0": "ROC0 digital SLDO",
        "VDDD_ROC1": "ROC1 digital SLDO",
        "VDDD_ROC2": "ROC2 digital SLDO",
        "VDDD_ROC3": "ROC3 digital SLDO",
        "VDDA_ROC0": "ROC0 analog SLDO",
        "VDDA_ROC1": "ROC1 analog SLDO",
        "VDDA_ROC2": "ROC2 analog SLDO",
        "VDDA_ROC3": "ROC3 analog SLDO",
        "GND": "GND reference",
        "NTC": "HDI NTC",
    }

    def __init__(self, resource="ASRL20::INSTR", sim=False):
        super().__init__(resource=resource, sim=sim, pin_map="QUAD")


@Instrument.register
class RelayBoardDOUBLE(RelayBoard, key="resource"):
    # OSU pin variants
    MEASURE_WRAPPER_TYPES = {
        "VDDA_A": "Analog SLDO A",
        "VDDA_B": "Analog SLDO B",
        "VDDD_A": "Digital SLDO A",
        "VDDD_B": "Digital SLDO B",
        "VMUX_A": "Analog VMUX A",
        "VMUX_B": "Analog VMUX B",
        "IMUX_A": "Analog IMUX A",
        "IMUX_B": "Analog IMUX B",
        "GND_A": "GND reference",
    }

    def __init__(self, resource="ASRL20::INSTR", sim=False):
        super().__init__(resource=resource, sim=sim, pin_map="QUAD")
