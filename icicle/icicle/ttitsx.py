"""TTI class for TTI TSX1820P-style low voltage power supplies."""

import time
import logging

from .instrument import acquire_lock, Instrument, ChannelError
from .scpi_instrument import (
    SCPIInstrument,
    is_in,
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
class TTITSX(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for TTI TSX1820P or similar low voltage power
    supply."""

    BAUD_RATE = 9600

    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout (ms)."""

    COM_RESET = "*RST; STATUS:PRESET; *CLS"
    """Instrument Reset SCPI command."""

    SOURCE_TYPES = {
        "DC": "DC Voltage (V)/Current (A)",
    }
    OUTPUTS = 1
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
            "QUERY": "LSR1?",
            "parser": numeric_int,
        },
        "EVENT_STATUS_REGISTER": {
            "QUERY": "*ESR?",
            "parser": numeric_int,
        },
        "OUTPUT": {
            "SET": "OP1 {}",
            "QUERY": "OP1?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "VRANGE": {
            "SET": "VRANGE1 {}",
            "QUERY": "VRANGE1?",
            "verifier": is_in(1, 2, 3, 4, 5, 6, 7, "1", "2", "3", "4", "5", "6", "7"),
            # Output1: 1 = 30V/6A, 2 = 15V/10A, 3 = 60V/3A, 4 = 30V/12A, 5 = 15V/20A,
            # 6 = 60V/6A, 7 = 120V/3A.
            "parser": numeric_int,
        },
        "OVP": {
            "SET": "OVP1 {}",
            "QUERY": "OVP1?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("VP1",)),
            "unit": "V",
        },
        "OCP": {
            "SET": "OCP1 {}",
            "QUERY": "OCP1?",
            "verifier": verifier_or(is_numeric(), is_in("ON", "OFF")),
            "parser": float_or_strings("OFF", replace=("CP1",)),
            "unit": "A",
        },
        "TRIP_CLEAR": {"QUERY": "LSR1?"},
        "OVP_TRIPPED": {"QUERY": "LSR1?", "parser": bitfield_bool(2)},
        "OCP_TRIPPED": {"QUERY": "LSR1?", "parser": bitfield_bool(3)},
        "LOCAL": {"SET": "LOCAL"},
        "DAMPING": {
            "SET": "DAMPING1 {}",
            "verifier": is_in("ON", "OFF", "LOW", "MED", "HIGH"),
        },
        "VOLTAGE": {
            "SET": "V {}",
            "QUERY": "V?",
            "verifier": is_numeric(min=-120, max=120),
            "parser": strip_float("V"),
            "unit": "V",
        },
        "OUTPUT_VOLTAGE": {"QUERY": "VO?", "parser": strip_float("V"), "unit": "V"},
        "CURRENT": {
            "SET": "I {}",
            "QUERY": "I?",
            "verifier": is_numeric(min=0, max=20),
            "parser": strip_float("I"),
            "unit": "A",
        },
        "OUTPUT_CURRENT": {"QUERY": "IO?", "parser": strip_float("A"), "unit": "A"},
        "VOLTAGE_STEP": {
            "SET": "DELTAV1 {}",
            "QUERY": "DELTAV1?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAV1"),
            "unit": "V",
        },
        "CURRENT_STEP": {
            "SET": "DELTAI1 {}",
            "QUERY": "DELTAI1?",
            "verifier": is_numeric(),
            "parser": strip_float("DELTAI1"),
            "unit": "A",
        },
        "INCREMENT_VOLTAGE": {
            "SET": "INCV1",
        },
        "DECREMENT_VOLTAGE": {
            "SET": "DECV1",
        },
        "INCREMENT_CURRENT": {
            "SET": "INCI1",
        },
        "DECREMENT_CURRENT": {
            "SET": "DECI1",
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
            return self._instrument.query("CHANNEL_STATUS_REGISTER")

        @property
        def state(self):
            return self._instrument.query("OUTPUT")

        @state.setter
        def state(self, value):
            self._instrument.set("OUTPUT", value)

        @property
        def voltage(self):
            return self._instrument.query("VOLTAGE")

        @voltage.setter
        def voltage(self, value):
            self._instrument.set("VOLTAGE", value)

        @property
        def current(self):
            return self._instrument.query("CURRENT")

        @current.setter
        def current(self, value):
            self._instrument.set("CURRENT", value)

        @property
        def voltage_limit(self):
            ret = self._instrument.query("OVP")
            return None if ret == "OFF" else ret

        @voltage_limit.setter
        def voltage_limit(self, value):
            if (value is None) or (value is False):
                value = "OFF"
            self._instrument.set("OVP", value)

        @property
        def current_limit(self):
            ret = self._instrument.query("OCP")
            return None if ret == "OFF" else ret

        @current_limit.setter
        def current_limit(self, value):
            if (value is None) or (value is False):
                value = "OFF"
            self._instrument.set("OCP", value)

        @property
        def voltage_trip(self):
            return self._instrument.query("OVP_TRIPPED")

        def reset_voltage_trip(self):
            return self._instrument.query("TRIP_CLEAR")

        @property
        def current_trip(self):
            return self._instrument.query("OCP_TRIPPED")

        def reset_current_trip(self):
            return self._instrument.query("TRIP_CLEAR")

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
            return self._instrument.query(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type),
            )

    def __init__(self, resource="ASRL7::INSTR", outputs=1, sim=False):
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

    def off(self, **kwargs):
        """Turn off output.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `off`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value.
        """
        # uses lock from sub-call
        return self.set("OUTPUT", 0, **kwargs)

    @acquire_lock()
    def on(self):
        """Turn on output. For some reason needs an explicit check/retry loop since this
        sometimes fails?

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `on`-call is nested within).

        :return: read-back value.
        """
        ret = self.set("OUTPUT", 1, no_lock=True)
        time.sleep(0.1)
        count = 0
        while self.status(no_lock=True) == 0 and count < 3:
            logger.error("Tried to turn LV on but is still off! " "Retrying...")
            ret = self.set("OUTPUT", 1, no_lock=True)
            count += 1
        return ret

    def status(self, **kwargs):
        """Check status of output.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `status`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: status of output.
        """
        # uses lock from sub-call
        return int(self.query("OUTPUT", **kwargs))

    @acquire_lock()
    def measure(self):
        """Measure output voltage, current.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure`-call is nested within).

        :return: tuple `(voltage, current)`.
        """
        return (
            self.query("OUTPUT_VOLTAGE", no_lock=True),
            self.query("OUTPUT_CURRENT", no_lock=True),
        )

    @acquire_lock()
    def monitor_step(self):
        """Helper function for monitoring/logging especially for dirigent."""
        vset = float(self.query("VOLTAGE", no_lock=True))
        iset = float(self.query("CURRENT", no_lock=True))
        vout, iout = self.measure(no_lock=True)
        output_status = float(self.status(no_lock=True))
        return vout, iout, vset, iset, output_status

    @acquire_lock()
    def _power_cycle(self, delay):
        """Callback function that can be used with `sweep()`.

        :param delay: delay during cycle (between off and on).

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `_power_cycle`-call is nested within).
        """
        self.off(no_lock=True)
        time.sleep(delay)
        self.on(no_lock=True)

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
        measure_function=None,
        measure_args=None,
        query_args=None,
        log_function=None,
        power_cycle_each_step=False,
        **kwargs,
    ):
        """Sweep `target_setting`.

        :param target_setting: name of setting that `set()` call should target.
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
            measure_function=measure_function,
            measure_args=measure_args,
            log_function=log_function,
            execute_each_step=(
                lambda: (
                    self._power_cycle(1, delay, no_lock=True)
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

    def publish(self, client, line, topic=None):
        """Publish data line (from Monitoring) to MQTT.

        TODO: don't reparse line here; instead pass/access raw data?

        :param client: MQTTClient object to publish with.
        :param line: MonitoringLogger line to publish.
        """

        if line.startswith("#"):
            return
        topic = topic or self._resource[9:-7]
        values = line.split("\t")
        client.publish(
            f"{topic}/voltage",
            f"{topic} voltage={float(values[1].strip())}",
        )
        client.publish(
            f"{topic}/current",
            f"{topic} current={float(values[2].strip())}",
        )
