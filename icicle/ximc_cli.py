"""XIMC CLI module. See ``ximc --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the ximc_instrument module.
"""

import click
import logging

from .ximc_instrument import XimcInstrument
from .cli_utils import with_instrument, print_output, verbosity

# ================== CLI METHODS ===================
# Below are script-like functions designed to be called to replace individual python
# scripts. They generally should:
# 1. instantiate and connect to instrument without resetting it
# 2. perform action
# 3. disconnect without ramping down or terminating instrument


@click.group(invoke_without_command=True, no_args_is_help=True)
@click.option(
    "-v", "--verbose", count=True, help="Verbose output (-v = INFO, -vv = DEBUG)"
)
@click.option(
    "-R",
    "--resource",
    metavar="TARGET",
    type=str,
    default="xi-com:///dev/tty.usbmodem00007E611",
    help="XIMC resource address (default: xi-com:///dev/tty.usbmodem00007E611)",
)
@click.option(
    "-C",
    "--calibration",
    metavar="MULTIPLIER",
    type=float,
    default=None,
    help="Override calibration multiplier from steps for user units",
)
@click.option(
    "-M",
    "--microstep-mode",
    metavar="MODE_ENUM",
    type=int,
    default=None,
    help=(
        "Microstep mode to set when connecting to device (see "
        "https://libximc.xisupport.com/doc-en/ximc_8h.html#flagset_microstepmode)"
    ),
)
@click.option(
    "-U",
    "--user-units",
    metavar="UNIT",
    type=click.Choice(("mm", "um", "cm", "m", "inch", "NM")),
    default="mm",
    help=(
        "Microstep mode to set when connecting to device (see "
        "https://libximc.xisupport.com/doc-en/ximc_8h.html#flagset_microstepmode)"
    ),
)
@click.option(
    "-D", "--devices", is_flag=True, default=False, help="List devices and exit"
)
@click.pass_context
def cli(
    ctx,
    resource,
    verbose,
    calibration,
    microstep_mode,
    user_units,
    devices,
    cls=XimcInstrument,
):
    logging.basicConfig(level=verbosity(verbose))
    if devices:
        print(f'{"Index":<8}{"Resource":<40}{"Name":<20}')
        print("-" * 68)
        for device in cls.devices():
            print(f'{device["idx"]:<8}{device["resource"]:<40}{device["name"]:<20}')
        if ctx.invoked_subcommand:
            print("-" * 68)
    if ctx.invoked_subcommand:
        ctx.obj = cls(
            resource=resource,
            calibration=calibration,
            microstep_mode=microstep_mode,
            user_units=user_units,
        )


@cli.command("identify", help="Print information about device")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("calibration", help="Read calibration values from device")
@with_instrument
@print_output
def cli_calibration(instrument):
    return instrument.read_calibration()


@cli.command("microstep_mode", help="Read calibration values from device")
@click.argument(
    "microstep_mode",
    metavar="MICROSTEP_MODE",
    required=False,
    default=None,
    type=click.Choice(tuple([a.name for a in XimcInstrument.MicrostepMode])),
)
@with_instrument
@print_output
def cli_microstep_mode(instrument, microstep_mode):
    if not microstep_mode:
        return XimcInstrument.MicrostepMode(
            instrument.read_calibration()["microstep_mode"]
        ).name
    else:
        microstep_mode_ = XimcInstrument.MicrostepMode[microstep_mode]
        return XimcInstrument.MicrostepMode(
            instrument.set_microstep_mode(microstep_mode_)
        )


@cli.command("write_profile", help="Write profile file to device")
@click.argument("vendor", metavar="VENDOR", type=str, default=None, required=True)
@click.argument("device", metavar="DEVICE", type=str, default=None, required=True)
@with_instrument
def cli_write_profile(instrument, vendor, device):
    return instrument.write_profile(profile_vendor=vendor, profile_device=device)


@cli.command(
    "position",
    help=(
        "Move to position STEPS, MICROSTEPS; or query position if STEPS, MICROSTEPS "
        "not provided"
    ),
)
@click.argument("steps", metavar="STEPS", type=int, required=False, default=None)
@click.argument(
    "microsteps", metavar="MICROSTEPS", type=int, required=False, default=None
)
@click.option(
    "-r",
    "--readback",
    is_flag=True,
    default=True,
    help="Readback value after move (implies wait)",
)
@with_instrument
@print_output
def cli_position(instrument, steps, microsteps, readback):
    if steps is not None and microsteps is not None:
        return instrument.move(steps, microsteps, wait_and_readback=readback)
    else:
        return instrument.get_position()


@cli.command(
    "position_user",
    help=(
        "Move to position USER_UNITS; or query position if USER_UNITS not provided. "
        "Requires calibration to be set!"
    ),
)
@click.argument(
    "user_units", metavar="USER_UNITS", type=float, required=False, default=None
)
@click.option(
    "-r",
    "--readback",
    is_flag=True,
    default=True,
    help="Readback value after move (implies wait)",
)
@with_instrument
@print_output
def cli_position_user(instrument, user_units, readback):
    if user_units is not None:
        return instrument.move_user(user_units, wait_and_readback=readback)
    else:
        return instrument.get_position_user()


@cli.command("home", help="Home device")
@with_instrument
def cli_home(instrument):
    return instrument.home()


@cli.command("zero", help="Set device location to zero")
@with_instrument
def cli_zero(instrument):
    return instrument.zero()


@cli.command("wait", help="Wait for device to become ready")
@with_instrument
def cli_wait(instrument):
    return instrument.wait()


@cli.command(
    "movement_settings",
    help=(
        "Set SPEED, MICROSPEED, ACCELERATION, DECELERATION, ANTIPLAY_SPEED, "
        "ANTIPLAY_MICROSPEED or MOVEMENT_FLAGS; or query all if none of these is "
        "provided."
    ),
)
@click.option(
    "-s",
    "--speed",
    metavar="SPEED",
    type=int,
    required=False,
    default=None,
    help="Set speed",
)
@click.option(
    "-m",
    "--microspeed",
    metavar="MICROSPEED",
    type=int,
    required=False,
    default=None,
    help="Set microspeed",
)
@click.option(
    "-a",
    "--acceleration",
    metavar="ACCELERATION",
    type=int,
    required=False,
    default=None,
    help="Set acceleration",
)
@click.option(
    "-d",
    "--deceleration",
    metavar="DECELERATION",
    type=int,
    required=False,
    default=None,
    help="Set deceleration",
)
@click.option(
    "--antiplay_speed",
    metavar="ANTIPLAY_SPEED",
    type=int,
    required=False,
    default=None,
    help="Set antiplay speed",
)
@click.option(
    "--antiplay_microspeed",
    metavar="ANTIPLAY_MICROSPEED",
    type=int,
    required=False,
    default=None,
    help="Set antiplay microspeed",
)
@click.option(
    "-f",
    "movement_flags",
    metavar="MOVEMENT_FLAGS",
    type=int,
    required=False,
    default=None,
    help="Set movement flags",
)
@with_instrument
@print_output
def cli_movement_settings(
    instrument,
    speed,
    microspeed,
    acceleration,
    deceleration,
    antiplay_speed,
    antiplay_microspeed,
    movement_flags,
):
    if (
        speed is None
        and microspeed is None
        and acceleration is None
        and deceleration is None
        and antiplay_speed is None
        and antiplay_microspeed is None
        and movement_flags is None
    ):
        return instrument.get_movement_settings()
    else:
        return instrument.set_movement_settings(
            speed,
            microspeed,
            acceleration,
            deceleration,
            antiplay_speed,
            antiplay_microspeed,
            movement_flags,
        )
