"""Helpers that work with both the old (osu_v2.1.0_hvbox_dev, 2.1.0-44-g0227d6b) and
the new (2.5.0+) icicle versions while the stations migrate between them.

Once every station runs the new icicle, these can be simplified to the new API.
"""

import inspect
import logging

from icicle.icicle.instrument import acquire_lock
from icicle.icicle.instrument_cluster import (
    DummyInstrument,
    InstrumentCluster,
    InstrumentNotInstantiated,
)
from icicle.icicle.psi_hvbox import PSIHVBox

logger = logging.getLogger(__name__)

_ON_VALUES = (True, 1, "1", "ON", "TRUE", "YES", "Y")

# Keyword arguments the GUI passes through HV ramps to PowerChannel.sweep
_SWEEP_CALLBACKS = ("execute_each_step", "break_loop")


@acquire_lock()
def _ramp_hv_forwarding_callbacks(self, voltage, delay=1, step_size=5, measure=True, **kwargs):
    """New icicle's PoweringGroup.ramp_hv, but passing execute_each_step and
    break_loop on to the sweep (upstream accepts **kwargs and drops them, so ramp
    progress, IV measurements and aborting a ramp never run), and raising the
    source's voltage range when the target is beyond it.

    New icicle sets the range once, from the HV default_voltage, when the source
    is switched on, then refuses to ramp past it. A Keithley 2470 picks its 200 V
    range for a -120 V default, so the -350 V IV curve would fail. Old icicle raised
    the range to the ramp target before each ramp; this does the same. The range is
    only ever raised here, never lowered, so ramping down cannot clip the output.
    """
    if "VOLTAGE_RANGE" in self.source_channel.instrument.SETTINGS:
        voltage_range = self.source_channel.instrument.query("VOLTAGE_RANGE")
        if abs(voltage) > voltage_range:
            logger.info(
                f"Raising HV source voltage range from {voltage_range} V to cover "
                f"{voltage} V for powering group with modules {self.modulenames}."
            )
            # Sets the range from |voltage| and raises if the instrument cannot reach it
            self.source_set_hvrange(voltage)
            voltage_range = self.source_channel.instrument.query("VOLTAGE_RANGE")
        if abs(voltage) > voltage_range:
            raise ValueError(
                f"Target voltage {voltage} V exceeds source voltage range "
                f"{voltage_range} V for powering group with modules "
                f"{self.modulenames}."
            )
    else:
        logger.debug("Source channel does not support VOLTAGE_RANGE.")

    if abs(voltage - self.source_channel.measure_voltage.value) <= 5:
        self.voltage(voltage, no_lock=True)
        return

    if isinstance(self.hvbox, PSIHVBox):
        measure_fields = [f"Current {name} (A)" for name in self.modulenames]

        def measure_function(**_):
            return [module["hb"].measure_current.value for module in self.modules]

    else:
        measure_fields = None
        measure_function = None

    self.source_channel.sweep(
        voltage,
        delay,
        step_size,
        measure=measure,
        log_function=logger.info,
        measure_fields=measure_fields,
        measure_function=measure_function,
        **{k: kwargs[k] for k in _SWEEP_CALLBACKS if k in kwargs},
    )


def apply_icicle_patches():
    """Work around icicle issues; call before constructing an InstrumentCluster.

    - New icicle's InstrumentCluster.package_map lists the ADC board as "AdcBoard",
      but the class in adc_board.py is ADCBoard, so neither name loads. Old icicle
      (OSU branch) maps "ADCBoard".
    - New icicle's PoweringGroup.ramp_hv (recognisable by its `measure` parameter)
      drops execute_each_step/break_loop and won't ramp past the source's voltage
      range; old icicle's forwards **kwargs to the sweep and raises the range first.
    """
    InstrumentCluster.package_map.setdefault("ADCBoard", "adc_board")

    group_cls = InstrumentCluster.PoweringGroup
    if (
        group_cls.ramp_hv is not _ramp_hv_forwarding_callbacks
        and "measure" in inspect.signature(group_cls.ramp_hv).parameters
    ):
        group_cls.ramp_hv = _ramp_hv_forwarding_callbacks


def ramp_hv_all_groups(instruments, voltage, delay, step_size, **kwargs):
    """Ramp every powering group's HV source to `voltage`.

    Replaces InstrumentCluster.hv_set, which new icicle deprecates and which always
    raises RuntimeError there. Expects the HV outputs to already be enabled.
    """
    for group in instruments.powering_groups.values():
        group.ramp_hv(voltage, delay=delay, step_size=step_size, **kwargs)


def output_is_on(state) -> bool:
    """Interpret an instrument channel `state` reading as on/off.

    Supplies report bool, int or string depending on the instrument; a role with no
    instrument configured is a DummyInstrument whose attributes are
    InstrumentNotInstantiated objects, which count as off.
    """
    if isinstance(state, InstrumentNotInstantiated):
        return False
    if isinstance(state, str):
        state = state.strip().upper()
    return state in _ON_VALUES


def any_output_on(instruments, role: str) -> bool:
    """Whether the `role` ("lv" or "hv") output of any module in the cluster is on.

    Reads each module's channel state directly instead of InstrumentCluster.status(),
    which in the new icicle reads a relay board entry ("rb") that the cluster never
    creates and so always raises KeyError.
    """
    return any(
        output_is_on(getattr(module[role], "state", None))
        for module in instruments.get_modules().values()
        if role in module
    )


def coldbox_off_keep_lv(instruments) -> bool:
    """Turn off every module's TEC channel while leaving LV (and the dry air flush) on,
    so the modules can warm up before LV is switched off.

    Old icicle did this with cb_off(checkstatus=False). New icicle's cb_off() has no
    checkstatus option: with LV on it prompts on the terminal via input(), which hangs
    the GUI, and it also switches off the dry air flush. This does what cb_off_module
    does after its LV check.
    """
    turned_off = []
    for module in instruments.get_modules().values():
        cb = module.get("cb")
        if cb is None or isinstance(cb, DummyInstrument):
            continue
        cb.state = False
        # Old icicle reports POWER as int, new as bool
        turned_off.append(not output_is_on(cb.state))
    return all(turned_off)


def measured_temperature(cb_channel) -> float:
    """Measured temperature of a coldbox TemperatureChannel, in degC.

    Old icicle: `measure_temperature` returns the value. New icicle: it returns a
    MeasureChannel whose `.value` is the reading.
    """
    reading = cb_channel.measure_temperature
    return float(getattr(reading, "value", reading))


def hvbox_channel_id(hb_channel):
    """HV distribution box channel number of a module's "hb" channel wrapper.

    Renamed from `_hvbox_channel` (old icicle) to `hvbox_channel_id` (new icicle).
    """
    if hasattr(hb_channel, "hvbox_channel_id"):
        return hb_channel.hvbox_channel_id
    return hb_channel._hvbox_channel
