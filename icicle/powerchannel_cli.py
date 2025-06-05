"""PowerChannel CLI module. See ``powerchannel --help`` for description, or read Click
decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the `Instrument.PowerChannel` class.
"""

import click
import logging
import inspect
from math import ceil

from .instrument import Instrument
from .hmp4040 import HMP4040  # noqa: F401
from .caendt8033n import CaenDT8033N  # noqa: F401
from .tti import TTI  # noqa: F401
from .ttitsx import TTITSX  # noqa: F401
from .keysighte3633a import KeysightE3633A  # noqa: F401
from .keithley2410 import Keithley2410  # noqa: F401
from .mp1 import MP1  # noqa: F401
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
@click.pass_context
def cli(ctx, instrument, channel, resource, verbose, simulate):
    logging.basicConfig(level=verbosity(verbose))
    cls = Instrument.registered_classes[instrument]
    if resource is None:
        resource = inspect.signature(cls.__init__).parameters["resource"].default
    ctx.obj = cls(resource=resource, sim=simulate).channel("PowerChannel", channel)


@cli.command(
    "identify",
    help=("Run identify command on device this channel belongs to."),
)
@with_instrument
@print_output
def cli_identify(channel):
    return channel.instrument.identify()


@cli.command(
    "state",
    help=(
        "Set state to STATE (ON/OFF) for channel, or query current state if STATE not "
        "specified."
    ),
)
@click.argument(
    "state",
    metavar="[STATE]",
    type=click.Choice(("0", "1", "OFF", "off", "ON", "on")),
    required=False,
    default=None,
)
@with_instrument
@print_output
def cli_state(channel, state):
    if state is not None:
        channel.state = state
    return channel.state


@cli.command(
    "status",
    help=("Read status register."),
)
@with_instrument
@print_output
def cli_status(channel):
    return f"0x{channel.status:X}"


@cli.command(
    "set",
    help=("Set VOLTAGE (V) and CURRENT (I) for channel"),
)
@click.argument("voltage", metavar="[VOLTAGE]", type=float, required=True)
@click.argument("current", metavar="[CURRENT]", type=float, required=True)
@with_instrument
@print_output
def cli_set(channel, voltage, current):
    channel.voltage = voltage
    channel.current = current
    return {"Set voltage:": channel.voltage, "Set current:": channel.current}


@cli.command(
    "voltage",
    help=(
        "Set VOLTAGE (V) for channel, or query current voltage if VOLTAGE not "
        "specified."
    ),
)
@click.argument(
    "voltage", metavar="[VOLTAGE]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_voltage(channel, voltage):
    if voltage is not None:
        channel.voltage = voltage
    return channel.voltage


@cli.command(
    "current",
    help=("Set CURRENT (A) for channel, or query current if CURRENT not " "specified."),
)
@click.argument(
    "current", metavar="[CURRENT]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_current(channel, current):
    if current is not None:
        channel.current = current
    return channel.current


@cli.command(
    "voltage_limit",
    help=(
        "Set VOLTAGE_LIMIT (V) for channel, or query current voltage limit if "
        "VOLTAGE_LIMIT not specified."
    ),
)
@click.argument(
    "voltage_limit", metavar="[VOLTAGE_LIMIT]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_voltage_limit(channel, voltage_limit):
    if voltage_limit is not None:
        channel.voltage_limit = voltage_limit
    return channel.voltage_limit


@cli.command(
    "current_limit",
    help=(
        "Set CURRENT_LIMIT (A) for channel, or query current limit if CURRENT_LIMIT "
        "not specified."
    ),
)
@click.argument(
    "current_limit", metavar="[CURRENT_LIMIT]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_current_limit(channel, current_limit):
    if current_limit is not None:
        channel.current_limit = current_limit
    return channel.current_limit


@cli.command(
    "voltage_trip",
    help=("Query whether an unhandled voltage trip has occurred."),
)
@click.option(
    "--reset", is_flag=True, default=False, help="Reset voltage trip condition."
)
@with_instrument
@print_output
def cli_voltage_trip(channel, reset):
    if reset:
        channel.reset_voltage_trip()
    return channel.voltage_trip


@cli.command(
    "current_trip",
    help=("Query whether an unhandled current trip has occurred."),
)
@click.option(
    "--reset", is_flag=True, default=False, help="Reset current trip condition."
)
@with_instrument
@print_output
def cli_current_trip(channel, reset):
    if reset:
        channel.reset_current_trip()
    return channel.current_trip


@cli.command(
    "measure_voltage",
    help=("Measure VOLTAGE (V) for channel"),
)
@with_instrument
@print_output
def cli_measure_voltage(channel):
    return channel.measure_voltage.value


@cli.command(
    "measure_current",
    help=("Measure CURRENT (A) for channel."),
)
@with_instrument
@print_output
def cli_measure_current(channel):
    return channel.measure_current.value


@cli.command("sweep_voltage", help="Sweep voltage to target voltage TARGET (V).")
@click.argument("target", metavar="TARGET", type=float)
@click.option(
    "-s",
    "--step",
    metavar="STEP",
    type=float,
    default=None,
    help="step size (V)",
)
@click.option(
    "-d",
    "--delay",
    metavar="DELAY",
    type=float,
    default=1.0,
    help="settling time between steps (seconds; defaults to 1)",
)
@click.option(
    "-n",
    "--n-steps",
    metavar="NSTEPS",
    type=int,
    default=None,
    help="number of steps with delay DELAY to perform",
)
@click.option(
    "-r",
    "--rate",
    metavar="VOLTS_PER_SECOND",
    type=float,
    default=None,
    help="rate to sweep (in V/s)",
)
@click.option("-m", "--measure", is_flag=True, help="Measure (V, I) at each step.")
@click.option("-l", "--no-log", is_flag=True, help="Turn off logging at each step.")
@with_instrument
@print_output
def cli_sweep_voltage(channel, target, step, delay, n_steps, rate, measure, no_log):
    if rate is not None:
        if delay is None or delay == 0:
            raise RuntimeError("Need to specify both rate and step length (delay)")
        if rate == 0:
            raise RuntimeError("Rate needs to be non-zero")
        cval = channel.voltage
        sweep_range = target - cval
        n_steps = int(ceil(abs((sweep_range / rate) / delay)))

    return channel.sweep(
        target,
        delay,
        step,
        n_steps,
        measure=measure,
        log_function=None if no_log else print,
        set_property="voltage",
    )


@cli.command("sweep_current", help="Sweep current to target current TARGET (A).")
@click.argument("target", metavar="TARGET", type=float)
@click.option(
    "-s",
    "--step",
    metavar="STEP",
    type=float,
    default=None,
    help="step size (A)",
)
@click.option(
    "-d",
    "--delay",
    metavar="DELAY",
    type=float,
    default=1.0,
    help="settling time between steps (seconds; defaults to 1)",
)
@click.option(
    "-n",
    "--n-steps",
    metavar="NSTEPS",
    type=int,
    default=None,
    help="number of steps with delay DELAY to perform",
)
@click.option(
    "-r",
    "--rate",
    metavar="VOLTS_PER_SECOND",
    type=float,
    default=None,
    help="rate to sweep (in V/s)",
)
@click.option("-m", "--measure", is_flag=True, help="Measure (V, I) at each step.")
@click.option("-l", "--no-log", is_flag=True, help="Turn off logging at each step.")
@with_instrument
@print_output
def cli_sweep_current(channel, target, step, delay, n_steps, rate, measure, no_log):
    if rate is not None:
        if delay is None or delay == 0:
            raise RuntimeError("Need to specify both rate and step length (delay)")
        if rate == 0:
            raise RuntimeError("Rate needs to be non-zero")
        cval = channel.current
        sweep_range = target - cval
        n_steps = int(ceil(abs((sweep_range / rate) / delay)))

    return channel.sweep(
        target,
        delay,
        step,
        n_steps,
        measure=measure,
        log_function=None if no_log else print,
        set_property="current",
    )
