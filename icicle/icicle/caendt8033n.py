from .instrument import Instrument, acquire_lock, ChannelError
from .visa_instrument import retry_on_fail_visa
from .scpi_instrument import SCPIInstrument, is_in, is_numeric, is_integer

import time
import logging

logger = logging.getLogger(__name__)


@Instrument.register
class CaenDT8033N(SCPIInstrument, key="resource"):
    BAUD_RATE = 9600
    TIMEOUT = 10000
    OUTPUTS = 8
    MEASURE_TYPES = {
        "VOLT:DC": "DC Voltage (V)",
        "CURR:DC": "DC Current (A)",
    }
    SET_REQUIRES_READBACK = True
    MEASURE_COMMAND_MAP = {"VOLT:DC": "OUTPUT_VOLTAGE", "CURR:DC": "OUTPUT_CURRENT"}
    COM_RESET = "DCL"

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "$CMD:MON,PAR:BDNAME"},
        "CLEAR": {"SET": "$CMD:SET,PAR:BDCLR"},
        "STATUS": {"QUERY": "$CMD:MON,CH:8,PAR:STAT"},
        "STATUS1": {"QUERY": "$CMD:MON,CH:0,PAR:STAT"},
        "STATUS2": {"QUERY": "$CMD:MON,CH:1,PAR:STAT"},
        "STATUS3": {"QUERY": "$CMD:MON,CH:2,PAR:STAT"},
        "STATUS4": {"QUERY": "$CMD:MON,CH:3,PAR:STAT"},
        "STATUS5": {"QUERY": "$CMD:MON,CH:4,PAR:STAT"},
        "STATUS6": {"QUERY": "$CMD:MON,CH:5,PAR:STAT"},
        "STATUS7": {"QUERY": "$CMD:MON,CH:6,PAR:STAT"},
        "STATUS8": {"QUERY": "$CMD:MON,CH:7,PAR:STAT"},
        # VOLTAGE in range [-4000V, 0V] but set with positive values.
        "VOLTAGE": {
            "SET": "$CMD:SET,CH:8,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:8,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE1": {
            "SET": "$CMD:SET,CH:0,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:0,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE2": {
            "SET": "$CMD:SET,CH:1,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:1,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE3": {
            "SET": "$CMD:SET,CH:2,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:2,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE4": {
            "SET": "$CMD:SET,CH:3,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:3,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE5": {
            "SET": "$CMD:SET,CH:4,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:4,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE6": {
            "SET": "$CMD:SET,CH:5,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:5,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE7": {
            "SET": "$CMD:SET,CH:6,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:6,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "VOLTAGE8": {
            "SET": "$CMD:SET,CH:7,PAR:VSET,VAL:{}",
            "QUERY": "$CMD:MON,CH:7,PAR:VSET",
            "verifier": is_numeric(max=4000, min=0),
        },
        "OUTPUT_VOLTAGE": {"QUERY": "$CMD:MON,CH:8,PAR:VMON"},
        "OUTPUT_VOLTAGE1": {"QUERY": "$CMD:MON,CH:0,PAR:VMON"},
        "OUTPUT_VOLTAGE2": {"QUERY": "$CMD:MON,CH:1,PAR:VMON"},
        "OUTPUT_VOLTAGE3": {"QUERY": "$CMD:MON,CH:2,PAR:VMON"},
        "OUTPUT_VOLTAGE4": {"QUERY": "$CMD:MON,CH:3,PAR:VMON"},
        "OUTPUT_VOLTAGE5": {"QUERY": "$CMD:MON,CH:4,PAR:VMON"},
        "OUTPUT_VOLTAGE6": {"QUERY": "$CMD:MON,CH:5,PAR:VMON"},
        "OUTPUT_VOLTAGE7": {"QUERY": "$CMD:MON,CH:6,PAR:VMON"},
        "OUTPUT_VOLTAGE8": {"QUERY": "$CMD:MON,CH:7,PAR:VMON"},
        # CURRENT in range [0A, 3e-3 A] but passed in uA
        "CURRENT": {
            "SET": "$CMD:SET,CH:8,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:8,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT1": {
            "SET": "$CMD:SET,CH:0,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:0,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT2": {
            "SET": "$CMD:SET,CH:1,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:1,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT3": {
            "SET": "$CMD:SET,CH:2,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:2,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT4": {
            "SET": "$CMD:SET,CH:3,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:3,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT5": {
            "SET": "$CMD:SET,CH:4,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:4,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT6": {
            "SET": "$CMD:SET,CH:5,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:5,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT7": {
            "SET": "$CMD:SET,CH:6,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:6,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "CURRENT8": {
            "SET": "$CMD:SET,CH:7,PAR:ISET,VAL:{}",
            "QUERY": "$CMD:MON,CH:7,PAR:ISET",
            "verifier": is_numeric(max=3e3, min=0),
        },
        "OUTPUT_CURRENT": {"QUERY": "$CMD:MON,CH:8,PAR:IMON"},
        "OUTPUT_CURRENT1": {"QUERY": "$CMD:MON,CH:0,PAR:IMON"},
        "OUTPUT_CURRENT2": {"QUERY": "$CMD:MON,CH:1,PAR:IMON"},
        "OUTPUT_CURRENT3": {"QUERY": "$CMD:MON,CH:2,PAR:IMON"},
        "OUTPUT_CURRENT4": {"QUERY": "$CMD:MON,CH:3,PAR:IMON"},
        "OUTPUT_CURRENT5": {"QUERY": "$CMD:MON,CH:4,PAR:IMON"},
        "OUTPUT_CURRENT6": {"QUERY": "$CMD:MON,CH:5,PAR:IMON"},
        "OUTPUT_CURRENT7": {"QUERY": "$CMD:MON,CH:6,PAR:IMON"},
        "OUTPUT_CURRENT8": {"QUERY": "$CMD:MON,CH:7,PAR:IMON"},
        "OUTPUT": {
            "SET": "$CMD:SET,CH:8,PAR:{}",
            "QUERY": "$CMD:MON,CH:8,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT1": {
            "SET": "$CMD:SET,CH:0,PAR:{}",
            "QUERY": "$CMD:MON,CH:0,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT2": {
            "SET": "$CMD:SET,CH:1,PAR:{}",
            "QUERY": "$CMD:MON,CH:1,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT3": {
            "SET": "$CMD:SET,CH:2,PAR:{}",
            "QUERY": "$CMD:MON,CH:2,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT4": {
            "SET": "$CMD:SET,CH:3,PAR:{}",
            "QUERY": "$CMD:MON,CH:3,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT5": {
            "SET": "$CMD:SET,CH:4,PAR:{}",
            "QUERY": "$CMD:MON,CH:4,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT6": {
            "SET": "$CMD:SET,CH:5,PAR:{}",
            "QUERY": "$CMD:MON,CH:5,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT7": {
            "SET": "$CMD:SET,CH:6,PAR:{}",
            "QUERY": "$CMD:MON,CH:6,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "OUTPUT8": {
            "SET": "$CMD:SET,CH:7,PAR:{}",
            "QUERY": "$CMD:MON,CH:7,PAR:STAT",
            "verifier": is_in("ON", "OFF"),
        },
        "RAMP_UP": {
            "SET": "$CMD:SET,CH:8,PAR:RUP,VAL:{}",
            "QUERY": "$CMD:MON,CH:8,PAR:RUP",
            "verifier": is_integer(),
        },
        "RAMP_DOWN": {
            "SET": "$CMD:SET,CH:8,PAR:RDWN,VAL:{}",
            "QUERY": "$CMD:MON,CH:8,PAR:RDWN",
            "verifier": is_integer(),
        },
        "POWER_DOWN": {
            "SET": "$CMD:SET,CH:8,PAR:PDWN,VAL:{}",
            "QUERY": "$CMD:MON,CH:8,PAR:PDWN",
            "verifier": is_in("RAMP", "KILL"),
        },
    }
    SETTINGS["COMPLIANCE_CURRENT"] = SETTINGS["CURRENT"]
    SETTINGS["STATUS_RAW"] = SETTINGS["STATUS"]
    for i in range(OUTPUTS):
        SETTINGS[f"COMPLIANCE_CURRENT{i+1}"] = SETTINGS[f"CURRENT{i+1}"]
        SETTINGS[f"STATUS_RAW{i+1}"] = SETTINGS[f"STATUS{i+1}"]

    STATUS_TRANSLATION = [
        "ON",
        "Ramping Up",
        "Ramping Down",
        "OVC",
        "OVV",
        "UNV",
        "Tripped",
        "OVP",
        "high temperature",
        "OVT",
        "Switch KILL",
        "Interlock",
        "Switch OFF",
        "Generic Fail",
    ]

    default_step_size = 10

    class PowerChannel(Instrument.PowerChannel):
        # Needs to be adapted to status object as soon as implemented
        # Currently points to state

        @property
        def status(self):
            return self._instrument.query_channel("OUTPUT", self._channel)

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
        def current_limit(self):
            return self._instrument.query_channel("CURRENT", self._channel)

        @current_limit.setter
        def current_limit(self, value):
            self._instrument.set_channel("CURRENT", self._channel, value)

        @property
        def voltage_limit(self):
            return self._instrument.query_channel("VOLTAGE", self._channel)

        @voltage_limit.setter
        def voltage_limit(self, value):
            self._instrument.set_channel("VOLTAGE", self._channel, value)

        @property
        def measure_voltage(self):
            return self._measure_voltage

        @property
        def measure_current(self):
            return self._measure_current

        def sweep(
            self,
            target_value,
            delay=1,
            step_size=None,
            n_steps=None,
            measure=False,
            set_property="voltage",
            measure_function=None,
            measure_args=None,
            measure_fields=None,
            log_function=None,
            spacing=17,
            execute_each_step=None,
            break_loop=None,
        ):
            def check_voltage():
                i = 0
                while (abs(self.voltage - self.measure_voltage.value) > 2) and i < 10:
                    time.sleep(1)
                    i += 1

            rval = super().sweep(
                target_value,
                delay=delay,
                step_size=step_size,
                n_steps=n_steps,
                measure=measure,
                set_property="voltage",
                measure_function=measure_function,
                measure_args=measure_args,
                measure_fields=measure_fields,
                log_function=log_function,
                spacing=spacing,
                execute_each_step=check_voltage,
                break_loop=break_loop,
            )

            time.sleep(2)
            return rval

    class MeasureChannel(Instrument.MeasureChannel):
        @property
        def status(self):
            return int(self._instrument.query_channel("STATUS_RAW", self._channel))

        @property
        def status_verbose(self):
            return self._instrument.query_channel("STATUS", self._channel)

        @property
        def value(self):
            return self._instrument.query_channel(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type),
                self._channel,
            )

    def __init__(self, resource="ASRL12::INSTR", outputs=8, sim=False):
        super().__init__(resource, sim=sim)
        self._outputs = outputs

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
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
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    @property
    def outputs(self):
        return range(1, self._outputs + 1)

    def validate_channel(self, channel, raise_exception=True):
        if int(channel) in self.outputs:
            return True
        else:
            if raise_exception:
                raise ChannelError(
                    f"Channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    def set_channel(self, setting, channel, *value, **kwargs):
        self.validate_channel(int(channel), raise_exception=True)
        return self.set(f"{setting}{channel}", *value, **kwargs)

    def query_channel(self, setting, channel, **kwargs):
        self.validate_channel(int(channel), raise_exception=True)
        return self.query(f"{setting}{channel}", **kwargs)

    @acquire_lock()
    @retry_on_fail_visa()
    def set(self, setting, *value, check_readback_only=False):
        channel = ""
        if any(char.isdigit() for char in setting):
            channel = setting[-1]
            setting = setting[:-1]

        value = value[0]

        # Converting Ampere to uA
        if setting in ["CURRENT", "COMPLIANCE_CURRENT"]:
            if value >= 3e-3 or value < 1e-6:
                raise ValueError(
                    "Current for CaenDT8033N needs to be in range [1e-6, 3e-3]"
                )
            value = value * 1e6

        # Converting Voltage to positive value for communication
        if setting == "VOLTAGE":
            if value > 0 or value < -4000:
                raise ValueError(
                    "Voltage for CaenDT8033N needs to be in range [-4000, 0]"
                )
            else:
                value = abs(value)

        # Unification of On/Off commands
        if setting == "OUTPUT":
            if value == 0:
                value = "OFF"
            if value == 1:
                value = "ON"

        rval = super().set(
            f"{setting}{channel}",
            value,
            check_readback_only=check_readback_only,
            no_lock=True,
            attempts=1,
        )

        return rval

    @acquire_lock()
    @retry_on_fail_visa()
    def query(self, setting, *params):
        rval = super().query(setting, *params, no_lock=True, attempts=1)

        if isinstance(rval, str):
            if rval == "#CMD:OK":
                return True
            elif "#CMD:OK,VAL:" in rval:
                rval = rval[rval.find("VAL:") + 4 :]
            else:
                raise ValueError

        values = rval.split(";")

        if any(char.isdigit() for char in setting):
            setting = setting[:-1]

        for i, value in enumerate(values):
            # Converting to Ampere
            if setting in ["CURRENT", "OUTPUT_CURRENT", "COMPLIANCE_CURRENT"]:
                values[i] = round(float(value) * 1e-6, 8)

            elif setting in ["OUTPUT", "STATUS"]:
                value = int(value)

                if setting == "OUTPUT":
                    values[i] = value % 2
                else:
                    value_str = ""
                    binary = bin(value)[2:].zfill(14)

                    if binary[-1] == "1":
                        value_str += "ON"
                    else:
                        value_str += "OFF"

                    for k in range(1, 14):
                        if binary[-k - 1] == "1":
                            value_str += " - " + self.STATUS_TRANSLATION[k]

                    values[i] = value_str

            elif setting in ["VOLTAGE", "OUTPUT_VOLTAGE"]:
                values[i] = -float(value)
                if float(value) == 0:
                    values[i] = 0

            elif setting in ["IDENTIFIER", "POWER_DOWN"]:
                values[i] = value

            else:
                values[i] = float(value)

        if len(values) > 1:
            return values
        else:
            return values[0]

    def identify(self, **kwargs):
        return self.query("IDENTIFIER", **kwargs)

    def off(self, channel, **kwargs):
        if channel == "ALL":
            ret = []
            for i in self.outputs:
                ret.append(self.set_channel("OUTPUT", i, "OFF"))
            time.sleep(0.1)
            return ret
        else:
            self.validate_channel(int(channel), raise_exception=True)
            return self.set_channel("OUTPUT", channel, "OFF", **kwargs)

    @acquire_lock()
    def on(self, channel):
        if channel == "ALL":
            ret = []
            for i in self.outputs:
                ret.append(self.set_channel("OUTPUT", i, "ON", no_lock=True))
            time.sleep(0.1)
            return ret
        else:
            self.validate_channel(int(channel), raise_exception=True)
            ret = self.set_channel("OUTPUT", int(channel), "ON", no_lock=True)
            time.sleep(0.1)
            return ret

    def status(self, channel, **kwargs):
        if channel == "ALL":
            return self.query("STATUS", **kwargs)
        else:
            self.validate_channel(int(channel), raise_exception=True)
            return self.query_channel("STATUS", int(channel), **kwargs)

    def state(self, channel, **kwargs):
        if channel == "ALL":
            return self.query("OUTPUT", **kwargs)
        else:
            self.validate_channel(int(channel), raise_exception=True)
            return int(self.query_channel("OUTPUT", int(channel), **kwargs))

    @acquire_lock()
    def measure(self, channel="ALL"):
        if channel == "ALL":
            ret = []
            for i in self.outputs:
                ret.append(self.query_channel("OUTPUT_VOLTAGE", int(i), no_lock=True))
                ret.append(self.query_channel("OUTPUT_CURRENT", int(i), no_lock=True))
            return ret
        else:
            self.validate_channel(int(channel), raise_exception=True)
            return (
                self.query_channel("OUTPUT_VOLTAGE", int(channel), no_lock=True),
                self.query_channel("OUTPUT_CURRENT", int(channel), no_lock=True),
            )

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

    def sweep_print_header(self, measured_unit=None):
        """Returns the header for print-outs, when doing a sweep. Helper function of
        instrument.sweep()

        :param measured_unit: not needed here. Is used in some cases, where a different
            device does the measurement than the sweep.
        """
        return (
            "Time",
            "V1(V)",
            "C1(A)",
            "V2(V)",
            "C2(A)",
            "V3(V)",
            "C3(A)",
            "V4(V)",
            "C4(A)",
            "V5(V)",
            "C5(A)",
            "V6(V)",
            "C6(A)",
            "V7(V)",
            "C7(A)",
            "V8(V)",
            "C8(A)",
        )

    def publish(self, client, line, channel="ALL", topic=None):
        """Publish data line (from Monitoring) to MQTT.

        TODO: don't reparse line here; instead pass/access raw data?

        :param client: MQTTClient object.
        :param line: logfile line from MonitoringLogger.
        """
        if line.startswith("#"):
            return
        topic = topic or self._resource[9:-7]
        values = line.split("\t")
        if channel == "ALL":
            for i in range(1, len(values), 2):
                client.publish(
                    f"{topic}/voltage{i//2+1}",
                    f"{topic} voltage{i//2+1}={float(values[i].strip())}",
                )
                client.publish(
                    f"{topic}/current{i//2+1}",
                    f"{topic} current{i//2+1}={float(values[i+1].strip())}",
                )
        else:
            self.validate_channel(int(channel))
            client.publish(
                f"{topic}/voltage{channel}",
                f"{topic} voltage{channel}={float(values[1].strip())}",
            )
            client.publish(
                f"{topic}/current{channel}",
                f"{topic} current{channel}={float(values[2].strip())}",
            )
