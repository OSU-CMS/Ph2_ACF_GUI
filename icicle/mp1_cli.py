"""Gulmay MP1 CLI module. See ``mp1 --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the mp1 module.
"""

import click
import logging

from .mp1 import MP1
from .cli_utils import with_instrument, print_output, verbosity

# ================== CLI METHODS ===================
# Below are script-like functions designed to be called to replace individual python
# scripts. They generally should:
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
    default="ASRL10::INSTR",
    help="VISA resource address (default: ASRL10::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.pass_context
def cli(ctx, resource, verbose, simulate, cls=MP1):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("on", help="Turn xrays ON")
@with_instrument
@print_output
def cli_on(instrument):
    return "ON" if instrument.on() else "COMMAND FAILED"


@cli.command("off", help="Turn xrays OFF")
@with_instrument
@print_output
def cli_off(instrument):
    return "OFF" if instrument.off() else "COMMAND FAILED"


@cli.command(
    "voltage",
    help="Set VOLTAGE (V) in kV, or query current voltage if VOLTAGE not specified.",
)
@click.argument("voltage", metavar="[VOLTAGE]", type=int, required=False, default=None)
@with_instrument
@print_output
def cli_voltage(instrument, voltage):
    if voltage is None:
        return instrument.query("VOLTAGE")
    else:
        ret = instrument.set("VOLTAGE", voltage)
        return ret if ret == voltage else "COMMAND FAILED"


@cli.command(
    "current",
    help="Set CURRENT (I) in mA, or query current voltage if CURRENT not specified.",
)
@click.argument(
    "current", metavar="[CURRENT]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_current(instrument, current):
    if current is None:
        return instrument.query("CURRENT")
    else:
        ret = instrument.set("CURRENT", current)
        return ret if ret == current else "COMMAND FAILED"


@cli.command("read_timer", help="Query current timer value.")
@with_instrument
@print_output
def cli_read_timer(instrument):
    return instrument.query("TIMER")


@cli.command("read_errors", help="Query current errors.")
@with_instrument
@print_output
def cli_read_errors(instrument):
    return instrument.query("ERRORS")


@cli.command("read_warmup", help="Query warmup routine.")
@with_instrument
@print_output
def cli_read_warmup(instrument):
    return instrument.query("WARMUP")


@cli.command("read_programme_revision", help="Query programme revision.")
@with_instrument
@print_output
def cli_read_programme_revision(instrument):
    return instrument.query("PROGRAMME_REVISION")


@cli.command("reset_timer", help="Reset timer.")
@with_instrument
def cli_reset_timer(instrument):
    return instrument.set("RESET_TIMER")


@cli.command("status", help="Query instrument status.")
@with_instrument
@print_output
def cli_status(instrument):
    return instrument.query("MODE")


@cli.command(
    "focus",
    help=(
        "Set focus (fine [F] or broad [B]), or query current focus if F|B not "
        "specified."
    ),
)
@click.argument(
    "focus",
    metavar="[F|B]",
    type=click.Choice(["F", "B"]),
    required=False,
    default=None,
)
@with_instrument
@print_output
def cli_focus(instrument, focus):
    if focus is None:
        return instrument.query("FOCUS")
    else:
        ret = instrument.set("FOCUS", focus)
        return ret if ret == focus else "COMMAND FAILED"


# ======== LOW-LEVEL COMMANDS - Take care if using these =========


@cli.command(
    "set",
    help="Set configuration field FIELD to value VALUE (see mp1.py for config table).",
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("value", metavar="VALUE", type=str, nargs=-1)
@with_instrument
@print_output
def cli_set(instrument, field, value):
    return instrument.set(field, *value, check_readback_only="same")


@cli.command(
    "query",
    help="Query configuration field FIELD (see mp1.py for config table).",
)
@click.argument("field", metavar="FIELD", type=str)
@with_instrument
@print_output
def cli_query(instrument, field):
    return instrument.query(field)


@cli.command(
    "scpi_write",
    help=(
        "Send SCPI command COMMAND. [LOW-LEVEL COMMAND; use only if you really know "
        "what you're doing...]"
    ),
)
@click.argument("command", metavar="COMMAND", type=str, nargs=-1)
@with_instrument
@print_output
def cli_scpi_write(instrument, command):
    return instrument._instrument.write(" ".join(command))


@cli.command(
    "scpi_query",
    help=(
        "Send SCPI command COMMAND, and immediately read response. [LOW-LEVEL "
        "COMMAND; use only if you really know what you're doing...]"
    ),
)
@click.argument("command", metavar="COMMAND", type=str, nargs=-1)
@with_instrument
@print_output
def cli_scpi_query(instrument, command):
    return instrument._instrument.query(" ".join(command))


@cli.command(
    "scpi_read",
    help=(
        "Attempt to read on SCPI serial line. [LOW-LEVEL COMMAND; use only if you "
        "really know what you're doing...]"
    ),
)
@with_instrument
@print_output
def cli_scpi_read(instrument):
    return instrument._instrument.read()
