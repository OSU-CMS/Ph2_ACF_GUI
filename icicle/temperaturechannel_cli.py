"""TemperatureChannel CLI module. See ``temperaturechannel --help`` for description, or
read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the `Instrument.TemperatureChannel` class.
"""

import click
import logging
import inspect

from .instrument import Instrument

try:
    from .hubercc508 import HuberCC508  # noqa: F401
except ImportError:
    pass
from .pidcontroller import PIDController  # noqa: F401
from .psi_coldbox import PSIColdbox  # noqa: F401
from .cli_utils import with_instrument, print_output, verbosity

#  ================== CLI METHODS ===================
#  Below are script-like functions designed to be called to replace individual python
#  scripts. They generally should:
#  1. instantiate and connect to instrument without resetting it
#  2. perform action
#  3. disconnect without ramping down or terminating instrument


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
    ctx.obj = cls(resource=resource, sim=simulate).channel(
        "TemperatureChannel", channel
    )


"""
@cli.command(
    "identify",
    help=("Run identify command on device this channel belongs to."),
)
@with_instrument
@print_output
def cli_identify(channel):
    return channel.instrument.identify()
"""


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


"""
@cli.command(
    "status",
    help=("Read status register."),
)
@with_instrument
@print_output
def cli_status(channel):
    return f"0x{channel.status:X}"
"""


@cli.command(
    "temperature",
    help=(
        "Set TEMPERATURE (degC) for channel, or query current temperature if "
        "TEMPERATURE not specified."
    ),
)
@click.argument(
    "temperature", metavar="[TEMPERATURE]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_temperature(channel, temperature):
    if temperature is not None:
        channel.temperature = temperature
    return channel.temperature


@cli.command(
    "speed",
    help=(
        "Set SPEED (RPM) for channel, or query current temperature if SPEED not "
        "specified."
    ),
)
@click.argument("speed", metavar="[SPEED]", type=float, required=False, default=None)
@with_instrument
@print_output
def cli_speed(channel, speed):
    if speed is not None:
        channel.speed = speed
    return channel.speed


@cli.command(
    "measure_temperature",
    help=("Measure TEMPERATURE (degC) for channel."),
)
@with_instrument
@print_output
def cli_measure_temperature(channel):
    return channel.measure_temperature.value


@cli.command(
    "measure_speed",
    help=("Measure SPEED (RPM) for channel."),
)
@with_instrument
@print_output
def cli_measure_speed(channel):
    return channel.measure_speed.value
