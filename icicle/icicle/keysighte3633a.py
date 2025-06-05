"""Keysight class for Keysight E3633A low voltage power supplies."""

import time
import logging

from .instrument import acquire_lock, Instrument, ChannelError
from .scpi_instrument import SCPIInstrument, is_in, is_numeric, truthy, is_integer
from .utils.parser_utils import (
    numeric_bool,
    numeric_int,
    strip_float,
)

logger = logging.getLogger(__name__)


@Instrument.register
class KeysightE3633A(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Keysight E3633A or similar low voltage power
    supply."""

    BAUD_RATE = 9600
    """Serial link Baud rate."""

    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout (ms)."""

    COM_RESET = "*RST; *CLS"
    """Instrument Reset SCPI command."""

    SOURCE_TYPES = {
        "DC": "DC Voltage (V)/Current (A)",
    }
    OUTPUTS = 1
    MEASURE_TYPES = {
        "VOLT:DC": "Output DC Voltage (V)",
        "CURR:DC": "Output DC Current (A)",
    }

    REGISTERS = {
        "STATUS_BYTE": "Status Byte",
        "EVENT_STATUS_REGISTER": "Event Status Register",
        "QUESTIONABLE_STATUS_REGISTER": "Questionable Status Register",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "SYSTEM_MODE": {"SET": "SYST:{}", "verifier": is_in("LOC", "REM", "RWL")},
        "STATUS_BYTE": {"QUERY": "*STB?", "parser": numeric_int},
        "STATUS_BYTE_ENABLE": {
            "SET": "*SRE {}",
            "QUERY": "*SRE?",
            "verifier": is_integer(min=1, max=1024),
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
        "QUESTIONABLE_STATUS_REGISTER_ENABLE": {
            "SET": ":STAT:QUES:ENAB {}",
            "QUERY": ":STAT:QUES:ENAB?",
            "verifier": is_integer(min=1, max=65535),
            "parser": numeric_int,
        },
        "QUESTIONABLE_STATUS_REGISTER": {
            "QUERY": ":STAT:QUES?",
            "parser": numeric_int,
        },
        "OUTPUT": {
            "SET": "OUTP:STAT {}",
            "QUERY": "OUTP:STAT?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "VRANGE": {
            "SET": "SOUR:VOLT:RANG {}",
            "QUERY": "SOUR:VOLT:RANG?",
            "verifier": is_in("LOW", "HIGH"),
        },
        "OVP": {
            "SET": "SOUR:VOLT:PROT:LEV {}",
            "QUERY": "SOUR:VOLT:PROT:LEV?",
            "verifier": is_numeric(min=0, max=50),
            "parser": strip_float("V"),
        },
        "OVP_ENABLED": {
            "SET": "SOUR:VOLT:PROT:STAT {}",
            "QUERY": "SOUR:VOLT:PROT:STAT?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OVP_TRIPPED": {
            "QUERY": "SOUR:VOLT:PROT:TRIP?",
            "parser": numeric_bool,
        },
        "OVP_CLEAR": {
            "SET": "SOUR:VOLT:PROT:CLE",
        },
        "OCP": {
            "SET": "SOUR:CURR:PROT:LEV {}",
            "QUERY": "SOUR:CURR:PROT:LEV?",
            "verifier": is_numeric(min=0, max=10),
            "parser": strip_float("I"),
        },
        "OCP_ENABLED": {
            "SET": "SOUR:CURR:PROT:STAT {}",
            "QUERY": "SOUR:CURR:PROT:STAT?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OCP_TRIPPED": {
            "QUERY": "SOUR:CURR:PROT:TRIP?",
            "parser": numeric_bool,
        },
        "OCP_CLEAR": {
            "SET": "SOUR:CURR:PROT:CLE",
        },
        "VOLTAGE": {
            "SET": "SOUR:VOLT:LEV {}",
            "QUERY": "SOUR:VOLT:LEV?",
            "verifier": is_numeric(min=-50, max=50),
            "parser": strip_float("V"),
        },
        "OUTPUT_VOLTAGE": {
            "QUERY": "MEAS:VOLT:DC?",
            "parser": strip_float("V"),
        },
        "CURRENT": {
            "SET": "SOUR:CURR:LEV {}",
            "QUERY": "SOUR:CURR:LEV?",
            "verifier": is_numeric(min=0, max=20),
            "parser": strip_float("I"),
        },
        "OUTPUT_CURRENT": {
            "QUERY": "MEAS:CURR:DC?",
            "parser": strip_float("I"),
        },
        "VOLTAGE_STEP": {
            "SET": "SOUR:VOLT:LEV:IMM:STEP {}",
            "QUERY": "SOUR:VOLT:LEV:IMM:STEP?",
            "verifier": is_numeric(min=-50, max=50),
            "parser": strip_float("V"),
        },
        "CURRENT_STEP": {
            "SET": "SOUR:CURR:LEV:IMM:STEP {}",
            "QUERY": "SOUR:CURR:LEV:IMM:STEP?",
            "verifier": is_numeric(min=0, max=10),
            "parser": strip_float("I"),
        },
        "SEL": {
            "SET": "INST:SEL {}",
            "QUERY": "INST:SEL?",
            "verifier": is_in("OUTP1", "OUT1", "OUTP2", "OUT2"),
        },
        "NSEL": {
            "SET": "INST:NSEL {}",
            "QUERY": "INST:NSEL?",
            "verifier": is_integer(min=1),
            "parser": numeric_int,
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    MEASURE_COMMAND_MAP = {"VOLT:DC": "OUTPUT_VOLTAGE", "CURR:DC": "OUTPUT_CURRENT"}

    class PowerChannel(Instrument.PowerChannel):
        """PowerChannel implementation for Keysight E3633A."""

        @property
        def status(self):
            """
            :returns: Status Byte value.
            """
            return self._instrument.query("STATUS_BYTE")

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
            if self._instrument.query_channel("OVP_ENABLED", self._channel):
                return self._instrument.query_channel("OVP", self._channel)
            else:
                return None

        @voltage_limit.setter
        def voltage_limit(self, value):
            if (value is None) or (value is False):
                self._instrument.set_channel("OVP_ENABLED", self._channel, False)
            else:
                self._instrument.set_channel("OVP", self._channel, value)
                self._instrument.set_channel("OVP_ENABLED", self._channel, True)

        @property
        def current_limit(self):
            if self._instrument.query_channel("OCP_ENABLED", self._channel):
                return self._instrument.query_channel("OCP", self._channel)
            else:
                return None

        @current_limit.setter
        def current_limit(self, value):
            if (value is None) or (value is False):
                self._instrument.set_channel("OCP_ENABLED", self._channel, False)
            else:
                self._instrument.set_channel("OCP", self._channel, value)
                self._instrument.set_channel("OCP_ENABLED", self._channel, True)

        @property
        def voltage_trip(self):
            return self._instrument.query_channel("OVP_TRIPPED", self._channel)

        def reset_voltage_trip(self):
            return self._instrument.set_channel("OVP_CLEAR", self._channel)

        @property
        def current_trip(self):
            return self._instrument.query_channel("OCP_TRIPPED", self._channel)

        def reset_current_trip(self):
            return self._instrument.set_channel("OCP_CLEAR", self._channel)

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for Keysight E3633A."""

        @property
        def status(self):
            """
            :returns: Event Status Register value.
            """
            return self._instrument.query("EVENT_STATUS_REGISTER")

        @property
        def value(self):
            return self._instrument.query_channel(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type),
                self._channel,
            )

    def __init__(self, resource="ASRL4::INSTR", outputs=1, sim=False):
        """
        :param resource: VISA Resource address. See VISA docs for more info.
        """
        super().__init__(resource, sim=sim)
        self._outputs = outputs

    def validate_channel(self, channel, raise_exception=True):
        """Check if an output exists on this device.

        :param channel: Channel number to validate as an output.
        """
        if channel >= 1 and channel <= self._outputs:
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
    def __enter__(self, recover_attempt=False):
        """Initialises connection to Keysight.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `Keysight` object in activated state.
        """
        super().__enter__(recover_attempt=recover_attempt, no_lock=True)
        self.set("SYSTEM_MODE", "REM", no_lock=True)
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection to Keysight.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.
        """
        self.set("SYSTEM_MODE", "LOC", no_lock=True)
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    @acquire_lock()
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

        if self.validate_channel(channel) and self._outputs > 1:
            self.set("NSEL", channel, no_lock=True)
        # disable lock before passing to set
        kwargs["no_lock"] = True
        return self.set(f"{setting}", *value, **kwargs)

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
        if self.validate_channel(channel) and self._outputs > 1:
            self.set("NSEL", channel, no_lock=True)
        # disable lock before passing to set
        kwargs["no_lock"] = True
        return self.query(f"{setting}", **kwargs)

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

    @acquire_lock()
    def on(self, channel):
        """Turn on output. For some reason needs an explicit check/retry loop since this
        sometimes fails?

        :param channel: output to be turned on.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `on`-call is nested within).

        :return: read-back value.
        """
        ret = self.set_channel("OUTPUT", channel, 1, no_lock=True)
        time.sleep(0.1)
        count = 0
        while self.status(channel, no_lock=True) == 0 and count < 3:
            logger.error("Tried to turn LV on but is still off! Retrying...")
            ret = self.set_channel("OUTPUT", channel, 1, no_lock=True)
            count += 1
        return ret

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
        vset = float(self.query_channel("VOLTAGE", channel, no_lock=True))
        iset = float(self.query_channel("CURRENT", channel, no_lock=True))
        vout, iout = self.measure(channel, no_lock=True)
        output_status = float(self.status(channel, no_lock=True))
        return vout, iout, vset, iset, output_status

    @acquire_lock()
    def _power_cycle(self, channel, delay):
        """Callback function that can be used with `sweep()`.

        :param channel: output to be cycled.
        :param delay: delay during cycle (between off and on).

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set`-call is nested within).
        """
        self.off(channel, no_lock=True)
        time.sleep(delay)
        self.on(channel, no_lock=True)

    def sweep(
        self,
        target_setting,
        channel,
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
        log_function=None,
        power_cycle_each_step=False,
        **kwargs,
    ):
        """Sweep `target_setting` on a given `channel`.

        :param target_setting: name of setting that `set()` call should target.
        :param channel: output to perform sweep on.
        :param target_value: value of setting to sweep to (from current).
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param n_steps: number of steps to perform (alternative to step_size - specify
            one but not both)
        :param set_function: function to use for `set()` call. Defaults to `self.set`
        :param set_args: additional keyword arguments to pass to `set()`
        :param measure: whether to measure at each step of sweep.
        :param measure_function: function to use for `measure()` call. Defaults to
            `self.measure`
        :param measure_args: additional keyword arguments to pass to `measure()`
        :param query_args: additional keyword arguments to pass to `query()` at each
            step.
        :param power_cycle_each_step: set callback function to `_power_cycle` with
            correct arguments.
        :param execute_each_step: accepts a callback function (no arguments) to be
            executed at each step of the sweep.

        :return: final parameter value[, measure data]
        """

        # uses lock from sub-call
        self.validate_channel(int(channel))
        execute_each_step = None
        if set_function is None:
            set_function = type(self).set_channel
        if set_args is None:
            set_args = {"channel": int(channel)}
        else:
            set_args["channel"] = int(channel)
        if query_function is None:
            query_function = type(self).query_channel
        if query_args is None:
            query_args = {"channel": int(channel)}
        else:
            query_args["channel"] = int(channel)
        execute_each_step = kwargs.pop("execute_each_step", None)
        return super().sweep(
            f"{target_setting}",
            target_value,
            delay=delay,
            step_size=step_size,
            n_steps=n_steps,
            measure=measure,
            set_function=set_function,
            set_args=set_args,
            query_function=query_function,
            query_args=query_args,
            measure_function=measure_function,
            measure_args=measure_args,
            conversion=None,
            log_function=log_function,
            execute_each_step=(
                lambda: (
                    self._power_cycle(channel, delay, no_lock=True)
                    if power_cycle_each_step
                    else execute_each_step
                )
            ),
            **kwargs,
        )

    def sweep_print_header(self, measured_unit=None):
        """Returns the header for print-outs, when doing a sweep.

        Helper function of instrument.sweep() @param: measured_unit: Type of measurement
        that is performed. If 'None', the measurement is only output Voltage and output
        Current.
        """
        # assert measured_unit in ("VOLT:DC", "CURR:DC", "SLDO")
        if measured_unit == "VOLT:DC" or "SLDO":
            return ("Output Voltage (V)",)
        elif measured_unit == "CURR:DC":
            return ("Output Current (A)",)
        elif measured_unit is None:
            return ("Time", "Output Voltage (V)", "Output Current (A)")

    def publish(self, client, line, channel="ALL"):
        """Publish data line (from Monitoring) to MQTT.

        TODO: don't reparse line here; instead pass/access raw data?

        :param client: MQTTClient object to publish with.
        :param line: MonitoringLogger line to publish.
        :param channel: which channel the data has been measured for. May be 'ALL'.
        """

        if line.startswith("#"):
            return
        instrument_name = self._resource[9:-7]
        values = line.split("\t")
        if channel == "ALL":
            for ch in self.outputs:
                client.publish(
                    f"/{instrument_name}/channel{ch}/voltage",
                    f"{instrument_name} voltage{ch}={float(values[2*ch-1].strip())}",
                )
                client.publish(
                    f"/{instrument_name}/channel{ch}/current",
                    f"{instrument_name} current{ch}={float(values[2*ch].strip())}",
                )
        else:
            # self.validate_channel(int(channel))
            client.publish(
                f"/{instrument_name}/channel{channel}/voltage",
                f"{instrument_name} voltage{channel}={float(values[1].strip())}",
            )
            client.publish(
                f"/{instrument_name}/channel{channel}/current",
                f"{instrument_name} current{channel}={float(values[2].strip())}",
            )
