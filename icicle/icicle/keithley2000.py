"""Keithley2000 class for Keithley 2000 multimeter."""

import time
import logging
from itertools import islice

from .instrument import acquire_lock, Instrument, ChannelError
from .visa_instrument import retry_on_fail_visa
from .scpi_instrument import (
    SCPIInstrument,
    is_in,
    is_integer,
    is_numeric,
    verifier_or,
    truthy,
)
from .utils.parser_utils import (
    numeric_float,
    numeric_int,
    truthy_bool,
    strip_str,
    float_list,
)

logger = logging.getLogger(__name__)


@Instrument.register
class Keithley2000(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Keithley2000 multimeter."""

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
    COM_IMMEDIATE_TRIGGER = "INIT:IMM"
    """Immediate Trigger SCPI command."""

    MEASURE_TYPES = {
        "VOLT:DC": "DC Voltage (V)",
        "VOLT:AC": "AC Voltage (V)",
        "CURR:DC": "DC Current (A)",
        "CURR:AC": "AC Current (A)",
        "RES": "Resistance (Ohm)",
        "FRES": "Four-Wire Resistance (Ohm)",
        "FREQ": "Frequency (Hz)",
        "PER": "Period (s)",
    }
    """Measurement types for this instrument."""

    REGISTERS = {
        "STATUS_BYTE": "Status Byte",
        "EVENT_STATUS_REGISTER": "Event Status Register",
        "QUESTIONABLE_STATUS_REGISTER": "Questionable Status Register",
        "MEASUREMENT_STATUS_REGISTER": "Measurement Status Register",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "SYSTEM_MODE": {"SET": ":SYST:{}", "verifier": is_in("LOC", "REM", "RWL")},
        "CONFIGURE": {
            "SET": ":CONF:{}",
            "QUERY": ":CONF?",
            "verifier": is_in(*MEASURE_TYPES.keys()),
            "parser": strip_str('"'),
        },
        "SENSE_FUNCTION": {
            "SET": ':SENS:FUNC "{}"',
            "QUERY": ":SENS:FUNC?",
            "verifier": is_in(*MEASURE_TYPES.keys()),
        },
        "READ": {"QUERY": ":READ?", "parser": numeric_float},
        "SAMPLE_COUNT": {
            "SET": ":SAMP:COUN {}",
            "QUERY": ":SAMP:COUN?",
            "verifier": is_integer(min=1, max=1024),
            "parser": numeric_int,
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
        # Can only set LINE INTEGRATION CYCLES on first 6 measure types
        "LINE_INTEGRATION_CYCLES": {
            "SET": ":SENS:{}:NPLC {:.2f}",
            "QUERY": ":SENS:{}:NPLC?",
            "verifier": (
                is_in(*islice(MEASURE_TYPES.keys(), 6)),
                is_numeric(min=0.02, max=10),
            ),
            "parser": numeric_float,
        },
        "ENABLE_AVERAGING": {
            "SET": ":SENS:AVER {}",
            "QUERY": ":SENS:AVER?",
            "verifier": is_in("ON", "OFF"),
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
        "INITIALISE_CONTINUOUS": {
            "SET": ":INIT:CONT {}",
            "QUERY": ":INIT:CONT?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": truthy_bool(),
        },
        "TRIGGER_COUNT": {
            "SET": ":TRIG:COUN {}",
            "QUERY": ":TRIG:COUN?",
            "verifier": verifier_or(is_integer(min=2, max=9999), is_in("INF")),
            # float("INF") == inf; thanks python :D
            "parser": numeric_int,
        },
        "TRIGGER_SOURCE": {
            "SET": ":TRIG:SOUR {}",
            "QUERY": ":TRIG:SOUR?",
            "verifier": is_in("IMM", "TIM", "MAN", "BUS", "EXT"),
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
        "TRIGGER_DELAY_AUTO": {
            "SET": ":TRIG:DEL:AUTO {}",
            "QUERY": ":TRIG:DEL:AUTO?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": truthy_bool(),
        },
        "TRACE_BUFFER_SIZE": {
            "SET": ":TRAC:POIN {}",
            "QUERY": ":TRAC:POIN?",
            "verifier": is_integer(min=2, max=1024),
            "parser": numeric_int,
        },
        "TRACE_BUFFER_SOURCE": {
            "SET": ":TRAC:FEED {}",
            "QUERY": ":TRAC:FEED?",
            "verifier": is_in("SENS", "SENS1", "CALC", "NONE"),
        },
        "TRACE_BUFFER_CONTROL_MODE": {
            "SET": ":TRAC:FEED:CONT {}",
            "QUERY": ":TRAC:FEED:CONT?",
            "verifier": is_in("NEXT", "NEV"),
        },
        "AUTOZERO": {
            "SET": ":SYST:AZER:STAT {}",
            "QUERY": ":SYS:AZER:STAT?",
            "verifier": truthy(true_output=1, false_output=0),
            "parser": truthy_bool(),
        },
        "SENSE_DATA": {"QUERY": ":SENS:DATA?", "parser": float_list(",")},
        "TRACE_DATA": {"QUERY": ":TRAC:DATA?", "parser": float_list(",")},
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for Keithley2000."""

        @property
        def value(self):
            return self._instrument.measure_single(self._measure_type)[0]

        @property
        def status(self):
            """
            :returns: Status Byte value.
            """
            return self._instrument.query("STATUS_BYTE")

    def __init__(self, resource="ASRL6::INSTR", sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        """
        super().__init__(resource, sim=sim)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to Keithley 2000.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `Keithley2000` object in activated state.
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
        """Closes connection to Keithley 2000.

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

    @acquire_lock()
    def selftest(self):
        """Run self-test.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `selftest`-call is nested within).

        :returns: result of `COM_SELFTEST` SCPI command.
        """
        return self._instrument.query(type(self).COM_SELFTEST)

    @acquire_lock()
    @retry_on_fail_visa()
    def clear_trace(self):
        """Clears measurement trace.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `clear_trace`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: result of `COM_CLEAR_TRACE` SCPI command.
        """
        return self._instrument.write(type(self).COM_CLEAR_TRACE)

    @acquire_lock()
    def enable_register(self):
        """Enables measurement register.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `enable_register`-call is nested within).

        :returns: True-ish value on success.
        """
        return self.set("MEASUREMENT_REGISTER", 512, no_lock=True) and self.set(
            "SERVICE_REQUEST_ENABLE", 1, no_lock=True
        )

    @acquire_lock()
    @retry_on_fail_visa()
    def immediate_trigger(self):
        """Send immediate trigger.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `no_lock`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: number of bytes written to serial interface.
        """
        return self._instrument.write(type(self).COM_IMMEDIATE_TRIGGER)

    def set_line_integration(self, what, line_integration_cycles=1, **kwargs):
        """Set line integration cycles for given measurement type.

        :param what: one of 'VOLT:DC', 'VOLT:AC', 'CURR:DC', 'CURR:AC', 'RES', 'FRES'.
        :param line_integration_cycles: number of line integration cycles.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set_line_integrations`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: read-back value from set(...).
        """
        return self.set(
            "LINE_INTEGRATION_CYCLES", what, line_integration_cycles, **kwargs
        )

    @acquire_lock()
    def set_all_line_integrations(self, line_integration_cycles=1):
        """Set line integration cycles for all measurement types.

        :param line_integration_cycles: number of line integration cycles.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set_all_line_integrations`-call is nested within).

        :returns: read-back value from set(...).
        """
        ret = True
        for what in islice(type(self).MEASURE_TYPES.keys(), 6):
            ret = ret and self.set(
                "LINE_INTEGRATION_CYCLES", what, line_integration_cycles, no_lock=True
            )
        return ret

    @acquire_lock()
    def measure_old(
        self, what, repetitions=1, delay=None, cycles=1.0, clear_trace=True
    ):
        """Old implementation of measure().

        Doesn't work reliably - do not use.
        """
        print(repetitions, what)
        assert what in ("VOLT", "VOLT:AC", "CURR", "CURR:AC", "RES", "FRES")
        self.set("TRIGGER_SOURCE", "IMM", no_lock=True)
        self.set_line_integration(what, cycles, no_lock=True)
        self.set("SAMPLE_COUNT", repetitions, no_lock=True)
        if repetitions > 1:
            self.set("TRACE_BUFFER_SIZE", repetitions, no_lock=True)
        self.set("SENSE_FUNCTION", f'"{what}"', no_lock=True)
        self.set("TRACE_BUFFER_CONTROL_MODE", "NEXT", no_lock=True)
        if delay is not None:
            self.set("DELAY", delay, no_lock=True)

        # Trigger
        # self.immediate_trigger(no_lock=True)

        if delay is not None:
            wait_time = repetitions * (delay + cycles / 60)
        else:
            wait_time = repetitions * cycles / 60
        logger.debug(f"Waiting {wait_time} seconds to collect data...")
        time.sleep(wait_time)

        rval = (
            [
                [float(aa) for aa in a.split(",") if aa != ""]
                for a in self.query("TRACE_DATA", no_lock=True).split(";")
            ],
            int(self.query("MEASUREMENT_EVENT_REGISTER", no_lock=True)),
        )
        if clear_trace:
            self.clear_trace(no_lock=True)
        return rval

    @acquire_lock()
    def measure(self, what, repetitions=1, delay=None, cycles=1.0, clear_trace=True):
        """Performs measurement of requested type.

        :param what: one of 'VOLT:DC', 'VOLT:AC', 'CURR:DC', 'CURR:AC', 'RES', 'FRES'.
        :param repetitions: how many measurements to make. Defaults to 1.
        :param delay: delay between measurements. Only used when repetitions > 1.
        :param cycles: number of line integration cycles for this measurment type.
            Defaults to 1.
        :param clear_trace: whether to clear trace after measurements. Defaults to True.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure`-call is nested within).

        :returns: list of measurement values (as floats).
        """
        if (
            self.query("CONFIGURE", no_lock=True).strip('"') != what
            or int(self.query("INITIALISE_CONTINUOUS", no_lock=True)) == 1
        ):
            self.set("CONFIGURE", what, no_lock=True)

        if what in self.SETTINGS["LINE_INTEGRATION_CYCLES"]["verifier"][0].valid:
            self.set_line_integration(what, cycles, no_lock=True)

        if repetitions > 1:
            rval = []
            for _ in range(repetitions):
                rval.append(float(self.query("READ", no_lock=True)))
                if delay is not None:
                    time.sleep(delay)
        else:
            rval = [float(self.query("READ", no_lock=True))]

        # if what == 'RES' or what =='FRES':
        #    self.set('SENSE_FUNCTION', 'VOLT:DC', no_lock=True)

        if clear_trace:
            self.clear_trace(no_lock=True)
        return rval

    def measure_single(self, what, clear_trace=True, **kwargs):
        """Perform single measurement.

        :param what: one of 'VOLT:DC', 'VOLT:AC', 'CURR:DC', 'CURR:AC', 'RES', 'FRES'.
        :param clear_trace: whether to clear trace after measurements. Defaults to True.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure_single`-call is nested within).

        :returns: measured value (float).
        """
        # uses lock from sub-call
        return self.measure(
            what, repetitions=1, delay=None, clear_trace=clear_trace, **kwargs
        )

    def publish(self, client, line, what, topic=None):
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
        client.publish(
            f"{topic}/{what.lower()}",
            f"{topic} {what.lower()}={float(values[1].strip())}",
        )

    def sweep_print_header(self, measured_unit):
        """Returns the header for print-outs, when doing a sweep.

        Helper function of instrument.sweep() @param: measured_unit: not needed here. Is
        required in some cases, where sweep and measurement are performed by different
        devices.
        """
        what = self.query("CONFIGURE", no_lock=True)
        assert what in ("VOLT:DC", "VOLT:AC", "CURR:DC", "CURR:AC", "RES", "FRES")
        if what == "VOLT:DC":
            output = "DC Voltage (V)"
        elif what == "VOLT:AC":
            output = "AC Voltage (V)"
        elif what == "CURR:DC":
            output = "DC Current (A)"
        elif what == "CURR:AC":
            output = "AC Current (A)"
        elif what == "RES":
            output = "Resistance (Ohm)"
        elif what == "FRES":
            output = "Four-Wire Resistance (Ohm)"
        return ("Time", output)


# Add back old command names for backward compatibility - TODO: deprecate these
Keithley2000.SETTINGS["MEASUREMENT_EVENT_REGISTER"] = Keithley2000.SETTINGS[
    "QUESTIONABLE_DATA_REGISTER_ENABLE"
]
Keithley2000.SETTINGS["MEASUREMENT_REGISTER"] = Keithley2000.SETTINGS[
    "QUESTIONABLE_DATA_REGISTER"
]
Keithley2000.SETTINGS["SERVICE_REQUEST_ENABLE"] = Keithley2000.SETTINGS[
    "STATUS_BYTE_ENABLE"
]
Keithley2000.SETTINGS["SENSE_AVERAGING"] = Keithley2000.SETTINGS["ENABLE_AVERAGING"]
Keithley2000.SETTINGS["SENSE_AVERAGING_COUNT"] = Keithley2000.SETTINGS[
    "AVERAGING_COUNT"
]
Keithley2000.SETTINGS["SENSE_AVERAGING_TYPE"] = Keithley2000.SETTINGS[
    "AVERAGING_FUNCTION"
]
Keithley2000.SETTINGS["AUTODELAY_SWITCH"] = Keithley2000.SETTINGS["TRIGGER_DELAY_AUTO"]
Keithley2000.SETTINGS["DELAY"] = Keithley2000.SETTINGS["TRIGGER_DELAY"]
Keithley2000.SETTINGS.update(
    {
        "SENSE_CURRENT_INTEGRATION": {
            "SET": ":SENS:CURR:NPLC {:.2f}",
            "QUERY": ":SENS:CURR:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_VOLTAGE_INTEGRATION": {
            "SET": ":SENS:VOLT:NPLC {:.2f}",
            "QUERY": ":SENS:VOLT:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_CURRENT_AC_INTEGRATION": {
            "SET": ":SENS:CURR:AC:NPLC {:.2f}",
            "QUERY": ":SENS:CURR:AC:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_VOLTAGE_AC_INTEGRATION": {
            "SET": ":SENS:VOLT:AC:NPLC {:.2f}",
            "QUERY": ":SENS:VOLT:AC:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_RESISTANCE_INTEGRATION": {
            "SET": ":SENS:RES:NPLC {:.2f}",
            "QUERY": ":SENS:RES:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
        "SENSE_FRESISTANCE_INTEGRATION": {
            "SET": ":SENS:FRES:NPLC {:.2f}",
            "QUERY": ":SENS:FRES:NPLC?",
            "verifier": is_numeric(min=0.01, max=100),
            "parser": numeric_float,
        },
    }
)
