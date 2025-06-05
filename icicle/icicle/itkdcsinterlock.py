"""ITkDCSInterlock class for Rhode&Schwarz/Hameg HMP4040 4-output power supply."""

from .instrument import Instrument, acquire_lock, ChannelError
from .scpi_instrument import (
    SCPIInstrument,
    is_integer,
)
from .utils.parser_utils import (
    numeric_bool,
    numeric_float,
)


@Instrument.register
class ITkDCSInterlock(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for NI CompactRIO-based DCS interlock in OPMD.

    Communicates with interlock GUI - the latter provides (currently read-only)
    access to some registers and computed values within FPGA and GUI.
    """

    TIMEOUT = 10000  # 10 seconds
    """Ethernet link timeout."""

    COM_RESET = ""
    """Instrument Reset SCPI command."""

    OUTPUTS = 4
    MEASURE_TYPES = {
        "PT100:TEMP": "PT100 temperature (slot 1, degC)",
        "PT100:RES": "PT100 resistance (slot 1, ohm)",
        "PT100_2:TEMP": "PT100 temperature (slot 2, degC)",
        "PT100_2:RES": "PT100 resistance (slot 2, ohm)",
        "NTC:TEMP": "NTC temperature (degC)",
        "NTC:VOLT": "NTC output voltage (degC)",
        "SHT85:TEMP": "SHT85 temperature (degC)",
        "SHT85:HUMI": "SHT85 rel. humidity (%)",
        "LID:VOLT": "Lid sensor voltage (V)",
        "PRES:VOLT": "Pressure sensor voltage (V)",
        "VAC:VOLT": "Vacuum sensor voltage (V)",
        "VREF:VOLT": "Reference voltage (V)",
        "RELAY:STATUS": "Relay status",
        "RELAY:TRIP": "Relay tripped?",
        "RELAY:READY": "Relay ready?",
        "RELAY:ON": "Relay on?",
    }

    REGISTERS = {
        "STATUS_BYTE": "Status Byte",
        "QUESTIONABLE_STATUS_REGISTER": "Questionable Status Register",
        "INSTRUMENT_STATUS_REGISTER": "Channel Summary Status Register",
        "EVENT_STATUS_REGISTER": "Event Status Register",
    }

    SOURCE_REGISTERS = {"CHANNEL_STATUS_REGISTER": "Channel Status Register"}

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "PT100:TEMP": {
            "QUERY": "PT100:MEAS:TEMP? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "PT100:RES": {
            "QUERY": "PT100:MEAS:RES? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "PT100_2:TEMP": {
            "QUERY": "PT100_2:MEAS:TEMP? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "PT100_2:RES": {
            "QUERY": "PT100_2:MEAS:RES? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "NTC:TEMP": {
            "QUERY": "NTC:MEAS:TEMP? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "NTC:VOLT": {
            "QUERY": "NTC:MEAS:VOLT? {:d}",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "SHT85:TEMP": {
            "QUERY": "SHT85:MEAS:TEMP?",
            "parser": numeric_float,
            "verifier": is_integer(0, OUTPUTS),
        },
        "SHT85:HUMI": {
            "QUERY": "SHT85:MEAS:HUMI?",
            "parser": numeric_float,
        },
        "LID:VOLT": {
            "QUERY": "LID:MEAS:VOLT?",
            "parser": numeric_float,
        },
        "PRES:VOLT": {
            "QUERY": "PRES:MEAS:VOLT?",
            "parser": numeric_float,
        },
        "VAC:VOLT": {
            "QUERY": "VAC:MEAS:VOLT?",
            "parser": numeric_float,
        },
        "VREF:VOLT": {
            "QUERY": "VREF:MEAS:VOLT?",
            "parser": numeric_float,
        },
        "RELAY:STATUS": {
            "QUERY": "RELAY:STATUS? {:d}",
            "verifier": is_integer(0, OUTPUTS),
        },
        "RELAY:READY": {
            "QUERY": "RELAY:READY? {:d}",
            "parser": numeric_bool,
            "verifier": is_integer(0, OUTPUTS),
        },
        "RELAY:TRIP": {
            "QUERY": "RELAY:TRIP? {:d}",
            "parser": numeric_bool,
            "verifier": is_integer(0, OUTPUTS),
        },
        "RELAY:ON": {
            "QUERY": "RELAY:ON? {:d}",
            "parser": numeric_bool,
            "verifier": is_integer(0, OUTPUTS),
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for the ITkDCSInterlock."""

        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            if (
                "SHT85" in self._measure_type
                or "LID" in self._measure_type
                or "VAC" in self._measure_type
                or "VREF" in self._measure_type
            ):
                self._query = lambda q, _: self._instrument.query(q)
            else:
                self._query = lambda q, c: self._instrument.query_channel(q, c)

        @property
        def status(self):
            """
            :returns: Connection?
            :rtype: int
            """
            return bool(self._instrument.identify())

        @property
        def value(self):
            return self._query(
                self._measure_type,
                self._channel,
            )

    def __init__(self, resource="TCPIP::localhost::9898::SOCKET", sim=False, outputs=4):
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
        """Initialises connection to HMP4040.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `HMP4040` object in activated state.
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
        """Closes connection to HMP4040.

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
        return self.query_channel("RELAY:STATUS", channel, **kwargs)

    @acquire_lock()
    def measure(self, channel):
        """Measure output voltage, current.

        :param channel: output to be measured.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure`-call is nested within).

        :return: tuple `(voltage, current)`.
        """
        return (
            self.query_channel("OUTPUT_VOLTAGE", channel, no_lock=True),
            self.query_channel("OUTPUT_CURRENT", channel, no_lock=True),
        )
