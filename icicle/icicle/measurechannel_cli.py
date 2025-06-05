"""MeasureChannel CLI module. See ``measurechannel --help`` for description, or read
Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the `Instrument.PowerChannel` class.
"""

import click
import logging
import inspect

from .instrument import Instrument
from .caendt8033n import CaenDT8033N  # noqa: F401
from .hmp4040 import HMP4040  # noqa: F401
from .itkdcsinterlock import ITkDCSInterlock  # noqa: F401
from .tti import TTI  # noqa: F401
from .ttitsx import TTITSX  # noqa: F401
from .keysighte3633a import KeysightE3633A  # noqa: F401
from .hp34401a import HP34401A  # noqa: F401
from .keithley2000 import Keithley2000  # noqa: F401
from .keithley6500 import Keithley6500  # noqa: F401
from .keithley2410 import Keithley2410  # noqa: F401
from .relay_board import RelayBoard  # noqa: F401
from .adc_board import AdcBoard  # noqa: F401
from .binder_climate_chamber import Binder  # noqa: F401
from .psi_coldbox import PSIColdbox  # noqa : F401
from .mp1 import MP1  # noqa: F401
from .lauda import Lauda  # noqa: F401
from .cli_utils import with_instrument, print_output, verbosity

# ================== CLI METHODS ===================
# Below are script-like functions designed to be called to replace individual python
# scripts. They generally should:
# 1. instantiate and connect to instrument without resetting it
# 2. perform action
# 3. disconnect without ramping down or terminating instrument


@click.group()
@click.argument(
    "instrument",
    type=click.Choice(Instrument.registered_classes),
    metavar="INSTRUMENT_CLASS",
)
@click.argument("channel", type=int, metavar="CHANNEL")
@click.argument("measure_type", type=str, metavar="MEASURE_TYPE")
@click.option(
    "-v", "--verbose", count=True, help="Verbose output (-v = INFO, -vv = DEBUG)"
)
@click.option(
    "-R",
    "--resource",
    metavar="TARGET",
    type=str,
    default=None,
    help="VISA resource address",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.option(
    "-H",
    "--hw-wrapper",
    type=(click.Choice(Instrument.registered_classes), str, int, str),
    metavar="INSTRUMENT_CLASS RESOURCE CHANNEL WRAPPER_TYPE",
    multiple=True,
    default=tuple(),
    help="Wrap this channel in a hardware channel wrapper (e.g. RelayBoard)",
)
@click.pass_context
def cli(
    ctx, instrument, channel, measure_type, resource, verbose, hw_wrapper, simulate
):
    logging.basicConfig(level=verbosity(verbose))
    cls = Instrument.registered_classes[instrument]
    if resource is None:
        resource = inspect.signature(cls.__init__).parameters["resource"].default
    channel_obj = cls(resource=resource, sim=simulate).channel(
        "MeasureChannel", channel, measure_type=measure_type
    )

    for w_instrument, w_resource, w_channel, w_type in hw_wrapper:
        w_cls = Instrument.registered_classes[w_instrument]
        if w_resource == "":
            w_resource = (
                inspect.signature(w_cls.__init__).parameters["resource"].default
            )
        channel_obj = w_cls(resource=w_resource, sim=simulate).channel(
            "MeasureChannelWrapper",
            w_channel,
            wrapper_type=w_type,
            wrapped_channel=channel_obj,
        )

    ctx.obj = channel_obj


@cli.command(
    "identify",
    help=("Run identify command on device this channel belongs to."),
)
@with_instrument
@print_output
def cli_identify(channel):
    return channel.instrument.identify()


@cli.command(
    "status",
    help=("Read status register."),
)
@with_instrument
@print_output
def cli_status(channel):
    return f"0x{channel.status:X}"


@cli.command(
    "value",
    help=("Perform measurement and return value"),
)
@with_instrument
@print_output
def cli_value(channel):
    return channel.value
