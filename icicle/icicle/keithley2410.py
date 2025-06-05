"""Keithley2410 class for Keithley 2410 source-measure unit."""

from .instrument import acquire_lock, Instrument, ChannelError
from .visa_instrument import retry_on_fail_visa
from .scpi_instrument import (
    SCPIInstrument,
    is_in,
    is_integer,
    is_numeric,
    verifier_or,
    truthy,
    map_to,
)
from .utils.parser_utils import (
    strip_str,
    numeric_int,
    numeric_bool,
    numeric_float,
    truthy_bool,
    int_map,
    float_list,
    str_map,
)


@Instrument.register
class Keithley2410(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Keithley2410 source-measure unit."""

    BAUD_RATE = 9600
    """Serial link Baud rate."""
    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout (ms)."""

    COM_SELFTEST = "*TST?"
    """Selftest SCPI command."""
    COM_RESET = "*RST; STATUS:PRESET; *CLS"
    """Instrument Reset SCPI command."""
    COM_CLEAR_TRACE = ":TRAC:CLE"
    """Clear trace SCPI command."""
    COM_DEVICE_CLEAR = "DCL"
    """Device Clear SCPI command."""

    SOURCE_TYPES = {
        "VOLT:DC": "DC Voltage (V)",
        "CURR:DC": "DC Current (A)",
    }
    OUTPUTS = 1
    MEASURE_TYPES = {
        "VOLT:DC": "DC Voltage (V)",
        "CURR:DC": "DC Current (A)",
        "RES": "Resistance (Ohm)",
    }
    """Measurement types for this instrument."""

    REGISTERS = {
        "STATUS_BYTE": "Status Byte",
        "EVENT_STATUS_REGISTER": "Event Status Register",
        "QUESTIONABLE_STATUS_REGISTER": "Questionable Status Register",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "SYSTEM_MODE": {
            # RS232 only
            "SET": ":SYST:{}",
            "QUERY": ":SYST:RWL?",
            "verifier": map_to({"LOC": "LOC", "REM": "RWL 0", "RWL": "RWL 1"}),
            "parser": int_map(
                {
                    0: "REM",
                    1: "RWL",
                }
            ),
        },
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
        "QUESTIONABLE_DATA_REGISTER_ENABLE": {
            "SET": ":STAT:QUES:ENAB {}",
            "QUERY": ":STAT:QUES:ENAB?",
            "verifier": is_integer(min=1, max=65535),
            "parser": numeric_int,
        },
        "QUESTIONABLE_DATA_REGISTER": {
            "QUERY": ":STAT:QUES:EVEN?",
            "parser": numeric_int,
        },
        "MEASUREMENT_STATUS_REGISTER_ENABLE": {
            "SET": ":STAT:MEAS:ENAB {}",
            "QUERY": ":STAT:MEAS:ENAB?",
            "verifier": is_integer(min=1, max=65535),
            "parser": numeric_int,
        },
        "MEASUREMENT_STATUS_REGISTER": {
            "QUERY": ":STAT:MEAS:EVEN?",
            "parser": numeric_int,
        },
        "OUTPUT": {
            "SET": ":OUTP {}",
            "QUERY": ":OUTP?",
            "verifier": truthy(true_output=1, false_output=0),
        },
        "PANEL": {
            "SET": ":ROUT:TERM {}",
            "QUERY": ":ROUT:TERM?",
            "verifier": is_in("FRON", "REAR"),
        },
        "SOURCE_FUNCTION": {
            "SET": ":SOUR:FUNC {}",
            "QUERY": ":SOUR:FUNC?",
            "verifier": map_to({"VOLT:DC": "VOLT", "CURR:DC": "CURR"}),
            "parser": str_map({"VOLT": "VOLT:DC", "CURR": "CURR:DC"}),
        },
        "VOLTAGE_RANGE": {
            "SET": ":SOUR:VOLT:RANG {}",
            "QUERY": ":SOUR:VOLT:RANG?",
            "verifier": is_numeric(min=-1100, max=1100),
            "parser": numeric_float,
        },
        "VOLTAGE_RANGE_AUTO": {
            "SET": ":SOUR:VOLT:RANG:AUTO {}",
            "QUERY": ":SOUR:VOLT:RANG:AUTO?",
            "verifier": truthy(true_output=1, false_output=0),
        },
        "VOLTAGE": {
            "SET": ":SOUR:VOLT:LEV {}",
            "QUERY": ":SOUR:VOLT:LEV?",
            "verifier": is_numeric(min=-1100, max=1100),
            "parser": numeric_float,
        },
        "SENSE_VOLTAGE_RANGE": {
            "SET": ":SENS:VOLT:RANG {}",
            "QUERY": ":SENS:VOLT:RANG?",
            "verifier": is_numeric(-1100, 1100),
            "parser": numeric_float,
        },
        "SENSE_VOLTAGE_RANGE_AUTO": {
            "SET": ":SENS:VOLT:RANG:AUTO {}",
            "QUERY": ":SENS:VOLT:RANG:AUTO?",
            "verifier": truthy(true_output=1, false_output=0),
        },
        "CURRENT_RANGE": {
            "SET": ":SOUR:CURR:RANG {}",
            "QUERY": ":SOUR:CURR:RANG?",
            # Takes max from 2430 Pulse Mode
            "verifier": is_numeric(min=-10.5, max=10.5),
            "parser": numeric_float,
        },
        "CURRENT_RANGE_AUTO": {
            "SET": ":SOUR:CURR:RANG:AUTO {}",
            "QUERY": ":SOUR:CURR:RANG:AUTO?",
            "verifier": truthy(true_output=1, false_output=0),
        },
        "SENSE_CURRENT_RANGE": {
            "SET": ":SENS:CURR:RANG {}",
            "QUERY": ":SENS:CURR:RANG?",
            # Takes max from 2430 Pulse Mode
            "verifier": is_numeric(min=-10.5, max=10.5),
            "parser": numeric_float,
        },
        "SENSE_CURRENT_RANGE_AUTO": {
            "SET": ":SENS:CURR:RANG:AUTO {}",
            "QUERY": ":SENS:CURR:RANG:AUTO?",
            "verifier": truthy(true_output=1, false_output=0),
        },
        "CURRENT": {
            "SET": ":SOUR:CURR:LEV {}",
            "QUERY": ":SOUR:CURR:LEV?",
            # Takes max from 2430 Pulse Mode
            "verifier": is_numeric(min=-10.5, max=10.5),
            "parser": numeric_float,
        },
        "DELAY": {
            "SET": ":SOUR:DEL {}",
            "QUERY": ":SOUR:DEL?",
            "verifier": is_numeric(min=0, max=9999.999),
        },
        "AUTODELAY": {
            "SET": ":SOUR:DEL:AUTO {}",
            "QUERY": ":SOUR:DEL:AUTO?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "SWEEP_ENABLE": {
            "SET": ":SOUR:{}:MODE {}",
            "QUERY": ":SOUR:{}:MODE?",
            "verifier": (
                is_in(*SOURCE_TYPES.keys()),
                truthy(
                    false_output="FIX",
                    true_output="SWE",
                    false_like=("FIX", "FIXED", 0, "0", "OFF", "N", "NO", "FALSE"),
                    true_like=("SWE", "SWEEP", 1, "1", "ON", "Y", "YES", "TRUE"),
                ),
            ),
            "parser": truthy_bool(
                false_like=("FIX", "FIXED", "LIST"),
                true_like=("SWE", "SWEEP"),
            ),
        },
        "SWEEP_START": {
            "SET": ":SOUR:{}:START {}",
            "QUERY": ":SOUR:{}:START?",
            "verifier": (
                is_in(*SOURCE_TYPES.keys()),
                is_numeric(min=-1100, max=1100),
            ),
            "parser": numeric_float,
        },
        "SWEEP_END": {
            "SET": ":SOUR:{}:END {}",
            "QUERY": ":SOUR:{}:END?",
            "verifier": (
                is_in(*SOURCE_TYPES.keys()),
                is_numeric(min=-1100, max=1100),
            ),
            "parser": numeric_float,
        },
        "SWEEP_STEP": {
            "SET": ":SOUR:{}:STEP {}",
            "QUERY": ":SOUR:{}:STEP?",
            "verifier": (
                is_in(*SOURCE_TYPES.keys()),
                is_numeric(min=-1100, max=1100),
            ),
            "parser": numeric_float,
        },
        "SWEEP_DIRECTION": {
            "SET": ":SOUR:SWE:DIR {}",
            "QUERY": ":SOUR:SWE:DIR?",
            "verifier": is_in("UP", "DOWN"),
        },
        "SWEEP_RANGE": {
            "SET": ":SOUR:SWE:RANG {}",
            "QUERY": ":SOUR:SWE:RANG?",
            "verifier": is_in("FIX", "AUTO", "BEST"),
        },
        "SWEEP_POINTS": {
            "SET": ":SOUR:SWE:POINT {}",
            "QUERY": ":SOUR:SWE:POIN?",
            "verifier": is_integer(min=1, max=2500),
            "parser": numeric_int,
        },
        "SWEEP_SPACE": {
            "SET": ":SOUR:SWE:SPAC {}",
            "QUERY": ":SOUR:SWE:SPAC?",
            "verifier": is_in("LIN", "LOG"),
        },
        "OCP": {
            "SET": ":SENS:CURR:PROT {}",
            "QUERY": ":SENS:CURR:PROT?",
            "verifier": verifier_or(is_numeric(min=-10.5, max=10.5), is_in("MAX")),
            "parser": numeric_float,
        },
        "OCP_TRIPPED": {
            "QUERY": ":SENS:CURR:PROT:TRIP?",
            "parser": numeric_bool,
        },
        "OVP": {
            "SET": ":SENS:VOLT:PROT {}",
            "QUERY": ":SENS:VOLT:PROT?",
            "verifier": verifier_or(is_numeric(min=-1100, max=1100), is_in("MAX")),
            "parser": numeric_float,
        },
        "OVP_TRIPPED": {
            "QUERY": ":SENS:VOLT:PROT:TRIP?",
            "parser": numeric_bool,
        },
        "SENSE_FUNCTION": {
            "SET": ':SENS:FUNC "{}"',
            "QUERY": ":SENS:FUNC?",
            "verifier": is_in(*MEASURE_TYPES.keys()),
            "parser": strip_str('"'),
        },
        "LINE_INTEGRATION_CYCLES": {
            "SET": ":SENS:{}:NPLC {:.2f}",
            "QUERY": ":SENS:{}:NPLC?",
            "verifier": (
                is_in(*MEASURE_TYPES.keys()),
                is_numeric(min=0.01, max=10),
            ),
            "parser": numeric_float,
        },
        "ENABLE_AVERAGING": {
            "SET": ":SENS:AVER {}",
            "QUERY": ":SENS:AVER?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": numeric_bool,
        },
        "AVERAGING_COUNT": {
            "SET": ":SENS:AVER:COUN {}",
            "QUERY": ":SENS:AVER:COUN?",
            "verifier": is_integer(min=1, max=100),
            "parser": numeric_int,
        },
        "AVERAGING_FUNCTION": {
            "SET": ":SENS:AVER:TCON {}",
            "QUERY": ":SENS:AVER:TCON?",
            "verifier": is_in("MOV", "REP"),
        },
        "TRIGGER_COUNT": {
            "SET": ":TRIG:COUN {}",
            "QUERY": ":TRIG:COUN?",
            "verifier": verifier_or(is_integer(min=1, max=2500), is_in("INF")),
            "parser": numeric_int,
        },
        "TRIGGER_SOURCE": {
            "SET": ":TRIG:SOUR {}",
            "QUERY": ":TRIG:SOUR?",
            "verifier": is_in("IMM", "TIM", "MAN", "BUS", "TLIN", "NST", "PST", "BST"),
        },
        "TRIGGER_TIMER": {
            "SET": ":TRIG:TIM {:.3f}",
            "QUERY": ":TRIG:TIM?",
            "verifier": is_numeric(min=0.001, max=999999.999),
            "parser": numeric_float,
        },
        "TRIGGER_DELAY": {
            "SET": ":TRIG:DEL {:.3f}",
            "QUERY": ":TRIG:DEL?",
            "verifier": is_numeric(min=0.001, max=999999.999),
            "parser": numeric_float,
        },
        "AUTOZERO": {
            "SET": ":SYST:AZER:STAT {}",
            "QUERY": ":SYS:AZER:STAT?",
            "verifier": is_in("ON", "OFF", "ONCE"),
        },
        "READ": {
            "QUERY": ":READ?",
            "parser": float_list(","),
        },
        "MEASURE": {
            "QUERY": ":MEAS{}?",
            "verifier": is_in(*MEASURE_TYPES.keys()),
            "parser": float_list(","),
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    MEASURE_TYPE_MAP = {"VOLT:DC": 0, "CURR:DC": 1, "RES": 2}

    class PowerChannel(Instrument.PowerChannel):
        """PowerChannel implementation for Keithley2410."""

        @property
        def status(self):
            """
            :returns: Measurement Status Register value.
            """
            return self._instrument.query("MEASUREMENT_STATUS_REGISTER")

        @property
        def state(self):
            return self._instrument.query("OUTPUT")

        @state.setter
        def state(self, value):
            self._instrument.set("OUTPUT", value)

        @property
        def source_mode(self):
            return self._instrument.query("SOURCE_FUNCTION")

        @source_mode.setter
        def source_mode(self, value):
            self._instrument.set("SOURCE_FUNCTION", value)

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
            return self._instrument.query("OVP")

        @voltage_limit.setter
        def voltage_limit(self, value):
            # Cannot turn off for this instrument - set to max
            if value is False or value is None:
                value = "MAX"
            self._instrument.set("OVP", value)

        @property
        def voltage_trip(self):
            return self._instrument.query("OVP_TRIPPED")

        def reset_voltage_trip(self):
            # Nothing to do - Keithley24xx trips do not latch
            # Return whether we are back out of compliance
            return not self.voltage_trip

        @property
        def current_limit(self):
            return self._instrument.query("OCP")

        @current_limit.setter
        def current_limit(self, value):
            # Cannot turn off for this instrument - set to max
            if value is False or value is None:
                value = "MAX"
            self._instrument.set("SENSE_CURRENT_RANGE", value)
            self._instrument.set("OCP", value)

        @property
        def current_trip(self):
            return self._instrument.query("OCP_TRIPPED")

        def reset_current_trip(self):
            # Nothing to do - Keithley24xx trips do not latch
            # Return whether we are back out of compliance
            return not self.current_trip

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for Keithley2410."""

        @property
        def status(self):
            """
            :returns: Measurement Status Register value.
            """
            return self._instrument.query("MEASUREMENT_STATUS_REGISTER")

        @property
        def value(self):
            if self._instrument.status() == 0:
                return 0.0
            else:
                return self._instrument.measure_single(self._measure_type)[
                    type(self._instrument).MEASURE_TYPE_MAP[self._measure_type]
                ]

    def __init__(self, resource="ASRL1::INSTR", sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        """
        super().__init__(resource, sim)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to Keithley 2410.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `Keithley2410` object in activated state.
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
        """Closes connection to Keithley 2410.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `HMP4040` object in activated state.
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

    @acquire_lock()
    def selftest(self):
        """Run self-test.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set`-call is nested within).

        :returns: result of `COM_SELFTEST` SCPI command.
        """
        return self._instrument.query(type(self).COM_SELFTEST)

    @acquire_lock()
    def device_clear(self):
        """Send Device Clear command.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `device_clear`-call is nested within).

        :returns: result of `COM_DCL` SCPI command.
        """
        return self._instrument.query(type(self).COM_DEVICE_CLEAR)

    def off(self, **kwargs):
        """Turn output off.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `off`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result of set(...).
        """
        # uses lock from sub-call
        return self.set("OUTPUT", "OFF", **kwargs)

    def on(self, **kwargs):
        """Turn output on.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `on`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result of set(...).
        """
        # uses lock from sub-call
        return self.set("OUTPUT", "ON", **kwargs)

    def status(self, **kwargs):
        """Read output status.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `status`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result of query(...).
        """
        # uses lock from sub-call
        return int(self.query("OUTPUT", **kwargs))

    @acquire_lock()
    @retry_on_fail_visa()
    def clear_trace(self):
        """Clears measurement trace.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result of `COM_CLEAR_TRACE` SCPI command.
        """
        return self._instrument.write(type(self).COM_CLEAR_TRACE)

    @acquire_lock()
    def reset(self):
        """Resets the instrument and clears the queue.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `reset`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result of `status:queue:clear;*RST;:stat:pres;:*CLS;` SCPI command.
        """
        return self._instrument.write("status:queue:clear;*RST;:stat:pres;:*CLS;")

    @acquire_lock()
    def measure(
        self,
        repetitions=1,
        delay=None,
        clear_trace=True,
        paste=False,
        measured_unit=None,
    ):
        """Measure output voltage, current averaged over `repetitions` measurements.

        :param repetitions: how many measurements to make. Defaults to 1.
        :param delay: delay between measurements. Only used when repetitions > 1.
        :param clear_trace: whether to clear trace after measurements. Defaults to True.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure`-call is nested within).

        :returns: tuple of `(volts, amps, ohms, timestamp, filter)`, averaged over
            measurements.
        """
        self.set("TRIGGER_COUNT", repetitions, no_lock=True)
        self.set("SENSE_FUNCTION", "VOLT:DC", no_lock=True)
        self.set("SENSE_FUNCTION", "CURR:DC", no_lock=True)
        if delay is not None:
            self.set("DELAY", delay, no_lock=True)
        if self.status(no_lock=True) == 0:
            volts, amps, ohms, timestamp, filter_ = 0, 0, 0, 0, 0
        elif repetitions == 1:
            volts, amps, ohms, timestamp, filter_ = self.query("READ", no_lock=True)
        # If we have more than one measurement per data point, we have to fill the
        # results into arrays.
        else:
            results_array = []
            results_array.append(self.query("READ", no_lock=True))
            volts = results_array[0][0::5]
            amps = results_array[0][1::5]
            ohms = results_array[0][2::5]
            timestamp = results_array[0][3::5]
            filter_ = results_array[0][4::5]
        if clear_trace:
            self.clear_trace(no_lock=True)
        if paste is True:
            if repetitions == 1:
                print(
                    "Time: "
                    + str(timestamp)
                    + "\t Voltage: "
                    + str(volts)
                    + "\t Current: "
                    + str(amps)
                )
            else:
                for i in range(repetitions):
                    print(
                        "Time: "
                        + str(timestamp[i])
                        + "\t Voltage: "
                        + str(volts[i])
                        + "\t Current: "
                        + str(amps[i])
                    )
        return volts, amps, ohms, timestamp, filter_

    def measure_single(self, clear_trace=True, **kwargs):
        """Perform single measurement.

        :param clear_trace: whether to clear trace after measurements. Defaults to True.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure_single`-call is nested within).

        :returns: tuple of `(volts, amps, ohms, timestamp, filter)`.
        """
        # uses lock from sub-call
        return self.measure(
            repetitions=1, delay=None, clear_trace=clear_trace, **kwargs
        )

    def set_voltage_range(self, low, high):
        """Set a voltage range limit on this instrument.

        :param low: Low voltage limit in V.
        :param high: High voltage limit in V.
        """
        self.SETTINGS["VOLTAGE"]["verifier"] = is_numeric(min=low, max=high)

    def change_voltage_range(self, positive):
        """Change to predefined software voltage range limit.

        :param positive: If `true`, limit is set to -1mV < V_set < 1kV. Else, limit is
            set to -1kV < V_set < 1mV.
        """
        if positive:
            self.set_voltage_range(-0.001, 1000.0)
        else:
            self.set_voltage_range(-1000.0, 0.001)

    def sweep_print_header(self, measured_unit=None):
        """Returns the header for print-outs, when doing a sweep. Helper function of
        instrument.sweep()

        :param measured_unit: not needed here. Is used in some cases, where a different
            device does the measurement than the sweep.
        """
        return (
            "Time",
            "Voltage (V)",
            "Current (A)",
            "Resistance",
            "PSU Time",
            "Status Register",
        )

    def publish(self, client, line, topic=None):
        """Publish data line (from Monitoring) to MQTT.

        TODO: don't reparse line here; instead pass/access raw data?

        :param client: MQTTClient object.
        :param line: logfile line from MonitoringLogger.
        """
        if line.startswith("#"):
            return
        topic = topic or self._resource[9:-7]
        values = line.split("\t")
        client.publish(
            f"{topic}/voltage", f"{topic} voltage={float(values[1].strip())}"
        )
        client.publish(
            f"{topic}/current", f"{topic} current={float(values[2].strip())}"
        )
        client.publish(
            f"{topic}/resistance", f"{topic} resistance={float(values[3].strip())}"
        )
        client.publish(
            f"{topic}/PSUTime", f"{topic} PSUTime={float(values[4].strip())}"
        )
        client.publish(
            f"{topic}/statusRegister",
            f"{topic} statusRegister={float(values[5].strip())}",
        )


# Deprecated endpoints for backward compatibility - remove these eventually!
Keithley2410.SETTINGS["COMPLIANCE_CURRENT"] = Keithley2410.SETTINGS["OCP"]
Keithley2410.SETTINGS["COMPLIANCE_CURRENT_TRIPPED"] = Keithley2410.SETTINGS[
    "OCP_TRIPPED"
]
Keithley2410.SETTINGS["COMPLIANCE_VOLTAGE"] = Keithley2410.SETTINGS["OVP"]
Keithley2410.SETTINGS["COMPLIANCE_VOLTAGE_TRIPPED"] = Keithley2410.SETTINGS[
    "OVP_TRIPPED"
]
Keithley2410.SETTINGS["SOURCE"] = Keithley2410.SETTINGS["SOURCE_FUNCTION"]
Keithley2410.SETTINGS["SENSE_AVERAGING"] = Keithley2410.SETTINGS["ENABLE_AVERAGING"]
Keithley2410.SETTINGS["SENSE_AVERAGING_COUNT"] = Keithley2410.SETTINGS[
    "AVERAGING_COUNT"
]
Keithley2410.SETTINGS["SENSE_AVERAGING_TYPE"] = Keithley2410.SETTINGS[
    "AVERAGING_FUNCTION"
]
Keithley2410.SETTINGS.update(
    {
        "SENSE_CURRENT_INTEGRATION": {
            "SET": ":SENS:CURR:DC:NPLC {}",
            "QUERY": ":SENS:CURR:DC:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_VOLTAGE_INTEGRATION": {
            "SET": ":SENS:VOLT:DC:NPLC {}",
            "QUERY": ":SENS:VOLT:DC:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_RESISTANCE_INTEGRATION": {
            "SET": ":SENS:RES:NPLC {}",
            "QUERY": ":SENS:RES:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "VOLTAGE_MODE": {
            "SET": ":SOUR:VOLT:MODE {}",
            "QUERY": ":SOUR:VOLT:MODE?",
            "verifier": is_in("FIX", "SWE"),
            "parser": numeric_float,
        },
        "VOLTAGE_SWEEP_START": {
            "SET": ":SOUR:VOLT:START {}",
            "QUERY": ":SOUR:VOLT:START?",
            "verifier": is_numeric(min=-1100, max=1100),
            "parser": numeric_float,
        },
        "VOLTAGE_SWEEP_END": {
            "SET": ":SOUR:VOLT:END {}",
            "QUERY": ":SOUR:VOLT:END?",
            "verifier": is_numeric(min=-1100, max=1100),
            "parser": numeric_float,
        },
        "VOLTAGE_SWEEP_STEP": {
            "SET": ":SOUR:VOLT:STEP {}",
            "QUERY": ":SOUR:VOLT:STEP?",
            "verifier": is_numeric(min=-1100, max=1100),
            "parser": numeric_float,
        },
        "CURR_MODE": {
            "SET": ":SOUR:CURR:MODE {}",
            "QUERY": ":SOUR:CURR:MODE?",
            "verifier": is_in("FIX", "SWE"),
        },
        "CURR_SWEEP_START": {
            "SET": ":SOUR:CURR:START {}",
            "QUERY": ":SOUR:CURR:START?",
            # Takes max from 2430 Pulse Mode
            "verifier": is_numeric(min=-10.5, max=10.5),
            "parser": numeric_float,
        },
        "CURR_SWEEP_END": {
            "SET": ":SOUR:CURR:END {}",
            "QUERY": ":SOUR:CURR:END?",
            # Takes max from 2430 Pulse Mode
            "verifier": is_numeric(min=-10.5, max=10.5),
            "parser": numeric_float,
        },
        "CURR_SWEEP_STEP": {
            "SET": ":SOUR:CURR:STEP {}",
            "QUERY": ":SOUR:CURR:STEP?",
            # Takes max from 2430 Pulse Mode
            "verifier": is_numeric(min=-10.5, max=10.5),
            "parser": numeric_float,
        },
    }
)
