from icicle import (  # noqa: F401
    adc_board,
    binder_climate_chamber,
    dummy,
    hmp4040,
    hp34401a,
    keithley2000,
    keithley6500,
    keithley2410,
    keysighte3633a,
    lauda,
    mp1,
    relay_board,
    tti,
    ttitsx,
    ximc_instrument,
    caenDT8033N,
)
from icicle.instrument import Instrument
import inspect

contents = """\
{inst} {ch} identify               | powerchannel -S {inst} {ch} identify
{inst} {ch} status                 | powerchannel -S {inst} {ch} status
{inst} {ch} voltage                | powerchannel -S {inst} {ch} voltage
{inst} {ch} voltage set            | powerchannel -S {inst} {ch} voltage 5
{inst} {ch} voltage_limit          | powerchannel -S {inst} {ch} voltage_limit
{inst} {ch} voltage_limit set      | powerchannel -S {inst} {ch} voltage_limit 5
{inst} {ch} voltage_trip           | powerchannel -S {inst} {ch} voltage_trip
{inst} {ch} voltage_trip reset     | powerchannel -S {inst} {ch} voltage_trip --reset
{inst} {ch} current                | powerchannel -S {inst} {ch} current
{inst} {ch} current set            | powerchannel -S {inst} {ch} current 3
{inst} {ch} current_limit          | powerchannel -S {inst} {ch} current_limit
{inst} {ch} current_limit set      | powerchannel -S {inst} {ch} current_limit 3
{inst} {ch} current_trip           | powerchannel -S {inst} {ch} current_trip
{inst} {ch} current_trip reset     | powerchannel -S {inst} {ch} current_trip --reset
{inst} {ch} sweep_voltage          | \
powerchannel -S {inst} {ch} sweep_voltage -s 2 -d 0 10
{inst} {ch} sweep_voltage n_steps  | \
powerchannel -S {inst} {ch} sweep_voltage -n 3 -d 0 10
{inst} {ch} sweep_voltage measure  | \
powerchannel -S {inst} {ch} sweep_voltage -s 2 -d 0 10 -m
{inst} {ch} sweep_voltage no log   | \
powerchannel -S {inst} {ch} sweep_voltage -s 2 -d 0 10 -l
{inst} {ch} sweep_voltage m+nl     | \
powerchannel -S {inst} {ch} sweep_voltage -s 2 -d 0 10 -m -l
{inst} {ch} sweep_current          | \
powerchannel -S {inst} {ch} sweep_current -s 0.2 -d 0 2
{inst} {ch} sweep_current n_steps  | \
powerchannel -S {inst} {ch} sweep_current -n 3 -d 0 2
{inst} {ch} sweep_current measure  | \
powerchannel -S {inst} {ch} sweep_current -s 0.2 -d 0 2 -m
{inst} {ch} sweep_current no log   | \
powerchannel -S {inst} {ch} sweep_current -s 0.2 -d 0 2 -l
{inst} {ch} sweep_current m+nl     | \
powerchannel -S {inst} {ch} sweep_current -s 0.2 -d 0 2 -m -l
{inst} {ch} measure_voltage        | powerchannel -S {inst} {ch} measure_voltage
{inst} {ch} measure_current        | powerchannel -S {inst} {ch} measure_current\
"""

for inst, cls in Instrument.registered_classes.items():
    if cls.PowerChannel is Instrument.PowerChannel:
        continue
    if "sim" not in inspect.signature(cls.__init__).parameters.keys():
        continue
    if hasattr(cls, "NO_SIMULATION") and cls.NO_SIMULATION:
        continue
    print(f"# == {inst} ==")
    for ch in range(1, cls.OUTPUTS + 1):
        print(contents.format(inst=inst, ch=ch))
