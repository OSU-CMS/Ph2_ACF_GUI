"""TTI class for TTI MX100TP-style low voltage power supplies."""

import time
import logging

from .instrument import acquire_lock, Instrument, ChannelError
from .scpi_instrument import (
    SCPIInstrument,
    is_in,
    is_integer,
    is_numeric,
    truthy,
    verifier_or,
    map_to,
)
from .utils.parser_utils import (
    bitfield_bool,
    numeric_bool,
    numeric_int,
    strip_float,
    float_or_strings,
    int_map,
)

logger = logging.getLogger(__name__)


@Instrument.register
class TTI(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for TTI MX180TP or similar low voltage power
    supply."""

    BAUD_RATE = 9600

    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout (ms)."""

    COM_RESET = "*RST; STATUS:PRESET; *CLS"
    """Instrument Reset SCPI command."""

    SOURCE_TYPES = {
        "DC": "DC Voltage (V)/Current (A)",
    }
    OUTPUTS = 3
    MEASURE_TYPES = {
        "VOLT:DC": "Output DC Voltage (V)",
        "CURR:DC": "Output DC Current (A)",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "SYSTEM_MODE": {
            "SET": "{}",
            "QUERY": "IFLOCK?",
            "verifier": map_to(
                {
                    "LOC": "IFLOCK 0;LOCAL",
                    "REM": "IFLOCK 0;",  # Any command puts TTI PSU into remote mode
                    "RWL": "IFLOCK 1",
                }
            ),
            "parser": int_map(
                {
                    0: "REM",
                    1: "RWL",
                    -1: "LOCK_UNAVAILABLE",
                }
            ),
        },
        "STATUS_BYTE": {
            "QUERY": "*STB?",
            "parser": numeric_int,
        },
        "CHANNEL_STATUS_REGISTER": {
            "QUERY": "LSR{}?",
            "verifier": is_integer(min=1, max=3),
            "parser": numeric_int,
        },
        "EVENT_STATUS_REGISTER": {
            "QUERY": "*ESR?",
            "parser": numeric_int,
        },
        "OUTPUT1": {
            "SET": "OP1 {}",
            "QUERY": "OP1?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OUTPUT2": {
            "SET": "OP2 {}",
            "QUERY": "OP2?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OUTPUT3": {
            "SET": "OP3 {}",
            "QUERY": "OP3?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "OUTPUTALL": {
            "SET": "OPALL {}",
            "verifier": truthy(true_output=1, false_output=0),
        },
        "VRANGE1": {
            "SET": "VRANGE1 {}",
            "QUERY": "VRANGE1?",
            "verifier": is_in(1, 2, 3, 4, 5, 6, 7, "1", "2", "3", "4", "5", "6", "7"),
            # Output1: 1 = 30V/6A, 2 = 15V/10A, 3 = 60V/3A, 4 = 30V/12A, 5 = 15V/20A,
            # 6 = 60V/6A, 7 = 120V/3A.
            "parser": numeric_int,
        },
        "VRANGE2": {
            "SET": "VRANGE2 {}",
            "QUERY": "VRANGE2?",
            "verifier": is_in(1, 2, 3, "1", "2", "3"),
            # Output2: 1 = 30V/6A, 2 = 15V/10A, 3 = 60V/3A.
            "parser": numeric_int,
        },
        "VRANGE3": {
            "SET": "VRANGE3 {}",
            "QUERY": "VRANGE3?",
            "verifier": is_in(1, 2, "1", "2"),
            # Output3: 1 = 5.5V/3A, 2 = 12V/1.5A.
            "parser": numeric_int,
        },
        "OVP1": {
            "SET": "OVP1 {}",
            "QUERY": "OVP1?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("VP1",)),
            "unit": "V",
        },
        "OVP2": {
            "SET": "OVP2 {}",
            "QUERY": "OVP2?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("VP2",)),
            "unit": "V",
        },
        "OVP3": {
            "SET": "OVP3 {}",
            "QUERY": "OVP3?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("VP3",)),
            "unit": "V",
        },
        "OCP1": {
            "SET": "OCP1 {}",
            "QUERY": "OCP1?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("CP1",)),
            "unit": "A",
        },
        "OCP2": {
            "SET": "OCP2 {}",
            "QUERY": "OCP2?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("CP2",)),
            "unit": "A",
        },
        "OCP3": {
            "SET": "OCP3 {}",
            "QUERY": "OCP3?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("CP3",)),
            "unit": "A",
        },
        "TRIP_CLEAR1": {"QUERY": "LSR1?"},
        "TRIP_CLEAR2": {"QUERY": "LSR2?"},
        "TRIP_CLEAR3": {"QUERY": "LSR3?"},
        "OVP_TRIPPED1": {"QUERY": "LSR1?", "parser": bitfield_bool(2)},
        "OVP_TRIPPED2": {"QUERY": "LSR2?", "parser": bitfield_bool(2)},
        "OVP_TRIPPED3": {"QUERY": "LSR3?", "parser": bitfield_bool(2)},
        "OCP_TRIPPED1": {"QUERY": "LSR1?", "parser": bitfield_bool(3)},
        "OCP_TRIPPED2": {"QUERY": "LSR2?", "parser": bitfield_bool(3)},
        "OCP_TRIPPED3": {"QUERY": "LSR3?", "parser": bitfield_bool(3)},
        "LOCAL": {"SET": "LOCAL"},
        "DAMPING1": {
            "SET": "DAMPING1 {}",
            "verifier": is_in("ON", "OFF", "LOW", "MED", "HIGH"),
        },
        "DAMPING2": {
            "SET": "DAMPING2 {}",
            "verifier": is_in("ON", "OFF", "LOW", "MED", "HIGH"),
        },
        "DAMPING3": {
            "SET": "DAMPING3 {}",
            "verifier": is_in("ON", "OFF", "LOW", "MED", "HIGH"),
        },
        "VOLTAGE1": {
            "SET": "V1 {}",
            "QUERY": "V1?",
            "verifier": is_numeric(min=-120, max=120),
            "parser": strip_float("V1"),
            "unit": "V",
        },
        "VOLTAGE2": {
            "SET": "V2 {}",
            "QUERY": "V2?",
            "verifier": is_numeric(min=-60, max=60),
            "parser": strip_float("V2"),
            "unit": "V",
        },
        "VOLTAGE3": {
            "SET": "V3 {}",
            "QUERY": "V3?",
            "verifier": is_numeric(min=-12, max=12),
            "parser": strip_float("V3"),
            "unit": "V",
        },
        "OUTPUT_VOLTAGE1": {"QUERY": "V1O?", "parser": strip_float("V"), "unit": "V"},
        "OUTPUT_VOLTAGE2": {"QUERY": "V2O?", "parser": strip_float("V"), "unit": "V"},
        "OUTPUT_VOLTAGE3": {"QUERY": "V3O?", "parser": strip_float("V"), "unit": "V"},
        "CURRENT1": {
            "SET": "I1 {}",
            "QUERY": "I1?",
            "verifier": is_numeric(min=0, max=20),
            "parser": strip_float("I1"),
            "unit": "A",
        },
        "CURRENT2": {
            "SET": "I2 {}",
            "QUERY": "I2?",
            "verifier": is_numeric(min=0, max=10),
            "parser": strip_float("I2"),
            "unit": "A",
        },
        "CURRENT3": {
            "SET": "I3 {}",
            "QUERY": "I3?",
            "verifier": is_numeric(min=0, max=3),
            "parser": strip_float("I3"),
            "unit": "A",
        },
        "OUTPUT_CURRENT1": {"QUERY": "I1O?", "parser": strip_float("A"), "unit": "A"},
        "OUTPUT_CURRENT2": {"QUERY": "I2O?", "parser": strip_float("A"), "unit": "A"},
        "OUTPUT_CURRENT3": {"QUERY": "I3O?", "parser": strip_float("A"), "unit": "A"},
        "VOLTAGE_STEP1": {
            "SET": "DELTAV1 {}",
            "QUERY": "DELTAV1?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAV1"),
            "unit": "V",
        },
        "VOLTAGE_STEP2": {
            "SET": "DELTAV2 {}",
            "QUERY": "DELTAV2?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAV2"),
            "unit": "V",
        },
        "VOLTAGE_STEP3": {
            "SET": "DELTAV3 {}",
            "QUERY": "DELTAV3?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAV3"),
            "unit": "V",
        },
        "CURRENT_STEP1": {
            "SET": "DELTAI1 {}",
            "QUERY": "DELTAI1?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAI1"),
            "unit": "A",
        },
        "CURRENT_STEP2": {
            "SET": "DELTAI2 {}",
            "QUERY": "DELTAI2?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAI2"),
            "unit": "A",
        },
        "CURRENT_STEP3": {
            "SET": "DELTAI3 {}",
            "QUERY": "DELTAI3?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAI3"),
            "unit": "A",
        },
        "INCREMENT_VOLTAGE1": {
            "SET": "INCV1",
        },
        "INCREMENT_VOLTAGE2": {
            "SET": "INCV1",
        },
        "INCREMENT_VOLTAGE3": {
            "SET": "INCV1",
        },
        "DECREMENT_VOLTAGE1": {
            "SET": "DECV1",
        },
        "DECREMENT_VOLTAGE2": {
            "SET": "DECV2",
        },
        "DECREMENT_VOLTAGE3": {
            "SET": "DECV3",
        },
        "INCREMENT_CURRENT1": {
            "SET": "INCI1",
        },
        "INCREMENT_CURRENT2": {
            "SET": "INCI1",
        },
        "INCREMENT_CURRENT3": {
            "SET": "INCI1",
        },
        "DECREMENT_CURRENT1": {
            "SET": "DECI1",
        },
        "DECREMENT_CURRENT2": {
            "SET": "DECI2",
        },
        "DECREMENT_CURRENT3": {
            "SET": "DECI3",
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    MEASURE_COMMAND_MAP = {"VOLT:DC": "OUTPUT_VOLTAGE", "CURR:DC": "OUTPUT_CURRENT"}

    class PowerChannel(Instrument.PowerChannel):
        """PowerChannel implementation for TTI."""

        @property
        def status(self):
            """
            :returns: Channel Status Register value.
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
            ret = self._instrument.query_channel("OVP", self._channel)
            return None if ret == "OFF" else ret

        @voltage_limit.setter
        def voltage_limit(self, value):
            if (value is None) or (value is False):
                value = "OFF"
            self._instrument.set_channel("OVP", self._channel, value)

        @property
        def current_limit(self):
            ret = self._instrument.query_channel("OCP", self._channel)
            return None if ret == "OFF" else ret

        @current_limit.setter
        def current_limit(self, value):
            if (value is None) or (value is False):
                value = "OFF"
            self._instrument.set_channel("OCP", self._channel, value)

        @property
        def voltage_trip(self):
            return self._instrument.query_channel("OVP_TRIPPED", self._channel)

        def reset_voltage_trip(self):
            return self._instrument.query_channel("TRIP_CLEAR", self._channel)

        @property
        def current_trip(self):
            return self._instrument.query_channel("OCP_TRIPPED", self._channel)

        def reset_current_trip(self):
            return self._instrument.query_channel("TRIP_CLEAR", self._channel)

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for TTI."""

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

    def __init__(self, resource="ASRL3::INSTR", outputs=3, sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        :param outputs: How many outputs this TTI power supply has.
        """
        super().__init__(resource, sim=sim)
        self._outputs = outputs

    @property
    def outputs(self):
        """
        :returns: Iterator over all outputs (i.e. numbers 1, 2, ...)
        """
        return range(1, self._outputs + 1)

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
        # uses lock from sub-call
        self.validate_channel(int(channel), raise_exception=True)
        return self.set(f"{setting}{channel}", *value, **kwargs)

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
        # uses lock from sub-call
        self.validate_channel(int(channel), raise_exception=True)
        return self.query(f"{setting}{channel}", **kwargs)

    def off(self, channel, **kwargs):
        """Turn off output.

        :param channel: output to be turned off.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `off`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value.
        """
        # uses lock from sub-call
        if channel != "ALL":
            self.validate_channel(int(channel), raise_exception=True)
        self.validate_channel(int(channel), raise_exception=True)
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
        if channel == "ALL":
            ret = self.set_channel("OUTPUT", channel, 1, no_lock=True)
            time.sleep(0.1)
            count = 0
            while (
                0 in [self.status(ch, no_lock=True) for ch in self.outputs]
                and count < 3
            ):
                for ch in self.outputs:
                    if self.status(ch, no_lock=True) == 0:
                        logger.error(
                            f"Tried to turn LV CHANNEL {ch} on but is still off! "
                            "Retrying..."
                        )
                ret = self.set_channel("OUTPUT", channel, 1, no_lock=True)
                count += 1
        else:
            self.validate_channel(int(channel), raise_exception=True)
            ret = self.set_channel("OUTPUT", int(channel), 1, no_lock=True)
            time.sleep(0.1)
            count = 0
            while self.status(int(channel), no_lock=True) == 0 and count < 3:
                logger.error(
                    f"Tried to turn LV CHANNEL {channel} on but is still off! "
                    "Retrying..."
                )
                ret = self.set_channel("OUTPUT", int(channel), 1, no_lock=True)
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
        self.validate_channel(channel, raise_exception=True)
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
        self.validate_channel(channel, raise_exception=True)
        return (
            self.query_channel("OUTPUT_VOLTAGE", channel, no_lock=True),
            self.query_channel("OUTPUT_CURRENT", channel, no_lock=True),
        )

    @acquire_lock()
    def monitor_step(self, channel):
        """Helper function for monitoring/logging especially for dirigent."""
        self.validate_channel(channel, raise_exception=True)
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
            that `_power_cycle`-call is nested within).
        """
        self.validate_channel(channel, raise_exception=True)
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
        measure_function=None,
        measure_args=None,
        query_args=None,
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
        execute_each_step = kwargs.pop("execute_each_step", None)
        return super().sweep(
            f"{target_setting}{channel}",
            target_value,
            delay=delay,
            step_size=step_size,
            n_steps=n_steps,
            measure=measure,
            set_function=set_function,
            set_args=set_args,
            measure_function=measure_function,
            measure_args=measure_args,
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

    def sweep_print_header(self, measured_unit):
        """Returns the header for print-outs, when doing a sweep.

        Helper function of instrument.sweep()

        :param measured_unit: Maybe required, as it is not necessarily the tti, that
            performs the measurement.
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
            for ch in self.outputs:
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
