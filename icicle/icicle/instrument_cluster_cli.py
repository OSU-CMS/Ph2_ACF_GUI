"""InstrumentCluster CLI module. See ``instrument_cluster --help`` for description, or
read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the instrument_cluster module.
"""

import click
import logging

from .instrument_cluster import InstrumentCluster
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
    "-L",
    "--lv",
    metavar="LV_CLASS",
    type=str,
    default=None,
    help="LV Power Supply class (e.g. TTI)",
)
@click.option(
    "-H",
    "--hv",
    metavar="HV_CLASS",
    type=str,
    default=None,
    help="HV Power Supply class (e.g. Keithley2410)",
)
@click.option(
    "-A",
    "--relay-board",
    metavar="RELAY_BOARD_CLASS",
    type=str,
    default=None,
    help="Relay Board Control class (e.g. RelayBoard)",
)
@click.option(
    "-M",
    "--multimeter",
    metavar="MULTIMETER_CLASS",
    type=str,
    default=None,
    help="Relay Board Control class (e.g. Keithley2000)",
)
@click.option(
    "-LR",
    "--lv-resource",
    metavar="TARGET",
    type=str,
    default="ASRL/dev/ttyACM0::INSTR",
    help='"lv" VISA resource address (default: ASRL/dev/ttyACM0::INSTR)',
)
@click.option(
    "-HR",
    "--hv-resource",
    metavar="TARGET",
    type=str,
    default="ASRL/dev/ttyUSB1::INSTR",
    help='"hv" VISA resource address (default: ASRL/dev/ttyUSB1::INSTR)',
)
@click.option(
    "-AR",
    "--relay-board-resource",
    metavar="TARGET",
    type=str,
    default="ASRL/dev/ttyUSB3::INSTR",
    help='"relay_board" VISA resource address (default: ASRL/dev/ttyUSB3::INSTR)',
)
@click.option(
    "-MR",
    "--multimeter-resource",
    metavar="TARGET",
    type=str,
    default="ASRL/dev/ttyUSB2::INSTR",
    help='"multimeter" VISA resource address (default: ASRL/dev/ttyUSB2::INSTR)',
)
@click.option(
    "-C",
    "--climate_chamber_resouce",
    metavar="TARGET",
    type=str,
    default="192.168.14.100",
    show_default=True,
    help='"climate_chamber" ip address (default: 192.168.14.100)',
)
@click.pass_context
def cli(
    ctx,
    lv,
    hv,
    relay_board,
    multimeter,
    lv_resource,
    hv_resource,
    relay_board_resource,
    multimeter_resource,
    verbose,
    cls=InstrumentCluster,
):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(
        lv=lv,
        hv=hv,
        relay_board=relay_board,
        multimeter=multimeter,
        lv_resource=lv_resource,
        hv_resource=hv_resource,
        relay_board_resource=relay_board_resource,
        multimeter_resource=multimeter_resource,
        reset_on_init=False,
        turn_off_on_close=False,
    )


@cli.command(
    "on",
    help=(
        'Turn all devices ON - connect "relay_board" to pin PIN, turn "lv" on at'
        'LV_VOLTAGE, LV_CURRENT, and ramp "hv" to HV_VOLTAGE.'
    ),
)
@click.argument("lv-voltage", metavar="LV_VOLTAGE", type=float)
@click.argument("lv-current", metavar="LV_CURRENT", type=float)
@click.argument("hv-voltage", metavar="HV_VOLTAGE", type=float)
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@click.option(
    "-d",
    "--hv-delay",
    metavar="SECONDS",
    type=float,
    default=1.0,
    help='Ramp delay for "hv"',
)
@click.option(
    "-s",
    "--hv-step-size",
    metavar="VOLTS",
    type=float,
    default=5.0,
    help='Step size for "hv" ramp up/down',
)
@click.option(
    "-p",
    "--relay-pin",
    metavar="PIN",
    type=str,
    default="OFF",
    help='Pin to connect on "relay_board" before turning on.',
)
@click.option(
    "-m",
    "--measure",
    is_flag=True,
    help="Measure (V, I, R, keithley_timestamp, filter) at each step.",
)
@with_instrument
@print_output
def cli_on(
    instrument,
    lv_voltage,
    lv_current,
    hv_voltage,
    lv_channel,
    hv_delay,
    hv_step_size,
    relay_pin,
    measure,
):
    return instrument.on(
        relay_pin,
        lv_channel,
        lv_voltage,
        lv_current,
        hv_voltage,
        hv_delay,
        hv_step_size,
        measure,
    )


@cli.command(
    "off",
    help=(
        'Turn all devices OFF - ramp down "hv", turn off "lv", disconnect'
        '"relay_board"'
    ),
)
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@click.option(
    "-d",
    "--hv-delay",
    metavar="SECONDS",
    type=float,
    default=1.0,
    help='Ramp delay for "hv"',
)
@click.option(
    "-s",
    "--hv-step-size",
    metavar="VOLTS",
    type=float,
    default=5.0,
    help='step size for "hv" ramp up/down',
)
@click.option(
    "-m",
    "--measure",
    is_flag=True,
    help="Measure (V, I, R, keithley_timestamp, filter) at each step.",
)
@with_instrument
@print_output
def cli_off(instrument, lv_channel, hv_delay, hv_step_size, measure):
    return instrument.off()


@cli.command(
    "lv_on", help='Turn "lv" ON with voltage LV_VOLTAGE and current LV_CURRENT'
)
@click.argument("lv-voltage", metavar="LV_VOLTAGE", type=float)
@click.argument("lv-current", metavar="LV_CURRENT", type=float)
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@with_instrument
def cli_lv_on(instrument, lv_voltage, lv_current, lv_channel):
    return instrument.lv_on(lv_channel, lv_voltage, lv_current)


@cli.command("lv_off", help='Turn "lv" OFF')
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@with_instrument
def cli_lv_off(instrument, lv_channel):
    return instrument.lv_off(channel=lv_channel)


@cli.command("hv_on", help='Turn "hv" ON and ramp to HV_VOLTAGE')
@click.argument("hv-voltage", metavar="HV_VOLTAGE", type=float)
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@click.option(
    "-d",
    "--hv-delay",
    metavar="SECONDS",
    type=float,
    default=1.0,
    help='Ramp delay for "hv"',
)
@click.option(
    "-s",
    "--hv-step-size",
    metavar="VOLTS",
    type=float,
    default=5.0,
    help='step size for "hv" ramp up/down',
)
@click.option(
    "-m",
    "--measure",
    is_flag=True,
    help="Measure (V, I, R, keithley_timestamp, filter) at each step.",
)
@with_instrument
@print_output
def cli_hv_on(instrument, lv_channel, hv_voltage, hv_delay, hv_step_size, measure):
    return instrument.hv_on(lv_channel, hv_voltage, hv_delay, hv_step_size, measure)


@cli.command("hv_off", help='Turn "hv" OFF and ramp down')
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@click.option(
    "-d",
    "--hv-delay",
    metavar="SECONDS",
    type=float,
    default=1.0,
    help='Ramp delay for "hv"',
)
@click.option(
    "-s",
    "--hv-step-size",
    metavar="VOLTS",
    type=float,
    default=5.0,
    help='step size for "hv" ramp up/down',
)
@click.option(
    "-m",
    "--measure",
    is_flag=True,
    help="Measure (V, I, R, keithley_timestamp, filter) at each step.",
)
@with_instrument
@print_output
def cli_hv_off(instrument, lv_channel, hv_delay, hv_step_size, measure):
    return instrument.hv_off(lv_channel, hv_delay, hv_step_size, measure)


@cli.command("status", help="Query output status")
@click.option(
    "-c",
    "--lv-channel",
    metavar="CHANNEL",
    type=int,
    default=1,
    help='Channel for "lv" operations',
)
@with_instrument
@print_output
def cli_status(instrument, lv_channel):
    return instrument.status(lv_channel)


@cli.command("identify", help="Identify all connected instruments.")
@with_instrument
@print_output
def cli_identify(instrument):
    return {
        "lv": instrument._lv.query("IDENTIFIER"),
        "hv": instrument._hv.query("IDENTIFIER"),
        "multimeter": instrument._multimeter.query("IDENTIFIER"),
    }
