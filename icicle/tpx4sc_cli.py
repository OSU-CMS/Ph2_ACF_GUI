"""TPX4SC CLI module. See ``tpx4sc --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the tpx4sc module.
"""

import click
import logging
from time import sleep

from .tpx4sc import TPX4SC
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
    default="ASRL30::INSTR",
    help="VISA resource address (default: ASRL30::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.pass_context
def cli(ctx, resource, verbose, simulate, cls=TPX4SC):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)


@cli.command("identify", help="Identify Timepix4 chip")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("status", help="Query Timepix4 status")
@with_instrument
@print_output
def cli_status(instrument):
    return instrument.status()


@cli.command("measure", help="Get number of transferred data packets in this run.")
@with_instrument
@print_output
def cli_measure(instrument):
    vals = instrument.measure()
    if len(vals) == 2:
        bot, top = instrument.measure()
        return f"Bottom half bytes: {bot}  |  Top half bytes: {top}"
    elif len(vals) == 6:
        return f"L1A: {vals[0]:d} | " + " | ".join(
            f"Pl{i} {vals[i]:d}" for i in range(1, 6)
        )
    else:
        return vals


@cli.command("configure", help="Configure Timepix4")
@with_instrument
@print_output
def cli_configure(instrument):
    ret = instrument.set("CONFIGURE")
    if "W000" in instrument.identify():
        instrument.set("THRESHOLD", 2000)
        sleep(0.2)
        instrument.set("THRESHOLD", 2000)
        sleep(0.2)
        instrument.set("THRESHOLD", 2000)
    return ret


@cli.command("start_run", help="Start data-taking run")
@click.argument("run_number", metavar="[RUN_NUMBER]", type=int, required=True)
@with_instrument
@print_output
def cli_start_run(instrument, run_number):
    return instrument.set("START_RUN", run_number)


@cli.command("stop_run", help="Stop data-taking run")
@with_instrument
@print_output
def cli_stop_run(instrument):
    return instrument.set("STOP_RUN")


@cli.command("start_monitoring", help="Start monitoring")
@with_instrument
@print_output
def cli_start_monitoring(instrument):
    return instrument.set("START_MONITORING")


@cli.command("stop_monitoring", help="Stop monitoring")
@with_instrument
@print_output
def cli_stop_monitoring(instrument):
    return instrument.set("STOP_MONITORING")


@cli.command(
    "threshold",
    help=(
        "Set THRESHOLD (electrons) for Timepix4, or query current threshold "
        "if THRESHOLD not specified."
    ),
)
@click.argument(
    "threshold", metavar="[THRESHOLD]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_threshold(instrument, threshold):
    if threshold is None:
        return instrument.query("THRESHOLD")
    else:
        return instrument.set("THRESHOLD", threshold)


@cli.command("user_data", help="Inject USER_DATA into data stream")
@click.argument("user_data", metavar="[USER_DATA]", type=str, required=True)
@with_instrument
@print_output
def cli_user_data(instrument, user_data):
    return instrument.set("USER_DATA", user_data)


@cli.command("restart_readout", help="Restart remote Timepix4 readout")
@with_instrument
@print_output
def cli_restart_readout(instrument):
    return instrument.set("RESTART_READOUT")


@cli.command("quit", help="Close remote tpx4sc instance")
@with_instrument
@print_output
def cli_quit(instrument):
    if (
        input("Are you sure you want to close a remote instance (Y/n): ")
        .lower()
        .strip()[0]
        == "y"
    ):
        return instrument.set("QUIT")
    else:
        return "No action taken."


# ======== LOW-LEVEL COMMANDS - Take care if using these =========


@cli.command(
    "set",
    help=(
        "Set configuration field FIELD to value VALUE (see keithley2410.py for "
        "config table)."
    ),
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("value", metavar="VALUE", type=str)
@with_instrument
@print_output
def cli_set(instrument, field, value):
    return instrument.set(field, value)


@cli.command(
    "query",
    help="Query configuration field FIELD (see keithley2410.py for config table).",
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
