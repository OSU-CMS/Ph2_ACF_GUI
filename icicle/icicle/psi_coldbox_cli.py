import click
import logging

from .psi_coldbox import PSIColdbox
from .cli_utils import with_instrument, print_output, verbosity


@click.group()
@click.option(
    "-v", "--verbose", count=True, help="Verbose output (-v = INFO, -vv = DEBUG)"
)
@click.option(
    "-H",
    "--host",
    metavar="TARGET",
    type=str,
    default="some.mqtt.server",
    show_default=True,
    help="coldbox IP address or host name",
)
@click.option(
    "-P",
    "--port",
    metavar="TARGET",
    type=int,
    default=1883,
    help="coldbox MQTT port",
    show_default=True,
)
@click.option(
    "-u",
    "--username",
    metavar="TARGET",
    type=str,
    default="",
    show_default=True,
    help="Username",
)
@click.option(
    "-p",
    "--password",
    metavar="TARGET",
    type=str,
    default="",
    show_default=True,
    help="Password",
)
@click.option(
    "-i",
    "--init",
    metavar="INIT",
    type=bool,
    default=False,
    show_default=True,
    help="Initalize with std config",
)
@click.pass_context
def cli(ctx, host, verbose, port, username, password, init, cls=PSIColdbox):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(
        resource=f"TCPIP::{host}::{port}::SOCKET",
        off_on_disconnect=False,
        username=username,
        password=password,
        init=init,
    )


@cli.command("on", help="on [CHANNEL]")
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_on(instrument, channel):
    return instrument.on(channel=channel)


@cli.command("off", help="off [CHANNEL]")
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_off(instrument, channel):
    return instrument.off(channel=channel)


@cli.command("reboot", help="reboot [CHANNEL]")
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_reboot(instrument, channel):
    return instrument.reboot(channel=channel)


@cli.command(
    "clear",
    hidden=True,
    help=("Send 'cmd ClearError tec [CHANNEL]'"),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_clear_error(instrument, channel):
    return instrument.clear_error(channel=channel)


@cli.command(
    "save",
    help=("Save PID variables to flash memory [for CHANNEL]."),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
# @click.option("-i", "--icicle", is_flag=True, help="Save PID values to ICICLE memory")
@with_instrument
# def cli_save_variables(instrument, channel, icicle):
def cli_save_variables(instrument, channel):
    # if icicle:
    #    instrument.save_variables_icicle(channel=channel)
    # else:
    instrument.save_variables_on_tec(channel=channel)


@cli.command(
    "load",
    help=("Load PID variables from flash memory [for CHANNEL]."),
)
@click.option("-i", "--icicle", is_flag=True, help="Load PID values from ICICLE")
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@with_instrument
def cli_load_variables(instrument, channel, icicle):
    if icicle:
        instrument.load_variables_from_icicle(channel=channel)
    else:
        instrument.load_variables_from_tec(channel=channel)


@cli.command(
    "exec",
    hidden=True,
    help=("Send string 'COMMAND' to coldbox"),
)
@click.argument("command", metavar="COMMAND", type=str)
@print_output
@with_instrument
def cli_exec(instrument, command):
    instrument.execute(command)


@cli.command(
    "set",  # hidden=True,
    help=("set FIELD VALUE [CHANNEL]"),
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("value", metavar="VALUE", type=str)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_set(instrument, field, value, channel):
    return instrument.set(field, value, channel)


@cli.command(
    "query",
    help=("query FIELD [CHANNEL]"),
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_query(instrument, field, channel):
    return instrument.query(field, channel)


@cli.command(
    "read",
    help=("read FIELD [CHANNEL]"),
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_read(instrument, field, channel):
    return instrument.read(field, channel)


@cli.command(
    "temp",
    help=(
        "Read temperature currently set for CHANNEL (without VALUE argument), "
        'or set TEMP [C] for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="TEMP", type=str, required=False)
@print_output
@with_instrument
def cli_temp(instrument, value, channel):
    key = "temperature_set"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "mode",
    help=(
        "Read Mode currently set for CHANNEL (without VALUE argument), "
        'or set Mode (0=T, 1=V) for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_mode(instrument, value, channel):
    key = "mode"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "kp",
    help=(
        "Read PID_kp currently set for CHANNEL (without VALUE argument), "
        'or set PID_kp for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_kp(instrument, value, channel):
    key = "PID_kp"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "ki",
    help=(
        "Read PID_ki currently set for CHANNEL (without VALUE argument), "
        'or set PID_ki for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_ki(instrument, value, channel):
    key = "PID_ki"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "kd",
    help=(
        "Read PID_kd currently set for CHANNEL (without VALUE argument), "
        'or set PID_kd for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_kd(instrument, value, channel):
    key = "PID_kd"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "version",
    help=("Read TEC firmware version [for CHANNEL]"),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@print_output
@with_instrument
def cli_version(instrument, channel):
    # instrument.execute("cmd GetSWVersion tec {channel}")
    return instrument.query("SOFTWARE_VERSION", channel)


@cli.command(
    "voltage",
    help=(
        "Read ControlVoltage_Set [V] currently set for CHANNEL (without VALUE "
        'argument), or set ControlVoltage_Set [V] for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_voltage(instrument, value, channel):
    key = "VOLTAGE"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "flush",
    help=("Read status of FLUSH or set FLUSH status with VALUE."),
)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_flush(instrument, value):
    key = "FLUSH"
    if value is None:
        return instrument.query(key)
    else:
        return instrument.set(key, value)


@cli.command(
    "rinse",
    help=("Read status of RINSE or set RINSE status with VALUE."),
)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_rinse(instrument, value):
    key = "RINSE"
    if value is None:
        return instrument.query(key)
    else:
        return instrument.set(key, value)


@cli.command(
    "throttle",
    help=("Set 'throttle N2' state with VALUE."),
)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_throttle(instrument, value):
    key = "THROTTLE_N2"
    if value is None:
        return instrument.read(key)
    else:
        return instrument.set(key, value)


@cli.command(
    "power",
    help=(
        "Read TEC on/off state for CHANNEL (without VALUE argument), "
        'or set TEC on/off state for CHANNEL. CHANNEL = 0 = "ALL".'
    ),
)
@click.argument("channel", metavar="CHANNEL", default="ALL", required=False)
@click.argument("value", metavar="VALUE", type=str, required=False)
@print_output
@with_instrument
def cli_power(instrument, value, channel):
    key = "POWER"
    if value is None:
        return instrument.query(key, channel)
    else:
        return instrument.set(key, value, channel)


@cli.command(
    "voltage_probe",
    help=(
        "Read voltages for all pins on probe card on CHANNEL. "
        "Returns -999 on readout error"
    ),
)
@click.argument("channel", metavar="CHANNEL", required=True)
@print_output
@with_instrument
def cli_vprobe(instrument, channel):
    key = "VOLTAGE_PROBE"
    return instrument.query(key, channel)
