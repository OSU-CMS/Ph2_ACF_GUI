"""PIDController class for TRICICLE PIDcontroller-UI instance."""

from .instrument import Instrument, acquire_lock, ChannelError
from .scpi_instrument import (
    SCPIInstrument,
    is_integer,
    is_numeric,
    truthy,
)
from .utils.parser_utils import (
    numeric_bool,
    numeric_float,
)


@Instrument.register
class PIDController(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for NI CompactRIO-based DCS interlock in OPMD.

    Communicates with interlock GUI - the latter provides (currently read-only)
    access to some registers and computed values within FPGA and GUI.
    """

    TIMEOUT = 10000  # 10 seconds
    """Ethernet link timeout."""

    COM_RESET = ""
    """Instrument Reset SCPI command."""

    CHANNELS = 99
    MEASURE_TYPES = {
        "TEMPERATURE_SETPOINT": "Setpoint",
        "TEMPERATURE": "Input value",
        "CONTROL": "Controlled parameter",
        "SPEED": "Speed parameter",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "TEMPERATURE_SETPOINT": {
            "QUERY": "SETP? {:d}",
            "SET": "SETP {:d} {:f}",
            "parser": numeric_float,
            "verifier": (is_integer(0, CHANNELS), is_numeric()),
        },
        "STATE": {
            "QUERY": "STAT? {:d}",
            "SET": "STAT {:d} {:d}",
            "parser": numeric_bool,
            "verifier": (
                is_integer(0, CHANNELS),
                truthy(true_output=1, false_output=0),
            ),
        },
        "CONTROL": {
            "QUERY": "CONT? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, CHANNELS),
        },
        "TEMPERATURE": {
            "QUERY": "VAL? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, CHANNELS),
        },
        "LOW_LIMIT": {
            "QUERY": "OUTP:LIM:LOW {:d}",
            "SET": "OUTP:LIM:LOW {:d} {:f}",
            "parser": numeric_float,
            "verifier": (is_integer(0, CHANNELS), is_numeric()),
        },
        "HIGH_LIMIT": {
            "QUERY": "OUTP:LIM:HIGH {:d}",
            "SET": "OUTP:LIM:HIGH {:d} {:f}",
            "parser": numeric_float,
            "verifier": (is_integer(0, CHANNELS), is_numeric()),
        },
        "KP": {
            "QUERY": "PARA:KP? {:d}",
            "SET": "PARA:KP {:d} {:f}",
            "parser": numeric_float,
            "verifier": (is_integer(0, CHANNELS), is_numeric()),
        },
        "KI": {
            "QUERY": "PARA:KI? {:d}",
            "SET": "PARA:KI {:d} {:f}",
            "parser": numeric_float,
            "verifier": (is_integer(0, CHANNELS), is_numeric()),
        },
        "KD": {
            "QUERY": "PARA:KD? {:d}",
            "SET": "PARA:KD {:d} {:f}",
            "parser": numeric_float,
            "verifier": (is_integer(0, CHANNELS), is_numeric()),
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for the ITkDCSInterlock."""

        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)

        @property
        def status(self):
            """
            :returns: Connection?
            :rtype: int
            """
            return bool(self._instrument.identify())

        @property
        def value(self):
            if self._measure_type == "SPEED":
                return self._instrument.query_channel("KP", self._channel)
            return self._instrument.query_channel(
                self._measure_type,
                self._channel,
            )

    class TemperatureChannel(Instrument.TemperatureChannel):
        """TemperatureChannel implementation for PIDController."""

        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self._speed = None

        @property
        def status(self):
            """
            :returns: Dummy status.
            """
            return 0

        @property
        def state(self):
            return self._instrument.query_channel("STATE", self.channel)

        @state.setter
        def state(self, value):
            self._instrument.set_channel("STATE", self.channel, value)

        @property
        def temperature(self):
            return self._instrument.query_channel("TEMPERATURE", self.channel)

        @temperature.setter
        def temperature(self, value):
            self._instrument.set_channel("TEMPERATURE_SETPOINT", self.channel, value)

        @property
        def speed(self):
            return self._speed

        @speed.setter
        def speed(self, value):
            if self._speed is None:
                self._Kp = self._instrument.query_channel("KP", self.channel)
                self._Kd = self._instrument.query_channel("KI", self.channel)
                self._Ki = self._instrument.query_channel("KD", self.channel)
            self._speed = value
            self._instrument.set_channel("KP", self.channel, value * self._Kp)
            self._instrument.set_channel("KI", self.channel, value * self._Ki)
            self._instrument.set_channel("KD", self.channel, value * self._Kd)

    def __init__(
        self, resource="TCPIP::localhost::19898::SOCKET", sim=False, outputs=4
    ):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        :param outputs: How many relays this interlock device has. Defaults to 4.
        """
        super().__init__(resource, sim=sim)
        self._outputs = outputs

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to TRICICLE PIDcontroller-UI.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `PIDController` object in activated state.
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
        """Closes connection to TRICICLE PIDcontroller-UI.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.
        """
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    def validate_channel(self, channel, raise_exception=True):
        """Check if an output exists on this device.

        :param channel: channel number to validate as an output.
        """
        if channel >= 1 and channel <= self._outputs:
            return True
        else:
            if raise_exception:
                raise ChannelError(
                    f"channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    def set_channel(self, setting, channel, *value, **kwargs):
        """Set `setting` on instrument to `value` for output `channel`, and read-back
        using equivalent `query()`.

        :param setting: key in class dictionary SETTINGS.
        :param channel: channel to set this setting on.
        :param value: target value for `setting`.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set_channel`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value if `query()` available, else whether number of bytes
            written during `set()` meets expectation
        """
        self.validate_channel(int(channel))
        # uses lock from sub-call
        return self.set(f"{setting}", channel - 1, *value, **kwargs)

    def query_channel(self, setting, channel, **kwargs):
        """Query `setting` on instrument for output `channel`.

        :param setting: key in class dictionary SETTINGS.
        :param channel: output to set this setting on.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query_channel`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: data returned by device for given query.
        """
        self.validate_channel(int(channel))
        # uses lock from sub-call
        return self.query(f"{setting}", channel - 1, **kwargs)

    def status(self, channel, **kwargs):
        """Check status of relay.

        :param channel: relay to be checked.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `status`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: status of output.
        """
        # uses lock from sub-call
        return self.query_channel("STATE", channel, **kwargs)
