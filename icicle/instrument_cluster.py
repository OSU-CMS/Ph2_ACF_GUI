"""InstrumentCluster class wrapping HV, LV and AdcBoard for multiple modules.

This module also contains the `DummyInstrument`, `DummyFunction` and
`InstrumentNotInstantiated` classes, and `MissingRequiredInstrumentError` and
`BadStatusForOperationError` exceptions.
"""

import logging
import time

from .instrument import Instrument, acquire_lock
from .relay_board import RelayBoard
from .adc_board import AdcBoard
from .psi_coldbox import PSIColdbox
from .keithley2410 import Keithley2410

logger = logging.getLogger(__name__)


class InstrumentNotInstantiated:
    """Filler class to replace response from a missing instrument in the cluster."""

    def __str__(self):
        return "InstrumentNotInstantiated"

    def __repr__(self):
        return str(self)


class DummyInstrument:
    """Filler class to replace an instrument in the cluster that has not been provided
    or instantiated.

    All calls to any possible function should return an
    InstrumentNotInstantiated
    object, except init, enter, exit...
    """

    class DummyFunction(InstrumentNotInstantiated):
        def __init__(self, instrument, name):
            self._instrument = instrument
            self._name = name

        def __call__(self, *args, **kwargs):
            argstring = ", ".join([str(a) for a in args])
            kwargstring = ", ".join(
                ["{key} : {value}" for key, value in kwargs.items()]
            )
            logger.debug(
                f"Instrument {self._instrument} not provided - command "
                f"{self._name} ({argstring}, {kwargstring}) ignored."
            )

            return InstrumentNotInstantiated()

        def __str__(self):
            return (
                f'Instrument "{self._instrument}" not provided - value '
                f"{self._name} returned as DummyFunction."
            )

        def __repr__(self):
            return str(self)

    def __init__(self, instrument):
        self._instrument = instrument

    def __getattr__(self, name):
        return type(self).DummyFunction(self._instrument, name)

    def __enter__(self, recover_attempt=False):
        return self

    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        pass


class MissingRequiredInstrumentError(RuntimeError):
    """Error to be thrown if an instrument required for the cluster is missing."""

    pass


class BadStatusForOperationError(RuntimeError):
    """Error to be thrown if an instrument has incorrect status for the requested
    operation."""

    pass


class InstrumentCluster(Instrument):
    """Instrument cluster class designed to provide seamless access to a cluster
    consisting of HV, LV, Relay Board and Multimeter, with required protections to
    ensure these are only used in safe configurations."""

    # Specify which package to use for which instrument class.
    package_map = {
        # HV
        "Keithley2410": "keithley2410",
        "CaenDT8033N": "caendt8033n",
        # LV
        "TTI": "tti",
        "TTITSX": "ttitsx",
        "HMP4040": "hmp4040",
        "DummyInstrument": "dummy",
        "KeysightE3633A": "keysighte3633a",
        # Relay Board
        "RelayBoard": "relay_board",
        "AdcBoard": "adc_board",
        # Multimeter
        "Keithley2000": "keithley2000",
        "HP34401A": "hp34401a",
        # Climate chamber
        "Binder": "binder_climate_chamber",
        # Coldbox
        "PSIColdbox": "psi_coldbox",
    }

    class RB_Wrapper:
        """Class that handles multiple RB MeasureChannels Designed as a wrapper
        containing one MeasureChannel() object per pin."""

        def __init__(self, parent_instrument, pins):
            if pins is None:
                pins = parent_instrument.get_pin_map()
            self._pins = {}
            for pin in pins:
                self._pins[pin] = parent_instrument.channel(
                    "MeasureChannel", pin, measure_type="voltage"
                )
            self._instrument = parent_instrument

        def __enter__(self):
            for pin in self._pins.values():
                pin.__enter__()

        def __exit__(self):
            for pin in self._pins.values():
                pin.__exit__()

        def query_all(self):
            res = {}
            for idx, pin in enumerate(self._pins.values()):
                res[pin.channel] = pin.read if idx > 0 else pin.value
            return res

        def query_pin(self, pin):
            return self._pins[pin].value

        @property
        def pin_map(self):
            return self._instrument.get_pin_map()

    def __init__(self, instrument_dict=None, channels_dict=None, resource="cluster1"):
        """"""
        super().__init__(resource=resource)

        self._instrument_dict = {}
        assert instrument_dict is not None
        # instantiate the instruments
        for name, device in instrument_dict.items():
            if "sim" in device:
                temp_object = Instrument.load_instrument_class(
                    InstrumentCluster.package_map[device["class"]],
                    device["class"],
                )(resource=device["resource"], sim=device["sim"])
            else:
                temp_object = Instrument.load_instrument_class(
                    InstrumentCluster.package_map[device["class"]],
                    device["class"],
                )(resource=device["resource"])
            temp_object.role = None
            for key, value in device.items():
                if not key == "class" and not key == "resource" and not key == "sim":
                    if hasattr(temp_object, key):
                        raise ValueError(
                            f"The provided variable {key} to the object "
                            + f"{type(temp_object)} does "
                            + "already exist. Please use another name"
                        )
                    if isinstance(value, str):
                        exec(f"temp_object.{key} = '{value}'")
                    else:
                        exec(f"temp_object.{key} = {value}")
            self._instrument_dict[name] = temp_object
        self._module_dict = {}
        assert channels_dict is not None
        for number, channel in channels_dict.items():
            temp_dict = {}
            if "lv" in channel.keys():
                instr_object = self._instrument_dict[channel["lv"]["instrument"]]
                instr_object.role = "lv"
                temp_dict["lv"] = instr_object.channel(
                    "PowerChannel", channel["lv"]["channel"]
                )
                temp_dict["lv"].instr_name = channel["lv"]["instrument"]
                assert hasattr(instr_object, "default_current")
                assert hasattr(instr_object, "default_voltage")
                temp_dict["lv"].default_voltage = float(instr_object.default_voltage)
                temp_dict["lv"].default_current = float(instr_object.default_current)
            else:
                temp_dict["lv"] = DummyInstrument("lv")
            if "hv" in channel.keys():
                instr_object = self._instrument_dict[channel["hv"]["instrument"]]
                instr_object.role = "hv"
                temp_dict["hv"] = instr_object.channel(
                    "PowerChannel", channel["hv"]["channel"]
                )
                temp_dict["hv"].instr_name = channel["hv"]["instrument"]
                assert hasattr(instr_object, "default_current")
                assert hasattr(instr_object, "default_voltage")
                temp_dict["hv"].default_voltage = float(instr_object.default_voltage)
                temp_dict["hv"].default_current = float(instr_object.default_current)
                temp_dict["hv"].default_delay = (
                    instr_object.default_delay
                    if hasattr(instr_object, "default_delay")
                    else 1
                )
                temp_dict["hv"].default_step_size = (
                    instr_object.default_step_size
                    if hasattr(instr_object, "default_step_size")
                    else 5
                )
            else:
                temp_dict["hv"] = DummyInstrument("hv")
            if "ab" in channel.keys():
                instr_object = self._instrument_dict[channel["ab"]["instrument"]]
                instr_object.role = "ab"
                # User must define "ab" as AdcBoard class
                if isinstance(instr_object, AdcBoard):
                    channels = (
                        channel["ab"]["channel"] if "channel" in channel["ab"] else None
                    )
                    temp_dict["ab"] = self.RB_Wrapper(
                        self._instrument_dict[channel["ab"]["instrument"]],
                        channels,
                    )
                # However, certain users can also perform
                # this measurement using a PSIColdbox
                elif isinstance(instr_object, PSIColdbox):
                    temp_dict["ab"] = instr_object.channel(
                        "MeasureChannel",
                        channel["ab"]["channel"],
                        measure_type="VOLTAGE",
                    )
            else:
                temp_dict["ab"] = DummyInstrument("ab")
            # Assigning Coldbox channels
            if "cb" in channel.keys():
                instr_object = self._instrument_dict[channel["cb"]["instrument"]]
                instr_object.role = "cb"
                temp_dict["cb"] = instr_object.channel(
                    "TemperatureChannel", channel["cb"]["channel"]
                )
                temp_dict["cb"].instr_name = channel["cb"]["instrument"]
                assert hasattr(instr_object, "default_temperature")
                temp_dict["cb"].default_temperature = float(
                    instr_object.default_temperature
                )
                temp_dict["cb"].default_step_size = (
                    instr_object.default_speed
                    if hasattr(instr_object, "default_speed")
                    else 0
                )
            else:
                temp_dict["cb"] = DummyInstrument("cb")

            self._module_dict[number] = temp_dict

        self._climate_chamber = (
            self._instrument_dict["climate_chamber"]
            if "climate_chamber" in self._instrument_dict.keys()
            else DummyInstrument("climate_chamber")
        )

    def get_modules(self):
        """Getter method to get the modules (channels to each module in a dictionary).

        :return: Module dictionary
        """
        return self._module_dict

    def get_instruments(self):
        """Getter method to get the instruments used.

        :return: instrument dictionary
        """
        return self._instrument_dict

    def get_hv(self):
        """Returns list of hv instruments connected.

        :return: list of instruments that assume the role of HV in some channel
            (warning: changes to direct instruments may not be contained to only active
            channels!)
        """
        hv_list = [x for x in self._instrument_dict.values() if x.role == "hv"]
        return hv_list

    def get_lv(self):
        """Returns list of LV instruments connected.

        :return: list of instruments that assume the role of LV in some channel
            (warning: changes to direct instruments may not be contained to only active
            channels!)
        """
        lv_list = [x for x in self._instrument_dict.values() if x.role == "lv"]
        return lv_list

    def get_cb(self):
        """Returns list of Coldbox instruments connected.

        :return: list of instruments that assume the role of coldbox in some channel
            (warning: changes to direct instruments may not be contained to only active
            channels!)
        """
        cb_list = [x for x in self._instrument_dict.values() if x.role == "cb"]
        return cb_list

    def climate_chamber(self):
        """Getter method for the climate chamber.

        :return: climate chamber object
        """
        return self._climate_chamber

    def sleep(self, delay):
        """Utility function to allow sleep to return True.

        :param delay: Sleep time in seconds.
        """
        time.sleep(delay)
        return True

    def assert_status(
        self,
        status_check,
        required_status,
        operation,
        problem,
        proposed_solution,
        solution=None,
    ):
        """Assert that check_status returns True. If not, the user will be informed
        about.

        :param status_check: function to check the status of the system right now
        :param current_status: the status the system has right now
        :param required_status: the required state
        :param operation: the operation that was performed
        :param problem: the problem that was encountered
        :param proposed_solution: How to solve the problem
        :param solution: a function to run, if the problem can be solved automatically.
            if there is no proposed solution, the programm will be terminated.
        """
        if isinstance(required_status, str):
            required_status = int(required_status)
        current_status = status_check()

        if isinstance(current_status, InstrumentNotInstantiated):
            return True
        if isinstance(required_status, InstrumentNotInstantiated):
            return True

        current_status = int(current_status)
        if not current_status == required_status:
            if solution is not None:
                y_n = input(
                    "\033[1m\033[91m[Attention] "
                    "\033[0m Bad Status encountered during runtime\n"
                    f"Operation: {operation}\n"
                    f"Problem: {problem}\n"
                    f"Proposed Solution: {proposed_solution}\n"
                    "\033[1m\033[92m[Y]\033[0m to continue "
                    "with proposed solution,\n"
                    "\033[1m\033[91m[N]\033[0m to quit. \n >>>> "
                )
                if y_n == "y" or y_n == "Y":
                    solution()
                    if int(status_check()) == required_status:
                        return True
                    else:
                        input(
                            f"Attempted Solution: {proposed_solution}\n"
                            "The attempted "
                            "solution did not work. Press Enter to terminate."
                            "\n >>>>"
                        )
            else:
                input(
                    f"\033[1m\033[91m[Attention] \033[0m "
                    "Bad Status encountered during "
                    f"runtime \nOperation: {operation} \nProblem: {problem} \n"
                    f"Proposed Solution: {proposed_solution} \n"
                    "Press Enter to terminate the programm. \n >>>>"
                )
            raise BadStatusForOperationError(
                f"Operation: {operation}, Problem: {problem}, "
                f"Proposed Solution: {proposed_solution}"
            )
        else:
            return True

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Begin context.

        Enters context on LV, HV, Relay Board, Multimeter in order, hence initialising
        these instruments. Sets HV compliance current immediately on startup.
        """
        for instrument in self._instrument_dict.values():
            instrument.__enter__(recover_attempt=recover_attempt)
            if isinstance(instrument, PSIColdbox):
                logger.info("Loading PSI coldbox calibration")
                instrument.load_variables_from_icicle(0)
                instrument.set("MODE", 0, 0)
        super().__enter__(recover_attempt=recover_attempt, no_lock=True)

        for _, module in self._module_dict.items():
            for channel in module.values():
                channel.__enter__()
            self.hv_set_ocp_module(module, module["hv"].default_current, no_lock=True)
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Ends context. Leaves context on Multimeter, Relay Board, HV, LV in order,
        hence closing these instruments.

        :param exception_type: Exception type thrown causing close, or None.
        :param exception_value: Exception thrown causing close, or None.
        :param traceback: Exception traceback.
        :param recover_attempt: Whether this is a recovery attempt when instrument
            fails/disconnects (i.e. from @retry_on_fail).
        """
        # if self._turn_off_on_close: -> Keep this in mind
        #     self.off(no_lock=True)
        self.off(no_lock=True)
        for instrument in self._instrument_dict.values():
            instrument.__exit__(recover_attempt=recover_attempt)
        super().__exit__(recover_attempt=recover_attempt, no_lock=True)

    def remove_instrument(self, instrument_type):
        """Removes an instrument type from all modules.

        :param instrument_type: type of instrument to be removed
        """
        for module in self._module_dict.values():
            self.remove_instrument_module(self, module, instrument_type)

    def remove_instrument_module(self, module, instrument_type):
        """Remove an instrument from one module.

        :param module: module from which the instrument is to be removed
        :param instrument_type: type of instrument to be removed
        """

        if instrument_type not in module.values():
            raise ValueError("No valid instrument type.")
        if not isinstance(module[instrument_type], DummyInstrument):
            module[instrument_type].__exit__()
            module[instrument_type] = DummyInstrument("dummy")

    def reset(self, *args, **kwargs):
        """Resets all instruments.

        :param no_lock: Do not acquire lock for subinstruments. (@acquire_lock)
        :param attempts: Retry attempts for each subinstrument. (@retry_on_fail)
            Asserts, that HV and LV are turned off. If not, asks to turn both off.
        """
        self.off()  # Turning off everything before resetting
        return all([x.reset(*args, **kwargs) for x in self._instrument_dict.values()])

    @acquire_lock()
    def relay_pin_module(self, module, pin="OFF"):
        """Set relay pin.

        To be figured out:
        Current implementation is:
            One pin = One channel object
        Problem:
            We would have to create a new channel object
            every time we change the pin...
            Also instantiating a relay board is much more difficult
        """
        if not isinstance(self._instrument_dict(module["rb"].instrument, RelayBoard)):
            return True

        current_pin = module["rb"].value
        if current_pin == pin:
            return True

        """
        Asserts, that LV and HV are turned off. If not, asks to
        turn LV and HV off.
        """
        self.assert_status(
            lambda: module["lv"].state,
            0,
            operation="Changing relay board pin",
            problem="LV is active",
            proposed_solution="Turn off LV",
            solution=lambda: self._module_off(module["lv"]),
        )

        # sense_function = channel['dvm']['object'].query("SENSE_FUNCTION")
        # if (
        #    not isinstance(sense_function, InstrumentNotInstantiated)
        #    and "RES" in sense_function
        # ):
        #    self.assert_status(
        #        lambda: pin in ("OFF", "NTC"),
        #        True,
        #        operation="Changing relay board pin",
        #        problem="Multimeter is on resistance sensing mode",
        #        proposed_solution="Manually change multimeter mode",
        #    )

        # return channel['rb']['object'].set_pin(pin)
        return True

    # CURRENTLY NO SCAN USES THE MULTIMETER
    # TO BE INSTANTIATED AGAIN WHEN WE NEED THE RELAY BOARD AGAIN
    # @acquire_lock()
    # def multimeter_measure(
    #    self, measured_unit, repetitions=1, delay=None, cycles=1.0,
    #    clear_trace=True
    # ):
    #    """Measure using multimeter.

    #    Requires relay board to be connecting OFF or NTC if `measured_unit`
    #    is RES/FRES.

    #    :param measured_unit: Requested measurement type. Accepted are
    #        `VOLT:DC`, `VOLT:AC`, `CURR:DC`, `CURR:AC`, `RES`, `FRES`, `SLDO`
    #    :param repetitions: How many measurements to average. Defaults to 1.
    #    :param delay: Delay between repetitions (seconds).
    #    :param cycles: How many integration cycles per measurement.
    #        Defaults to 1.0.
    #    :param clear_trace: Whether to send 'clear trace' command
    #        after measurement. Default to True.

    #    :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
    #    """
    #    assert measured_unit in (
    #        "VOLT:DC",
    #        "VOLT:AC",
    #        "CURR:DC",
    #        "CURR:AC",
    #        "RES",
    #        "FRES",
    #        "SLDO",
    #    )

    #    if measured_unit in ("RES", "FRES"):
    #        self.assert_status(
    #            lambda: self._relay_board.query_pin() in ("OFF", "NTC"),
    #            True,
    #            operation="Measurement with Multimeter",
    #            problem="Relay board has pin other than OFF"
    #                    "or NTC connected",
    #            proposed_solution="Manually remove PIN",
    #        )
    #    if measured_unit == "SLDO":
    #        measured_unit = "VOLT:DC"
    #        # assert(self._lv is not None)
    #        measurement = self._lv.measure(self._default_lv_channel,
    #                      no_lock=True)
    #        measurement += tuple(
    #            self._multimeter.measure(
    #                measured_unit,
    #                repetitions=repetitions,
    #                delay=delay,
    #                cycles=cycles,
    #                clear_trace=clear_trace,
    #            )
    #        )
    #    else:
    #        measurement = self._multimeter.measure(
    #            measured_unit,
    #            repetitions=repetitions,
    #            delay=delay,
    #            cycles=cycles,
    #            clear_trace=clear_trace,
    #        )
    #    return measurement

    def lv_on(self, voltage=None, current=None):
        """Turn on all LV.

        Loops through all modules and turns them on

        :param voltage: Voltage to set on LV.
        :param current: Current to set on LV.
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(self.lv_on_module(module, voltage, current))
        return all(ret_all_modules)

    @acquire_lock()
    def lv_on_module(self, module, voltage=None, current=None):
        """Turn on LV.

        Requires HV to be off.

        :param module: For which module the LV should be turned on.
        :param voltage: Voltage to set on LV. Defaults on default_hv_voltage as
            specified on initialisation if None.
        :param current: Current to set on LV. Defaults on default_hv_current as
            specified on initialisation if None.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        volt = voltage if voltage is not None else module["lv"].default_voltage
        curr = current if current is not None else module["lv"].default_current

        module["lv"].voltage = volt
        module["lv"].current = curr

        if module["lv"].state is True:
            return True

        """
        Asserts, that HV is off. If not, asks to turn on LV anyway.
        """
        self.assert_status(
            lambda: module["lv"].state,
            module["hv"].state,
            operation="Turning on LV",
            problem="HV is active",
            proposed_solution="Turn on LV anyway",
            solution=lambda: self._module_on(module["lv"]),
        )

        module["lv"].state = True

        return module["lv"].state

    def lv_off(self, no_lock=False):
        """Turn off all LV.

        Loops through all modules and turns them off
        """

        ret = []
        for module in self._module_dict.values():
            ret.append(self.lv_off_module(module, no_lock=no_lock))
        return all(ret)

    @acquire_lock()
    def lv_off_module(self, module):
        """Turn off LV.

        Requires HV to be off.

        :param module: The module for which the LV is to be turned off
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        if module["lv"].state is False:
            # Already off
            return True

        """
        Asserts, that HV is off. If not, asks to turn HV off.
        """
        self.assert_status(
            lambda: module["hv"].state,
            0,
            operation="Turning off LV",
            problem="HV is active",
            proposed_solution="Turn off HV",
            solution=lambda: self._module_off(module["hv"]),
        )
        module["lv"].state = False

        return module["lv"].state is False

    def lv_sweep(self, target, delay, step_size, measure=False, **kwargs):
        """Perform sweep for all LV. Does a loop through all devices (not performing
        simultaneously currently)

        :param what: name of setting that `set()` call should target.
        :param target: value of setting to sweep to (from current).
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param lv_channel: Which LV channel is in use. Defaults to default_lv_channel as
            specified on initialisation if None.
        :param measure: whether to measure at each step of sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(
                self.lv_sweep_module(
                    module, target, delay, step_size, measure, **kwargs
                )
            )
        return all(ret_all_modules)

    @acquire_lock()
    def lv_sweep_module(
        self, module, target, delay, step_size, measure=False, **kwargs
    ):
        """Sweep/Ramp LV from current to target voltage.

        Requires HV to be off.

        :param module: The module for which the lv should be swept
        :param what: name of setting that `set()` call should target.
        :param target: value of setting to sweep to (from current).
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param lv_channel: Which LV channel is in use. Defaults to default_lv_channel as
            specified on initialisation if None.
        :param measure: whether to measure at each step of sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """

        """
        Asserts that HV is turned off. If not, asks to turn HV off.
        """
        self.assert_status(
            lambda: module["hv"].state,
            0,
            operation="Ramping LV",
            problem="HV is active",
            proposed_solution="Turn off HV",
            solution=lambda: self._module_off(module["hv"]),
        )

        module["lv"].state = True
        ret = module["lv"].sweep(
            target,
            delay,
            step_size,
            measure=measure,
            log_function=logger.info,
            **kwargs,
        )
        if abs(target) < 1e-12:
            module["lv"].state = False
        return ret

    def hv_on(self, voltage=None, delay=None, step_size=None, measure=True, **kwargs):
        """Turn on the HV for all modules.

        Loops through all modules and turns them on

        :param voltage: Voltage to set on HV. Defaults on default_hv_voltage as
            specified on initialisation if None.
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param measure: whether to measure at each step of sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            if module["hv"].state is not True:
                ret_all_modules.append(
                    self.hv_on_module(
                        module, voltage, delay, step_size, measure, **kwargs
                    )
                )
        return ret_all_modules

    @acquire_lock()
    def hv_on_module(
        self,
        module,
        voltage=None,
        delay=None,
        step_size=None,
        measure=True,
        **kwargs,
    ):
        """Turn on HV and ramps to voltage.

        Requires LV to be on.

        :param module: for which module the HV should be turned on
        :param voltage: Voltage to set on HV. Defaults on default_hv_voltage as
            specified on initialisation if None.
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param measure: whether to measure at each step of sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        if voltage is None:
            voltage = module["hv"].default_voltage
        if delay is None:
            delay = module["hv"].default_delay
        if step_size is None:
            step_size = module["hv"].default_step_size

        """Check, if HV is already on and that Voltage is at 0
        """
        if module["hv"].state is True or isinstance(module["hv"], DummyInstrument):
            return True
        if abs(module["hv"].voltage) > 10:
            module["hv"].voltage = 0

        """Asserts that LV is turned on.

        If not asks to turn on LV
        """
        self.assert_status(
            lambda: module["lv"].state,
            1,
            operation="Turning on HV",
            problem="LV is not active",
            proposed_solution="Turn on LV",
            solution=lambda: self.lv_on_module(module),
        )

        # Keithley needs to set the voltage range.
        # Might be worth to transfer this to the keithley class
        if isinstance(module["hv"].instrument, Keithley2410):
            module["hv"].instrument.set("VOLTAGE_RANGE", module["hv"].default_voltage)

        module["hv"].state = True
        return module["hv"].sweep(
            target_value=voltage,
            set_property="voltage",
            delay=delay,
            step_size=step_size,
            measure=measure,
            log_function=logger.info,
            **kwargs,
        )

    def hv_set(
        self,
        voltage=None,
        delay=None,
        step_size=None,
        measure=False,
        measure_args=None,
        **kwargs,
    ):
        """Sets the voltage of the HV for each module.

        Loops through each module and sets the HV.

        :param voltage: voltage to be set for the HV
        :param delay: delay between steps as the HV has to be swept
        :param step_size: step size between steps in the sweep
        :param measure - if this is True, the hv will measure the
            output and voltage at each step.
        :param measure_args - measurement parameters in case,
            measurement is true. e.g. averages.
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(
                self.hv_set_module(
                    module,
                    voltage,
                    delay,
                    step_size,
                    measure,
                    measure_args,
                    **kwargs,
                )
            )
        return ret_all_modules

    def hv_set_module(
        self,
        module,
        voltage=None,
        delay=None,
        step_size=None,
        measure=False,
        measure_args=None,
        **kwargs,
    ):
        """Change HV voltage output for a single module.

        Requires LV to be on.

        :param voltage: voltage to be set for the HV
        :param delay: delay between steps as the HV has to be swept
        :param step_size: step size between steps in the sweep
        :param measure - if this is True, the hv will measure the
            output and voltage at each step.
        :param measure_args - measurement parameters in case,
            measurement is true. e.g. averages.
        """
        if voltage is None:
            voltage = module["hv"].default_voltage
        if delay is None:
            delay = module["hv"].default_delay
        if step_size is None:
            step_size = module["hv"].default_step_size

        """
        Asserts, that LV is turned on. If not, asks to turn on LV
        """

        self.assert_status(
            lambda: module["lv"].state,
            1,
            operation="Change HV Output",
            problem="LV is not active",
            proposed_solution="Turn on LV",
            solution=lambda: self.lv_on_module(module),
        )
        """Asserts, that if measure=False, also measure_args = false."""
        if measure is False:
            assert measure_args is None
            # a bit dodgy thing to not pass a non-existant measure_args
            measure_args = {"measured_unit": "V"}

        return module["hv"].sweep(
            target_value=voltage,
            delay=delay,
            step_size=step_size,
            measure=measure,
            measure_args=measure_args,
            log_function=logger.info,
        )

    def hv_off(self, delay=None, step_size=None, measure=True, **kwargs):
        """Ramps down and turns off HV for each module.

        Loops through each module and turns its HV off

        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param measure: whether to measure at each step of sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(
                self.hv_off_module(module, delay, step_size, measure, **kwargs)
            )
        return all(ret_all_modules)

    @acquire_lock()
    def hv_off_module(self, module, delay=None, step_size=None, measure=True, **kwargs):
        """Ramps HV to zero and turns off.

        Requires LV to be on.

        :param module: the module for which HV is to be turned off
        :param delay: delay between sweep steps (in seconds)
        :param step_size: step size between sweep steps
        :param measure: whether to measure at each step of sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        if module["hv"].state is False or isinstance(module["hv"], DummyInstrument):
            # Already off or DummyInstrument
            return True
        if delay is None:
            delay = module["hv"].default_delay
        if step_size is None:
            step_size = module["hv"].default_step_size

        """
        Asserts, that LV is turned on. If not, asks to turn on LV
        """

        #  To be debugged. Somehow this does not work...
        # self.assert_status(
        #     lambda: module["lv"].state,
        #     1,
        #     operation="Ramping down HV",
        #     problem="LV is not active",
        #     proposed_solution="Turn on LV",
        #     solution=lambda: self._module_on(module["lv"]),
        # )
        if abs(module["hv"].voltage) > 10:
            ret = module["hv"].sweep(
                target_value=0.0,
                delay=delay,
                step_size=step_size,
                measure=measure,
                log_function=logger.info,
                **kwargs,
            )
        else:
            ret = True

        self.sleep(2)
        module["hv"].state = False
        self.sleep(2)
        return module["hv"].state is False and ret

    def hv_set_ocp(self, compliance_current):
        """Sets the compliance current for all modules HV Loops through all modules and
        sets a compliance current for the HV.

        :param compliance_current: Compliance in Amps.
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(self.hv_set_ocp_module(module, compliance_current))
        return all(ret_all_modules)

    @acquire_lock()
    def hv_set_ocp_module(self, module, compliance_current):
        """Set HV compliance current and corresponding sense current range.

        :param compliance_current: Compliance in Amps.
        """
        module["hv"].current_limit = compliance_current
        return module["hv"].current_limit == compliance_current

    def cb_on(self, temperature=None):
        """Turns on the coldbox for all channels.

        :param temperature: Temperature to set at startup. default_temperature if None
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(self.cb_on_module(module, temperature))
        return all(ret_all_modules)

    @acquire_lock()
    def cb_on_module(self, module, temperature):
        """Sets temperature and turns on one channel on the coldbox."""
        if isinstance(module["cb"], DummyInstrument):
            return True

        temp = (
            temperature if temperature is not None else module["cb"].default_temperature
        )

        module["cb"].temperature = temp

        if not module["cb"].state:
            module["cb"].instrument.set("FLUSH", 1)
            module["cb"].state = True

        return module["cb"].state

    def cb_off(self, no_lock=False):
        """Turn off all coldbox channels.

        Loops through all modules and turns the coldbox off
        """

        ret = []
        for module in self._module_dict.values():
            ret.append(self.cb_off_module(module, no_lock=no_lock))
        return all(ret)

    @acquire_lock()
    def cb_off_module(self, module):
        """Turn off coldbox.

        Requires LV to be off.

        :param module: The module for which the coldbox is to be turned off
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        if module["cb"].state is False or isinstance(module["cb"], DummyInstrument):
            # Already off
            return True

        """
        Asserts, that LV is off. If not, asks to turn LV off.
        """
        self.assert_status(
            lambda: module["lv"].state,
            0,
            operation="Turning off coldbox",
            problem="LV is active",
            proposed_solution="Turn off LV",
            solution=lambda: self._module_off(module["lv"]),
        )
        module["cb"].state = False

        return module["cb"].state is False

    def on(
        self,
        lv_voltage=None,
        lv_current=None,
        hv_voltage=None,
        hv_delay=None,
        hv_step_size=None,
        cb_temperature=None,
        measure=True,
        **kwargs,
    ):
        """Turn all instruments on (and ramp up) in correct order and with sensible
        delays for all modules.

        :param lv_voltage: Voltage to set on LV. Defaults on default_voltage as
            specified on initialisation if None.
        :param lv_current: Current to set on LV. Defaults on default_current as
            specified on initialisation if None.
        :param hv_voltage: Voltage to ramp to on HV. Defaults on default_voltage as
            specified on initialisation if None.
        :param hv_delay: delay between HV sweep steps (in seconds)
        :param hv_step_size: step size between HV sweep steps
        :param measure: whether to measure at each step of HV sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(
                self.on_module(
                    module,
                    lv_voltage,
                    lv_current,
                    hv_voltage,
                    hv_delay,
                    hv_step_size,
                    cb_temperature,
                    measure,
                    **kwargs,
                )
            )
        return all(ret_all_modules)

    @acquire_lock()
    def on_module(
        self,
        module,
        lv_voltage=None,
        lv_current=None,
        hv_voltage=None,
        hv_delay=None,
        hv_step_size=None,
        cb_temperature=None,
        measure=False,
    ):
        """Turn all instruments on (and ramp up) in correct order and with sensible
        delays.

        :param relay_pin: Pin, see list/dict in the corresponding RelayBoard class.
        :param lv_channel: Which LV channel is in use. Defaults to default_channel as
            specified on initialisation if None.
        :param lv_voltage: Voltage to set on LV. Defaults on default_voltage as
            specified on initialisation if None.
        :param lv_current: Current to set on LV. Defaults on default_current as
            specified on initialisation if None.
        :param hv_voltage: Voltage to ramp to on HV. Defaults on default_voltage as
            specified on initialisation if None.
        :param hv_delay: delay between HV sweep steps (in seconds)
        :param hv_step_size: step size between HV sweep steps
        :param measure: whether to measure at each step of HV sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        if lv_voltage is None:
            lv_voltage = module["lv"].default_voltage
        if lv_current is None:
            lv_current = module["lv"].default_current
        if hv_voltage is None:
            hv_voltage = module["hv"].default_voltage
        if hv_delay is None:
            hv_delay = module["hv"].default_delay
        if hv_step_size is None:
            hv_step_size = module["hv"].default_step_size
        if cb_temperature is None:
            cb_temperature = module["cb"].default_temperature

        ret = self.cb_on_module(module=module, temperature=cb_temperature, no_lock=True)
        ret2 = self.lv_on_module(
            module=module,
            voltage=lv_voltage,
            current=lv_current,
            no_lock=True,
        )
        ret3 = self.hv_on_module(
            module=module,
            voltage=hv_voltage,
            delay=hv_delay,
            step_size=hv_step_size,
            measure=measure,
            no_lock=True,  # This lock is on instrument_cluster
        )
        return ret and ret2 and ret3

    def off(self, hv_delay=None, hv_step_size=None, measure=True, **kwargs):
        """Turns off all the channels for all modules Loops through all modules and
        turns off their channels in the correct order.

        :param hv_delay: delay between HV sweep steps (in seconds)
        :param hv_step_size: step size between HV sweep steps
        :param measure: whether to measure at each step of HV sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        ret_all_modules = []
        for module in self._module_dict.values():
            ret_all_modules.append(
                self.off_module(module, hv_delay, hv_step_size, measure, **kwargs)
            )
        return all(ret_all_modules)

    @acquire_lock()
    def off_module(self, module, hv_delay=None, hv_step_size=None, measure=False):
        """Turn all instruments off (and ramp down) in correct order.

        :param hv_delay: delay between HV sweep steps (in seconds)
        :param hv_step_size: step size between HV sweep steps
        :param measure: whether to measure at each step of HV sweep.
        :param no_lock: Do not attempt to acquire lock. (@acquire_lock)
        """
        if hv_delay is None:
            hv_delay = module["hv"].default_delay
        if hv_step_size is None:
            hv_step_size = module["hv"].default_step_size

        ret = self.hv_off(
            delay=hv_delay,
            step_size=hv_step_size,
            measure=measure,
            no_lock=True,
        )
        # give CAEN DT8033N enough time to properly deactivate
        self.sleep(5)  # this should suppress "Bad status encountered" message
        self.lv_off(no_lock=True)
        self.sleep(1)
        return self.cb_off(no_lock=True) is not False and ret

    def status(self, *args, **kwargs):
        """Requests the output/pin status of all the channels connected to all the
        modules."""
        ret_dict = {}
        for key, module in self._module_dict.items():
            ret_dict[key] = self.status_module(module, *args, **kwargs)
        return ret_dict

    @acquire_lock()
    def status_module(self, module, *args, **kwargs):
        """Request output/pin status for HV, LV and Relay board.

        Returns all results as dictionary.
        """
        return {
            "hv": module["hv"].state,
            "lv": module["lv"].state,
            "relay_board": module["rb"].state,
        }

    def _module_on(self, channel):
        """To turn on a channel when assert_state is triggered.

        This should only be triggered, when the instrument state is bad.
        """
        channel.state = True
        return channel.state

    def _module_off(self, channel):
        """To turn off mainly the HV supply when the LV supply is supposed to be turned
        off.

        This should only be triggered, when the instrument state is bad.
        """
        if channel.state is False:
            return True
        channel.sweep(
            target_value=0.0,
            delay=channel.default_delay,
            step_size=channel.default_step_size,
            measure=False,
            log_function=None,
        )
        channel.state = False
        return channel.state is False

    def abort(self, *args, **kwargs):
        """Alias for off()."""
        self.off(*args, **kwargs)

    def open(self, *args, **kwargs):
        """Alias for __enter__()."""
        self.__enter__(*args, **kwargs)

    def close(self, *args, **kwargs):
        """Alias for __exit__()."""
        self.__exit__(*args, **kwargs)
