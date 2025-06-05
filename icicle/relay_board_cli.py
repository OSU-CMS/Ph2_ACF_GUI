"""Relay Board CLI module. See ``relay_board --help`` for description, or read Click
decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the relay_board module.
"""

import click
import logging
from itertools import chain

from .relay_board import RelayBoard
from .cli_utils import with_instrument, print_output, verbosity, InContextObjectField

# ================== CLI METHODS ===================
# Below are script-like functions designed to be called to replace individual
# python scripts. They generally should:
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
    default="ASRL20::INSTR",
    help="VISA resource address (default: ASRL20::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.option(
    "-M",
    "--pin_map",
    type=click.Choice(RelayBoard.PIN_MAP.keys(), case_sensitive=False),
    default="QUAD",
    help="Selects a specific map for the pins on a module",
)
@click.pass_context
def cli(ctx, resource, verbose, pin_map, simulate, cls=RelayBoard):
    # Upgrade to child (shell) class:
    cls = cls.registered_classes[f"RelayBoard{pin_map}"]
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)


@cli.command("off", help="Disconnect all pins.")
@with_instrument
def cli_off(instrument):
    return not instrument.off()


@cli.command("set_pin", help="Set connected pin PIN on relay board.")
@click.argument(
    "pin",
    type=InContextObjectField(
        set(chain(*(m.keys() for m in RelayBoard.PIN_MAP.values()))), "_PIN_MAP"
    ),
)
@with_instrument
@print_output
def cli_set_pin(instrument, pin):
    return instrument.set_pin(pin)


@cli.command("query_pin", help="Query connected pin on relay board.")
@with_instrument
@print_output
def cli_query_pin(instrument):
    return instrument.query_pin()
