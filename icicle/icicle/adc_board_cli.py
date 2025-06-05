"""
Relay Board CLI module. See ``relay_board --help`` for description,
or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI
self-documentation should be sufficient.

See also the relay_board module.
"""

import click
import logging

from .adc_board import AdcBoard
from .cli_utils import with_instrument, verbosity

# ================== CLI METHODS ===================
# Below are script-like functions designed to be called to replace
# individual python scripts. They generally should:
# 1. instantiate and connect to instrument without resetting it
# 2. perform action
# 3. disconnect without ramping down or terminating instrument


@click.group()
@click.option(
    "-v", "--verbose", count=True, help="Verbose output (-v = INFO, -vv = DEBUG)"
)
@click.option(
    "-R",
    "--resource",
    metavar="TARGET",
    type=str,
    default="ASRL21::INSTR",
    help="VISA resource address (default: ASRL/dev/ttyUSB3::INSTR)",
)
@click.option(
    "-m",
    "--pin_map",
    metavar="MODULE",
    type=str,
    default=None,
    help="""Selects a specific map for the pins on a module.
    Generic if `None` (Currently QUAD or DOUBLE as options)""",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.pass_context
def cli(ctx, resource, verbose, pin_map, simulate, cls=AdcBoard):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, pin_map=pin_map, sim=simulate)


# end def cli


@cli.command(
    "on",
    help="Turn on AdcBoard. Does not do anything,"
    + "as AdcBoard is always on when plugged in.",
)
@with_instrument
def cli_on(instrument):
    return not instrument.on()


@cli.command(
    "off",
    help="Turn off AdcBoard. Does not do anything,"
    + "as AdcBoard cannot be turned off without pulling the plug.",
)
@with_instrument
def cli_off(instrument):
    return not instrument.off()


@cli.command("monitor", help="query and print voltage on pins")
@click.option(
    "-r",
    "--repeats",
    metavar="INT",
    type=int,
    default=0,
    help="Number of repeats of the monitoring. infinite if 0",
)
@with_instrument
def cli_monitor(instrument, repeats):
    instrument.monitor(repeats=repeats)


@cli.command("measure", help="query and print voltage on pins once")
@with_instrument
def cli_measure(instrument):
    instrument.monitor(repeats=1)


@cli.command("identify", help="get production and serial number of adc_board")
@with_instrument
def cli_identify(instrument, repeats=0):
    instrument.identify()


@cli.command("details", help="get detailed information about board")
@with_instrument
def cli_status(instrument, repeats=0):
    status_list = instrument.query_status()
    properties = [
        "Production ID",
        "Serial ID",
        "Hardware version",
        "Hardware assembly version",
        "Software version",
    ]
    for idx, prop in enumerate(properties):
        print(f"{prop} : {status_list[idx]}")
