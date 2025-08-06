"""
Class to perform the SLDO curve scanning
"""

from PyQt5.QtCore import QThread, pyqtSignal, QObject
from Gui.python.logging_config import logger
from icicle.icicle.adc_board import ADCBoard
from icicle.icicle.relay_board import RelayBoard
from icicle.icicle.instrument_cluster import InstrumentNotInstantiated
import numpy as np
import math
import traceback


class SLDOCurveWorker(QThread):
    finishedSignal = pyqtSignal()
    progressSignal = pyqtSignal(str, float)
    stopSignal = pyqtSignal(str)
    measure = pyqtSignal(np.ndarray, str, str)

    def __init__(
        self,
        instrument_cluster=None,
        moduleType=None,
        target_current=10,
        step_size=0.2,
        delay=0.2,
        integration_cycles=1,
        starting_current=1,
        max_voltage=1.8,
        pin_list=[],
        execute_each_step=lambda: None,
        testhandler=None,

    ):
        super().__init__()
        self.instruments = instrument_cluster
        self.delay = delay
        self.target_current = target_current
        self.step_size = step_size
        self.integration_cycles = integration_cycles
        self.starting_current = starting_current
        self.max_voltage = max_voltage
        self.pin_list = pin_list
        self.exiting = False
        self.moduleType = moduleType
        self.testhandler = testhandler
        self.Nsteps = int((self.target_current - self.starting_current) / self.step_size)
        self.PIN_MAPPINGS = {
            "DEFAULT": ADCBoard.DEFAULT_PIN_MAP,
            "DOUBLE": {
                # 1: 'VDDA_ROC1',
                2: 'VDDA_ROC13', #ROC U1B
                # 3: 'VDDD_ROC2',
                # 4: 'VDDD_ROC3',
                # 5: 'VDDD_ROC1',
                6: 'VDDD_ROC13', #ROC U1B
                # 7: None,
                # 8: 'TP8', #VOFS IN
                # 9: None,
                10: 'TP10', #VIN
                #  11: None,
                12: "VDDA_ROC12", #ROC U1A
                # 13: "VDDA_ROC1",
                14: "VDDD_ROC12", #ROC U1A
                # 15: "VDDD_ROC1",
                # 16: 'TP7A', #VOFS OUT
                # 17: 'TP7B', #VOFS OUT
            },
            "QUAD": {
                1: "VDDA_ROC14", #ROC U1C
                2: "VDDA_ROC15", #ROC U1D
                3: "VDDD_ROC14", #ROC U1C
                4: "VDDD_ROC15", #ROC U1D
                # 5: 'TP7C', #VOFS OUT
                # 6: 'TP7D', #VOFS OUT
                # 7: None,
                # 8: 'TP8', #VOFS IN
                # 9: None,
                10: 'TP10', #VIN
                # 11: None,
                12: "VDDA_ROC12", #ROC U1A
                13: "VDDA_ROC13", #ROC U1B
                14: "VDDD_ROC12", #ROC U1A
                15: "VDDD_ROC13", #ROC U1B
                # 16: 'TP7A', #VOFS OUT
                # 17: 'TP7B', #VOFShv_off OUT
            },
        }
        self.LV_index = -1
        self.execute_each_step = execute_each_step

    def run(self) -> None:
        if "adc_board" in self.instruments._instrument_dict.keys():
            self.adc_board = self.instruments._instrument_dict["adc_board"]
            # Set the pin map based on the module type
            self.adc_board._pin_map = self.PIN_MAPPINGS[
                self.moduleType.split(" ")[-1].replace("1x2", "DOUBLE").upper()
            ]
            logger.info("Running with ADC.")
            self.runWithADC()
        else:
            logger.error(
                "You do not have instruments required to run an SLDOScan connected.\nYou must have an Adc Board or (Relay Board and Multimeter)."
            )

    def runWithADC(self) -> None:
        """
        Run thread that will ramp up the LV while measuring from the multimeter, then ramp down doing the same thing.
        Combine the two list of measurments and return that from the function.

        measure: emitted after VI curve of each pin contains array of MM measurements
        progress: emitted after each cycle of the LV sweep
        """
        # Initialize a list to store the results
        self.labels = []
        # Turn off instruments
        starting_voltages = [
            np.abs(getattr(module["hv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        self.instruments.hv_off(
            execute_each_step=lambda: self.execute_each_step(starting_voltages)
        )
        self.instruments.lv_off()
        self.adc_board.__enter__()
        logger.info("Turned off the LV and HV")

        self.instruments.lv_on(current=self.starting_current, voltage=self.max_voltage)
        # sweep from the starting current to the target current, preliminary data processing
        data_up = self.instruments.lv_sweep(
            target=self.target_current,
            delay=self.delay,
            step_size=self.step_size,
            measure=True,
            measure_function=self.measureADCup,
            measure_args={},
            set_property="current",
        )

        self.determineLVIndex(data_up)
        data_up = data_up[self.LV_index][1]

        # sweep from the target current to the starting current, preliminary data processing
        data_down = self.instruments.lv_sweep(
            target=self.starting_current,
            delay=self.delay,
            step_size=self.step_size,
            measure=True,
            measure_function=self.measureADCdown,
            measure_args={},
            set_property="current",
        )[self.LV_index][1]
        self.instruments.lv_off()

        # extract lv voltage and lv current
        Currents_Up = [res[5] for res in data_up]
        Currents_Down = [res[5] for res in data_down]

        #Indexing for pin voltages
        index = 6
        pin10index = index + list(self.adc_board._pin_map.keys()).index(10)
        LV_Voltage_Up = [res[pin10index] for res in data_up]
        LV_Voltage_Down = [res[pin10index] for res in data_down]
        
        ### The structure of self.testhandler.VDDDup[channel][chip] os that it is a list of dictionaries
        ### where the keys are the current and the values are the voltage read from the GADC.
        for channel in self.instruments._module_dict:
            for chip in self.testhandler.VDDDup[channel]:
                VDDDupData = np.array([list(self.testhandler.VDDDup[channel][chip].keys()), list(self.testhandler.VINDup[channel][chip].values()), list(self.testhandler.VDDDup[channel][chip].values())])
                VDDDdownData = np.array([list(self.testhandler.VDDDdown[channel][chip].keys()), list(self.testhandler.VINDdown[channel][chip].values()), list(self.testhandler.VDDDdown[channel][chip].values())])
                VDDAupData = np.array([list(self.testhandler.VDDAup[channel][chip].keys()), list(self.testhandler.VINAup[channel][chip].values()), list(self.testhandler.VDDAup[channel][chip].values())])
                VDDAdownData = np.array([list(self.testhandler.VDDAdown[channel][chip].keys()), list(self.testhandler.VINAdown[channel][chip].values()), list(self.testhandler.VDDAdown[channel][chip].values())])
                VDDDresults = np.concatenate((VDDDupData, VDDDdownData), axis=0)
                VDDAresults = np.concatenate((VDDAupData, VDDAdownData), axis=0)

                self.measure.emit(VDDDresults, "VDDD_ROC{0}".format(chip), "GADC")
                self.measure.emit(VDDAresults, "VDDA_ROC{0}".format(chip), "GADC")

        print("LV Voltages Up: {0}\nLV Voltages Down: {1}".format(
            LV_Voltage_Up, LV_Voltage_Down
        ))
        print("Pin map used: {0}".format(self.adc_board._pin_map))

        for name in self.adc_board._pin_map.values():
            if index != pin10index:
                print('the name of the pin is {0}'.format(name))
                ADC_Voltage_Up = [res[index] for res in data_up]
                result_up = np.array([Currents_Up, LV_Voltage_Up, ADC_Voltage_Up])
                ADC_Voltage_Down = [res[index] for res in data_down]
                result_down = np.array([Currents_Down, LV_Voltage_Down, ADC_Voltage_Down])
                print("ADC Voltages Up: {0}\nADC Voltages Down: {1}".format(
                    ADC_Voltage_Up, ADC_Voltage_Down))
                index += 1
            else:
                index += 1
                continue

            results = np.concatenate((result_up, result_down), axis=0)
            # 0:up current, 1:up lv voltage, 2: up adc voltage 3: down current, 4: down lv voltage, 5: down adc voltage
            # name needs to be a string with the format VDDA_ROC12
            self.measure.emit(results, name, "PROBE")

        self.finishedSignal.emit()

    def measureADCup(self, no_lock=True, *args, **kwargs):
        self.updateProgress(1)
        ls = []
        self.testhandler.GADC_execute_each_step(upOrDown='up',total_steps=self.Nsteps)
        for pin in self.adc_board._pin_map.keys():
            ls.append(self.adc_board.query_channel(pin))
        return ls

    def measureADCdown(self, no_lock=True, *args, **kwargs):
        self.updateProgress(1)
        ls = []
        self.testhandler.GADC_execute_each_step(upOrDown='down',total_steps=self.Nsteps)
        for pin in self.adc_board._pin_map.keys():
            ls.append(self.adc_board.query_channel(pin))
        return ls

    def measureMM(self, no_lock=True, *args, **kwargs):
        self.updateProgress(len(self.pin_list))
        return [
            self.multimeter.measure(
                what="VOLT:DC", cycles=self.integration_cycles, no_lock=no_lock
            )
        ]

    def updateProgress(self, nLoops):
        num_steps = (
            math.ceil(abs(self.target_current - self.starting_current) / self.step_size)
            + 1
        )
        num_sweeps = (
            2
            * nLoops
            * sum(1 for key in self.instruments._instrument_dict.keys() if "lv" in key)
        )  # number of LVs connected, each LV gets its own sweep
        percentStep = 100 / (num_steps * num_sweeps)
        self.progressSignal.emit("SLDOScan", percentStep)

    def determineLVIndex(self, data):
        if self.LV_index == -1:
            for i, (_, ps) in enumerate(data):
                if any(abs(reading[5]) > 1e-4 for reading in ps):
                    self.LV_index = i
                    break
            if self.LV_index == -1:
                self.stopSignal.emit(
                    "No current was read from any LV power supply. Please ensure that a module is connected."
                )

    def setRelayPin(self, pin):
        if not isinstance(self.relayboard, RelayBoard):
            return True

        current_pin = self.relayboard.query_pin()
        if current_pin == pin:
            return True

        # Ensures that LV and HV are off
        for module in self.instruments._module_dict.values():
            self.instruments.assert_status(
                lambda: module["lv"].state,
                0,
                operation="Changing relay board pin",
                problem="LV is active",
                proposed_solution="Turn off LV",
                solution=lambda: self.instruments._module_off(module["lv"]),
            )

        sense_function = self.multimeter.query("SENSE_FUNCTION")
        if (
            not isinstance(sense_function, InstrumentNotInstantiated)
            and "RES" in sense_function
        ):
            self.instruments.assert_status(
                lambda: pin in ("OFF", "NTC"),
                True,
                operation="Changing relay board pin",
                problem="Multimeter is on resistance sensing mode",
                proposed_solution="Manually change multimeter mode",
            )

        return self.relayboard.set_pin(pin)


class SLDOCurveHandler(QObject):
    finishedSignal = pyqtSignal()
    makeSLDOplotSignal = pyqtSignal(np.ndarray, str, str)
    progressSignal = pyqtSignal(str, float)
    abortSignal = pyqtSignal()
    # measureSignal = pyqtSignal(str, object)

    def __init__(
        self,
        instrument_cluster,
        moduleType,
        end_current,
        starting_current,
        step_size,
        voltage_limit,
        execute_each_step,
        testhandler,
    ):
        super(SLDOCurveHandler, self).__init__()
        self.instruments = instrument_cluster
        self.end_current = end_current
        self.starting_current = starting_current
        self.step_size = step_size
        self.voltage_limit = voltage_limit
        self.execute_each_step = execute_each_step
        self.testhandler = testhandler
        self.test = SLDOCurveWorker(
            self.instruments,
            moduleType=moduleType,
            target_current=self.end_current,
            step_size=self.step_size,
            starting_current=self.starting_current,
            max_voltage=self.voltage_limit,
            execute_each_step=self.execute_each_step,
            testhandler=self.testhandler,
        )
        self.test.measure.connect(self.makePlots)
        self.test.progressSignal.connect(self.transmitProgress)
        self.test.finishedSignal.connect(self.finish)
        self.test.stopSignal.connect(self.stop)

    # This should take the list of results and make plots.  I think we should maybe move this to the TestHandler.
    def makePlots(self, total_result, pin, method):
        self.makeSLDOplotSignal.emit(total_result, pin, method)

    def SLDOScan(self):
        if not self.instruments:
            return
        self.test.start()

    def transmitProgress(self, measurementType, percentStep):
        self.progressSignal.emit(measurementType, percentStep)

    def finish(self):
        starting_voltages = [
            np.abs(getattr(module["hv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        self.instruments.hv_off(
            execute_each_step=lambda: self.execute_each_step(starting_voltages)
        )
        self.instruments.lv_off()
        self.finishedSignal.emit()

    def stop(self, reason=None):
        # Should only have a reason internally where the abort signal is necessary.
        # External calls can handle abort themselves. Avoiding accidental recursion.
        if reason:
            print(f"Aborting SLDO Scan. Reason: {reason}")
            try:
                starting_voltages = [
                    np.abs(getattr(module["hv"], "voltage"))
                    for module in self.instruments._module_dict.values()
                ]
                self.instruments.hv_off(
                    execute_each_step=lambda: self.execute_each_step(starting_voltages)
                )
                self.instruments.lv_off()
                self.test.terminate()
                self.abortSignal.emit()
            except Exception as err:
                logger.error(f"Failed to stop the SLDO test due to error {err}")
                logger.error(traceback.format_exc())
        else:
            try:
                starting_voltages = [
                    np.abs(getattr(module["hv"], "voltage"))
                    for module in self.instruments._module_dict.values()
                ]
                self.instruments.hv_off(
                    execute_each_step=lambda: self.execute_each_step(starting_voltages)
                )
                self.instruments.lv_off()
                self.test.terminate()
            except Exception as err:
                logger.error(f"Failed to stop the SLDO test due to error {err}")
                logger.error(traceback.format_exc())
