"""HMP4040 class for Rhode&Schwarz/Hameg HMP4040 4-output power supply."""

import time

from .instrument import Instrument, acquire_lock, ChannelError
from .scpi_instrument import (
    SCPIInstrument,
    is_in,
    is_numeric,
    truthy,
    verifier_or,
    is_integer,
)
from .utils.parser_utils import (
    numeric_bool,
    numeric_int,
    numeric_float,
)


@Instrument.register
class HMP4040(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Rhode&Schwarz/Hameg HMP4040 4-output power
    supply.

    Has some strange behaviours - take care to note difference between activating
    individual outputs and "turning on" all outputs.
    """

    TIMEOUT = 10000  # 10 seconds
    """Ethernet link timeout."""

    COM_RESET = "*RST; STATUS:PRESET; *CLS"
    """Instrument Reset SCPI command."""

    SOURCE_TYPES = {
        "DC": "DC Voltage (V)/Current (A)",
    }
    OUTPUTS = 4
    MEASURE_TYPES = {
        "VOLT:DC": "Output DC Voltage (V)",
        "CURR:DC": "Output DC Current (A)",
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
        "SYSTEM_MODE": {
            "SET": ":SYST:{}",
            "verifier": is_in("LOC", "MIX", "REM", "RWL"),
        },
        "STATUS_BYTE": {
            "QUERY": "*STB?",
            "parser": numeric_int,
        },
        "STATUS_BYTE_ENABLE": {
            "QUERY": "*SRE?",
            "parser": numeric_int,
        },
        "QUESTIONABLE_STATUS_REGISTER": {
            "QUERY": "STAT:QUES:EVEN?",
            "parser": numeric_int,
        },
        "QUESTIONABLE_STATUS_REGISTER_ENABLE": {
            "SET": "STAT:QUES:ENAB {}",
            "QUERY": "STAT:QUES:ENAB?",
            "verifier": is_integer(min=0, max=65535),
            "parser": numeric_int,
        },
        "INSTRUMENT_STATUS_REGISTER": {
            "QUERY": "STAT:QUES:INST:EVEN?",
            "parser": numeric_int,
        },
        "INSTRUMENT_STATUS_REGISTER_ENABLE": {
            "SET": "STAT:QUES:INST:ENAB {}",
            "QUERY": "STAT:QUES:INST:ENAB?",
            "verifier": is_integer(min=0, max=255),
            "parser": numeric_int,
        },
        "EVENT_STATUS_REGISTER": {
            "QUERY": "*ESR?",
            "parser": numeric_int,
        },
        "EVENT_STATUS_REGISTER_ENABLE": {
            "SET": "*ESE {}",
            "QUERY": "*ESE?",
            "verifier": is_integer(min=0, max=65535),
            "parser": numeric_int,
        },
        "CHANNEL_STATUS_REGISTER": {
            "QUERY": "STAT:QUES:INST:ISUM{}:EVEN?",
            "verifier": is_in(1, 2, 3, 4),
            "parser": numeric_int,
        },
        "CHANNEL_STATUS_REGISTER_ENABLE": {
            "SET": "STAT:QUES:INST:ISUM{}:ENAB {}",
            "QUERY": "STAT:QUES:INST:ISUM{}:ENAB?",
            "verifier": (is_in(1, 2, 3, 4), is_integer(min=0, max=65535)),
            "parser": numeric_int,
        },
        "INSTRUMENT": {
            "SET": "INST:NSEL {:d}",
            "QUERY": "INST:NSEL?",
            "verifier": is_integer(min=1, max=4),
            "parser": numeric_int,
        },
        "OUTPUT": {
            "SET": "OUTP:STAT {:d}",
            "QUERY": "OUTP:STAT?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OUTPUT_GENERAL": {
            "SET": "OUTP:GEN {:d}",
            "QUERY": "OUTP:GEN?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OUTPUT_SELECT": {
            "SET": "OUTP:SEL {:d}",
            "QUERY": "OUTP:SEL?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OVP": {
            "SET": "VOLT:PROT:LEV {:.2f}",
            "QUERY": "VOLT:PROT:LEV?",
            "verifier": verifier_or(is_numeric(min=0.1, max=32.5), is_in("MIN", "MAX")),
            "parser": numeric_float,
            "unit": "V",
        },
        "OVP_TRIPPED": {
            "QUERY": "VOLT:PROT:TRIP?",
            "parser": numeric_bool,
        },
        "OVP_CLEAR": {"SET": "VOLT:PROT:CLE"},
        "OUTPUT_CURRENT": {
            "QUERY": "MEAS:CURR:DC?",
            "parser": numeric_float,
            "unit": "A",
        },
        "OUTPUT_VOLTAGE": {
            "QUERY": "MEAS:VOLT:DC?",
            "parser": numeric_float,
            "unit": "V",
        },
        "VOLTAGE": {
            "SET": "SOUR:VOLT:LEV {:.2f}",
            "QUERY": "SOUR:VOLT:LEV?",
            "verifier": is_numeric(min=-32, max=32),
            "parser": numeric_float,
            "unit": "V",
        },
        "CURRENT": {
            "SET": "SOUR:CURR:LEV {:.2f}",
            "QUERY": "SOUR:CURR:LEV?",
            "verifier": is_numeric(min=0, max=10),
            "parser": numeric_float,
            "unit": "A",
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    MEASURE_COMMAND_MAP = {"VOLT:DC": "OUTPUT_VOLTAGE", "CURR:DC": "OUTPUT_CURRENT"}

    class PowerChannel(Instrument.PowerChannel):
        """PowerChannel implementation for the HMP4040."""

        @property
        def status(self):
            """
            :returns: Channel/Output Status Register value.
            :rtype: int
            """
            return self._instrument.query("CHANNEL_STATUS_REGISTER", self._channel)

        @property
        def state(self):
            return self._instrument.query_channel("OUTPUT", self._channel)

        @state.setter
        def state(self, value):
            self._instrument.set_channel("OUTPUT", self._channel, value)

        @property
        def voltage(self):
            return self._instrument.query_channel("VOLTAGE", self._channel)

        @voltage.setter
        def voltage(self, value):
            self._instrument.set_channel("VOLTAGE", self._channel, value)

        @property
        def current(self):
            return self._instrument.query_channel("CURRENT", self._channel)

        @current.setter
        def current(self, value):
            self._instrument.set_channel("CURRENT", self._channel, value)

        @property
        def voltage_limit(self):
            return self._instrument.query_channel("OVP", self._channel)

        @voltage_limit.setter
        def voltage_limit(self, value):
            # Cannot turn off for this instrument - set to max
            if value is False or value is None:
                value = "MAX"
            self._instrument.set_channel("OVP", self._channel, value)

        @property
        def voltage_trip(self):
            return self._instrument.query_channel("OVP_TRIPPED", self._channel)

        def reset_voltage_trip(self):
            return self._instrument.set_channel("OVP_CLEAR", self._channel)

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for the HMP4040."""

        @property
        def status(self):
            """
            :returns: Event Status Register value.
            :rtype: int
            """
            return self._instrument.query("EVENT_STATUS_REGISTER")

        @property
        def value(self):
            return self._instrument.query_channel(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type),
                self._channel,
            )

    def __init__(self, resource="ASRL2::INSTR", sim=False, outputs=4):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        :param outputs: How many outputs this HMPx0x0-style device has. Defaults to 4.
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

    @acquire_lock()
    def set_channel(self, setting, channel, *value):
        """Set `setting` on instrument to `value` for output `channel`, and read-back
        using equivalent `query()`.

        .. warning: Modifies currently selected `INSTRUMENT` on HMP4040 interface -
            this change persists.

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
        if not self.set("INSTRUMENT", channel, no_lock=True):
            raise ChannelError("Could not select correct output {channel}")
        return self.set(f"{setting}", *value, no_lock=True)

    @acquire_lock()
    def query_channel(self, setting, channel, **kwargs):
        """Query `setting` on instrument for output `channel`.

        .. warning: Modifies currently selected `INSTRUMENT` on HMP4040 interface -
            this change persists.

        :param setting: key in class dictionary SETTINGS.
        :param channel: output to set this setting on.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query_channel`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: data returned by device for given query.
        """
        self.validate_channel(int(channel))
        if not self.set("INSTRUMENT", channel, no_lock=True):
            raise ChannelError("Could not select correct output {channel}")
        return self.query(f"{setting}", no_lock=True)

    def off(self, channel, **kwargs):
        """Turn off output.

        :param channel: output to be turned off.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `off`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value.
        """
        # uses lock from sub-call
        return self.set_channel("OUTPUT", channel, 0, **kwargs)

    def on(self, channel, **kwargs):
        """Turn on output.

        :param channel: output to be turned on.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `on`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value.
        """
        # uses lock from sub-call
        return self.set_channel("OUTPUT", channel, 1, **kwargs)

    def status(self, channel, **kwargs):
        """Check status of output.

        :param channel: output to be turned on.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `status`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: status of output.
        """
        # uses lock from sub-call
        return int(self.query_channel("OUTPUT", channel, **kwargs))

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

    @acquire_lock()
    def monitor_step(self, channel):
        """Helper function for monitoring/logging especially for dirigent."""
        vset = self.query_channel("VOLTAGE", channel, no_lock=True)
        iset = self.query_channel("CURRENT", channel, no_lock=True)
        vout, iout = self.measure(channel, no_lock=True)
        output_status = float(self.status(channel, no_lock=True))
        return vout, iout, vset, iset, output_status

    @acquire_lock()
    def _power_cycle(self, channel, delay):
        """Callback function that can be used with `sweep()`.

        :param channel: output to be cycled.
        :param delay: delay during cycle (between off and on).

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `_power_cycle`-call is nested within).
        """
        self.off(channel, no_lock=True)
        time.sleep(delay)
        self.on(channel, no_lock=True)

    def sweep_print_header(self, measured_unit):
        """Returns the header for print-outs, when doing a sweep.

        Helper function of instrument.sweep() @param: measured_unit: Maybe required, as
        it is not necessarily the tti, that performs the measurement.
        """
        assert measured_unit in (
            "VOLT:DC",
            "VOLT:AC",
            "CURR:DC",
            "CURR:AC",
            "RES",
            "FRES",
            "SLDO",
            None,
        )
        if measured_unit == "VOLT:DC" or measured_unit == "SLDO":
            unit = "DC Voltage (V)"
        elif measured_unit == "VOLT:AC":
            unit = "AC Voltage (V)"
        elif measured_unit == "CURR:DC":
            unit = "DC Current (A)"
        elif measured_unit == "CURR:AC":
            unit = "AC Current (A)"
        elif measured_unit == "RES":
            unit = "Resistance (Ohm)"
        elif measured_unit == "FRES":
            unit = "Four-Wire Resistance (Ohm)"
        elif measured_unit is None:
            return ("Time", "Output Voltage (V)", "Output Current (A)")
        return ("Time", "Output Voltage (V)", "Output Current (A)", unit)

    def publish(self, client, line, channel="ALL", topic=None):
        """Publish data line (from Monitoring) to MQTT.

        TODO: don't reparse line here; instead pass/access raw data?

        :param client: MQTTClient object to publish with.
        :param line: MonitoringLogger line to publish.
        :param channel: which channel the data has been measured for. May be 'ALL'.
        """

        if line.startswith("#"):
            return
        topic = topic or self._resource[9:-7]
        values = line.split("\t")
        if channel == "ALL":
            for ch in range(1, self._outputs + 1):
                client.publish(
                    f"{topic}/channel{ch}/voltage",
                    f"{topic} voltage{ch}={float(values[2 * ch - 1].strip())}",
                )
                client.publish(
                    f"{topic}/channel{ch}/current",
                    f"{topic} current{ch}={float(values[2 * ch].strip())}",
                )
        else:
            self.validate_channel(int(channel))
            client.publish(
                f"{topic}/channel{channel}/voltage",
                f"{topic} voltage{channel}={float(values[1].strip())}",
            )
            client.publish(
                f"{topic}/channel{channel}/current",
                f"{topic} current{channel}={float(values[2].strip())}",
            )
