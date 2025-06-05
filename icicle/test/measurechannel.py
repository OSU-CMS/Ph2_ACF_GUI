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
    ximc_instrument,
    caenDT8033N,
)
from icicle.instrument import Instrument
import inspect

for inst, cls in Instrument.registered_classes.items():
    if not hasattr(cls, "MEASURE_TYPES"):
        continue
    if "sim" not in inspect.signature(cls.__init__).parameters.keys():
        continue
    if hasattr(cls, "NO_SIMULATION") and cls.NO_SIMULATION:
        continue
    print(f"# == {inst} ==")
    for typ in cls.MEASURE_TYPES:
        print(
            f"{inst} {typ} identify ".ljust(35)
            + f" | measurechannel -S {inst} 1 {typ} identify"
        )
        print(
            f"{inst} {typ} value ".ljust(35)
            + f" | measurechannel -S {inst} 1 {typ} value"
        )
        print(
            f"{inst} {typ} status ".ljust(35)
            + f" | measurechannel -S {inst} 1 {typ} status"
        )
    print()
