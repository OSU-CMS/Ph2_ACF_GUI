"""
Helpers that work around InstrumentCluster behaviour in upstream icicle
(pinned at MR !136, commit 40c3faf) without modifying icicle itself.

- InstrumentCluster.status() raises for every configuration (it reads a
  module["rb"] key that is never created, and its "hb" entry cannot be built).
- PoweringGroup.ramp_hv() accepts **kwargs but does not pass them on to the
  sweep, so execute_each_step and break_loop callbacks are silently dropped.
- InstrumentCluster.lv_sweep() returns a single bool instead of the sweep data.
- InstrumentCluster.cb_off() requires LV to be off, and prompts on the terminal
  with input() if it is not.

Once these are fixed upstream the matching helpers can be removed.
"""

from icicle.icicle.instrument_cluster import DummyInstrument, InstrumentNotInstantiated
from icicle.icicle.psi_hvbox import PSIHVBox

from Gui.python.logging_config import get_logger

logger = get_logger(__name__)


def _is_on(state):
    """Interpret a channel state that may be a bool, int or string such as '1'."""
    if isinstance(state, InstrumentNotInstantiated):
        return False
    return str(state).strip().lower() in ("1", "true", "on")


def power_status(instruments):
    """Replacement for InstrumentCluster.status().

    :return: {module number: {"hv": bool, "lv": bool}}
    """
    status = {}
    for number, module in instruments._module_dict.items():
        status[number] = {
            "hv": _is_on(module["hv"].state),
            "lv": _is_on(module["lv"].state),
        }
    return status


def ramp_group(
    group,
    voltage,
    delay=1,
    step_size=5,
    measure=True,
    execute_each_step=None,
    break_loop=None,
):
    """Ramp the HV source of one powering group, passing the callbacks through.

    Mirrors PoweringGroup.ramp_hv(), except that the source voltage range is
    raised when the target exceeds it (as the GUI's IV curves require) instead
    of raising an error.

    :return: (final voltage, sweep data) as returned by PowerChannel.sweep()
    """
    source = group.source_channel
    if "VOLTAGE_RANGE" in source.instrument.SETTINGS:
        voltage_range = float(source.instrument.query("VOLTAGE_RANGE"))
        if abs(voltage) > voltage_range:
            logger.info(
                f"Raising HV source range from {voltage_range} V for target {voltage} V"
            )
            group.source_set_hvrange(voltage)

    if isinstance(group.hvbox, PSIHVBox):
        measure_fields = [f"Current {name} (A)" for name in group.modulenames]

        def measure_function(**_):
            return [module["hb"].measure_current.value for module in group.modules]

    else:
        measure_fields = None
        measure_function = None

    return source.sweep(
        voltage,
        delay,
        step_size,
        measure=measure,
        log_function=logger.info,
        measure_fields=measure_fields,
        measure_function=measure_function,
        execute_each_step=execute_each_step,
        break_loop=break_loop,
    )


def ramp_powering_groups(
    instruments,
    voltage,
    delay=1,
    step_size=5,
    measure=True,
    execute_each_step=None,
    break_loop=None,
):
    """Ramp every powering group's HV source to `voltage` with callbacks.

    Replacement for PoweringGroup.ramp_hv() and the deprecated
    InstrumentCluster.hv_set(). HV must already be on.
    """
    return [
        ramp_group(
            group, voltage, delay, step_size, measure, execute_each_step, break_loop
        )
        for group in instruments.powering_groups.values()
    ]


def hv_on_with_progress(
    instruments,
    voltage,
    delay=1,
    step_size=5,
    measure=True,
    execute_each_step=None,
    break_loop=None,
):
    """Turn HV on (if needed) and ramp to `voltage` with callbacks.

    Replacement for InstrumentCluster.hv_on(..., execute_each_step=...).
    """
    groups = list(instruments.powering_groups.values())
    if any(not _is_on(group.source_channel.state) for group in groups):
        # Turns the sources on and enables the HVBox channels at 0 V
        instruments.hv_on(voltage=0, delay=delay, step_size=step_size, measure=False)
    else:
        for group in groups:
            group.enable_all()
    return ramp_powering_groups(
        instruments, voltage, delay, step_size, measure, execute_each_step, break_loop
    )


def hv_ramp_down(
    instruments,
    delay=None,
    step_size=None,
    measure=True,
    execute_each_step=None,
    **kwargs,
):
    r"""Ramp HV to 0 V with a progress callback, then turn HV off.

    Replacement for InstrumentCluster.hv_off(execute_each_step=...).

    :param delay: delay between sweep steps. Defaults to the HV source's
        default_delay, as hv_off() did on the previous icicle branch.
    :param step_size: step size between sweep steps. Defaults to the HV
        source's default_step_size.
    :param \**kwargs: passed to InstrumentCluster.hv_off() (e.g. no_lock).
    """
    for group in instruments.powering_groups.values():
        source = group.source_channel
        if _is_on(source.state):
            ramp_group(
                group,
                0,
                delay if delay is not None else getattr(source, "default_delay", 1),
                (
                    step_size
                    if step_size is not None
                    else getattr(source, "default_step_size", 5)
                ),
                measure,
                execute_each_step=execute_each_step,
            )
    # Sources are now at 0 V, so this only disables the channels and sources
    return instruments.hv_off(measure=measure, **kwargs)


def lv_sweep_all(instruments, target, delay, step_size, measure=False, **kwargs):
    r"""Replacement for InstrumentCluster.lv_sweep() that returns the sweep data.

    :param \**kwargs: passed to PowerChannel.sweep() (e.g. set_property,
        measure_function, measure_args, execute_each_step).
    :return: list with one (final value, sweep data) entry per module
    """
    return [
        instruments.lv_sweep_module(
            module, target, delay, step_size, measure=measure, **kwargs
        )
        for module in instruments._module_dict.values()
    ]


def coldbox_off(instruments):
    """Turn off all coldbox channels while LV may still be on.

    Replacement for InstrumentCluster.cb_off(checkstatus=False). Dry air (FLUSH)
    is left as is.
    """
    ret = []
    for module in instruments._module_dict.values():
        coldbox = module["cb"]
        if isinstance(coldbox, DummyInstrument):
            continue
        if _is_on(coldbox.state):
            coldbox.state = False
        ret.append(not _is_on(coldbox.state))
    return all(ret)
