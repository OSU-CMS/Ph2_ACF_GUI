"""HP34401A class for HP34401A multimeter."""

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
)


logger = logging.getLogger(__name__)


@Instrument.register
class HP34401A(SCPIInstrument, key="resource"):
    BAUD_RATE = 9600
    """Serial link Baud rate."""
    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout (ms)."""
    READ_TERMINATION = "\r\n"
    """Read termination characters."""

    WRITE_TERMINATION = "\r\n"
    """Write termination characters."""
    SMALL_DELAY = 0.5
    COM_SELFTEST = "*TST?"
    """Selftest SCPI command."""
    COM_RESET = "*RST; STATUS:PRESET; *CLS"
    """Instrument Reset SCPI command."""
    COM_CLEAR_TRACE = ":TRAC:CLE"
    """Clear trace SCPI command."""

    COM_IMMEDIATE_TRIGGER = ":INIT:IMM"
    """Immediate Trigger SCPI command."""

    MEASURE_TYPES = {
        "VOLT:DC": "DC Voltage (V)",
        "VOLT:AC": "AC Voltage (V)",
        "CURR:DC": "DC Current (A)",
        "CURR:AC": "AC Current (A)",
        "RES": "Resistance (Ohm)",
        "FRES": "Four-Wire Resistance (Ohm)",
        "VOLT:DC:RAT": "DC Voltage Ratio",
        "FREQ": "Frequency (Hz)",
        "PER": "Period ()",  # put units
        "CONT": "Continuity",
        "DIOD": "Diode",
    }
    """Measurement types for this instrument."""

    REGISTERS = {
        "STATUS_BYTE": "Status Byte",
        "QUESTIONABLE_STATUS_REGISTER": "Questionable Status Register",
        "EVENT_STATUS_REGISTER": "Event Status Register",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
        "SYSTEM_MODE": {"SET": ":SYST:{}", "verifier": is_in("LOC", "REM", "RWL")},
        "CONFIGURE": {
            "SET": ":CONF:{}",
            "QUERY": ":CONF?",
            "verifier": is_in(*MEASURE_TYPES.keys()),
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
            "verifier": is_integer(min=1, max=50000),
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
            "QUERY": "*ESR {}",
            "verifier": is_integer(min=0, max=65536),
            "parser": numeric_int,
        },
        "QUESTIONABLE_STATUS_REGISTER": {
            "QUERY": ":STAT:QUES:EVEN?",
            "parser": numeric_int,
        },
        "QUESTIONABLE_STATUS_REGISTER_ENABLE": {
            "SET": ":STAT:QUES:ENAB {}",
            "QUERY": ":STAT:QUES:ENAB?",
            "verifier": is_integer(min=1, max=65535),
            "parser": numeric_int,
        },
        "MEASUREMENT_STATUS_REGISTER": {
            "QUERY": ":STAT:MEAS:EVEN?",
            "parser": numeric_int,
        },
        "MEASUREMENT_STATUS_REGISTER_ENABLE": {
            "SET": ":STAT:MEAS:ENAB {}",
            "QUERY": ":STAT:MEAS:ENAB?",
            "verifier": is_integer(min=1, max=65535),
            "parser": numeric_int,
        },
        # Can only set LINE INTEGRATION CYCLES on first 6 measure types
        "LINE_INTEGRATION_CYCLES": {
            "SET": ":SENS:{}:NPLC {:.2f}",
            "QUERY": ":SENS:{}:NPLC?",
            "verifier": (
                is_in(*islice(MEASURE_TYPES.keys(), 6)),
                is_numeric(min=0.02, max=100),
            ),
            "parser": numeric_float,
        },
        # A garbage command where query doesn't match set behaviour
        #  - hence truthy parser for simulation
        "ENABLE_AVERAGING": {  # maybe rename
            "SET": ":CALC:STAT {}",
            "QUERY": ":CALC:STAT?",
            "verifier": truthy(true_output="ON", false_output="OFF"),
            "parser": truthy_bool(),
        },
        "AVERAGING_COUNT": {
            "SET": ":CALC:AVER:COUN {}",
            "QUERY": ":CALC:AVER:COUN?",
            "verifier": is_integer(min=1, max=100),
            "parser": numeric_int,
        },
        "AVERAGING_FUNCTION": {
            "SET": ":CALC:FUNC {}",
            "QUERY": ":CALC:FUNC?",
            "verifier": is_in("NULL", "DB", "DBM", "AVER", "LIM"),
        },
        "TRIGGER_COUNT": {
            "SET": ":TRIG:COUN {}",
            "QUERY": ":TRIG:COUN?",
            "verifier": verifier_or(is_integer(min=1, max=9999), is_in("INF")),
            # float("INF") == inf; thanks python :D
            "parser": numeric_float,
        },
        "TRIGGER_SOURCE": {
            "SET": ":TRIG:SOUR {}",
            "QUERY": ":TRIG:SOUR?",
            "verifier": is_in("BUS", "IMM", "EXT"),
        },
        "TRIGGER_DELAY": {
            "SET": ":TRIG:DEL {}",
            "QUERY": ":TRIG:DEL?",
            "verifier": is_numeric(min=0, max=3600),  # in seconds
            "parser": numeric_float,
        },
        # Another garbage command where query doesn't match set behaviour
        "TRIGGER_DELAY_AUTO": {
            "SET": ":TRIG:DEL:AUTO {}",
            "QUERY": ":TRIG:DEL:AUTO?",
            "verifier": truthy(true_output="ON", false_output="OFF"),
            "parser": truthy_bool(),
        },
        # Another garbage command where query doesn't match set behaviour
        "AUTOZERO": {
            "SET": ":SENS:ZERO:AUTO {}",
            "QUERY": ":SENS:ZERO:AUTO?",
            "verifier": is_in("ON", "OFF", "ONCE"),
            "parser": truthy_bool(false_like=(0, "0", "OFF", "ONCE")),
        },
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for HP34401A."""

        @property
        def value(self):
            return self._instrument.measure_single(self._measure_type)[0]

        @property
        def status(self):
            """
            :returns: Measurement Status Register value.
            """
            return self._instrument.query("MEASUREMENT_STATUS_REGISTER")

    def __init__(self, resource="ASRL5::INSTR", sim=False):
        """
        :param resource: VISA Resource address. See VISA docs for more info.
        """
        super().__init__(resource, sim=sim)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to Multimeter.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `Multimeter` object in activated state.
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
        """Closes connection to Multimeter.

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
        return self.set(
            "QUESTIONABLE_STATUS_ENABLE_REGISTER", 512, no_lock=True
        ) and self.set("STATUS_BYTE_ENABLE", 1, no_lock=True)

    @acquire_lock()
    @retry_on_fail_visa()
    def immediate_trigger(self):
        """Send immediate trigger.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `immediate_trigger`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :returns: number of bytes written to serial interface.
        """
        return self._instrument.write(type(self).COM_IMMEDIATE_TRIGGER)

    def set_line_integration(self, what, line_integration_cycles=1, **kwargs):
        """Set line integration cycles for given measurement type.

        :param what: one of 'VOLT:DC', 'VOLT:AC', 'CURR:DC', 'CURR:AC', 'RES', 'FRES'.
        :param line_integration_cycles: number of line integration cycles.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `no_lock`-call is nested within).
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
        assert what in type(self).MEASURE_TYPES.keys()

        if self.query("CONFIGURE", no_lock=True).strip('"') != what:
            self.set("CONFIGURE", what, no_lock=True)
        if what not in ("PER", "CONT", "FREQ", "DIOD"):
            self.set_line_integration(
                what if what != "VOLT:DC:RAT" else "VOLT:DC", cycles, no_lock=True
            )
        # This if else block may need to be modified.  The stuff added block might not
        # jive with this.
        if repetitions > 1:
            rval = []
            for _ in range(repetitions):
                rval.append(float(self.query("READ", no_lock=True)))
                if delay is not None:
                    time.sleep(delay)
        else:
            rval = [float(self.query("READ", no_lock=True))]

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

    def sweep_print_header(self, *ignored):
        """Returns the header for print-outs, when doing a sweep.

        Helper function of instrument.sweep(). Required parameters (e.g.
            `measured_unit`) are ignored as not needed in this case.
        """
        what = self.query("CONFIGURE", no_lock=True)
        assert what in type(self).MEASURE_TYPES.keys()
        output = type(self).MEASURE_TYPES.get(what)
        return ("Time", output)


# Add back old command names for backward compatibility - TODO: deprecate these
HP34401A.SETTINGS["MEASUREMENT_EVENT_REGISTER"] = HP34401A.SETTINGS[
    "QUESTIONABLE_STATUS_REGISTER_ENABLE"
]
HP34401A.SETTINGS["MEASUREMENT_REGISTER"] = HP34401A.SETTINGS[
    "QUESTIONABLE_STATUS_REGISTER"
]
HP34401A.SETTINGS["SERVICE_REQUEST_ENABLE"] = HP34401A.SETTINGS["STATUS_BYTE_ENABLE"]
HP34401A.SETTINGS["SENSE_AVERAGING"] = HP34401A.SETTINGS["ENABLE_AVERAGING"]
HP34401A.SETTINGS["SENSE_AVERAGING_COUNT"] = HP34401A.SETTINGS["AVERAGING_COUNT"]
HP34401A.SETTINGS["SENSE_AVERAGING_TYPE"] = HP34401A.SETTINGS["AVERAGING_FUNCTION"]
HP34401A.SETTINGS["AUTODELAY_SWITCH"] = HP34401A.SETTINGS["TRIGGER_DELAY_AUTO"]
HP34401A.SETTINGS.update(
    {
        "SENSE_CURRENT_INTEGRATION": {
            "SET": ":SENS:CURR:DC:NPLC {:.2f}",
            "QUERY": ":SENS:CURR:DC:NPLC?",
            "verifier": is_numeric(min=0.02, max=100),
            "parser": numeric_float,
        },
        "SENSE_VOLTAGE_INTEGRATION": {
            "SET": ":SENS:VOLT:DC:NPLC {:.2f}",
            "QUERY": ":SENS:VOLT:DC:NPLC?",
            "verifier": is_numeric(min=0.02, max=100),
            "parser": numeric_float,
        },
        "SENSE_RESISTANCE_INTEGRATION": {
            "SET": ":SENS:RES:NPLC {:.2f}",
            "QUERY": ":SENS:RES:NPLC?",
            "verifier": is_numeric(min=0.02, max=100),
            "parser": numeric_float,
        },
        "SENSE_FRESISTANCE_INTEGRATION": {
            "SET": ":SENS:FRES:NPLC {:.2f}",
            "QUERY": ":SENS:FRES:NPLC?",
            "verifier": is_numeric(min=0.02, max=100),
            "parser": numeric_float,
        },
    }
)
