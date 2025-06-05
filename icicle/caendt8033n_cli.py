import time

import click
import logging

from .caendt8033n import CaenDT8033N
from .cli_utils import with_instrument, print_output, verbosity, MonitoringLogger
from .mqtt_client import MQTTClient


@click.group()
@click.option(
    "-v", "--verbose", count=True, help="Verbose output (-v = INFO, -vv = DEBUG)"
)
@click.option(
    "-R",
    "--resource",
    metavar="TARGET",
    type=str,
    default="ASRL12::INSTR",
    help="VISA resource address (default: ASRL12::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.pass_context
def cli(ctx, resource, verbose, simulate, cls=CaenDT8033N):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)


@cli.command("on", help="Turn output ON")
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("1", "2", "3", "4", "5", "6", "7", "8", "ALL")),
)
@with_instrument
def cli_on(instrument, channel):
    return instrument.on(channel)


@cli.command("off", help="Turn output OFF")
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("1", "2", "3", "4", "5", "6", "7", "8", "ALL")),
)
@with_instrument
def cli_off(instrument, channel):
    return instrument.off(channel)


@cli.command("status", help="Query output status")
@click.argument("channel", metavar="CHANNEL", required=True, default=None)
@with_instrument
@print_output
def cli_status(instrument, channel):
    return instrument.status(channel)


@cli.command("state", help="Query output state (ON / OFF)")
@click.argument("channel", metavar="CHANNEL", required=True, default=None)
@with_instrument
@print_output
def cli_state(instrument, channel):
    return instrument.state(channel)


@cli.command(
    "voltage",
    help=(
        "Set VOLTAGE (V) for output, or query current voltage if VOLTAGE not "
        "specified."
    ),
)
@click.argument("channel", metavar="CHANNEL", required=True, default=None)
@click.argument(
    "voltage", metavar="[VOLTAGE]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_voltage(instrument, channel, voltage):
    if channel == "ALL":
        if voltage is None:
            return instrument.query("VOLTAGE")
        else:
            return instrument.set("VOLTAGE", voltage)
    else:
        if voltage is None:
            return instrument.query_channel("VOLTAGE", channel)
        else:
            return instrument.set_channel("VOLTAGE", channel, voltage)


@cli.command("reset", help="Reset instrument. Will safely ramp down and perform reset.")
@with_instrument
def cli_reset(instrument):
    instrument.__exit__(recover_attempt=False)
    instrument.__enter__(recover_attempt=False)
    return True


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command(
    "compliance_current",
    help=(
        "Set compliance current to COMPLIANCE_CURRENT (A) for output, or query "
        "current compliance current if COMPLIANCE_CURRENT not specified."
    ),
)
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("1", "2", "3", "4", "5", "6", "7", "8", "ALL")),
)
@click.argument(
    "compliance_current",
    metavar="[COMPLIANCE_CURRENT]",
    type=float,
    required=False,
    default=None,
)
@with_instrument
@print_output
def cli_compliance_current(instrument, channel, compliance_current):
    if channel == "ALL":
        if compliance_current is None:
            return instrument.query("COMPLIANCE_CURRENT")
        else:
            compliance = instrument.set("COMPLIANCE_CURRENT", compliance_current)
            return "Compliance: {:.2g}A".format(compliance)
    else:
        instrument.validate_channel(channel)
        if compliance_current is None:
            return instrument.query_channel("COMPLIANCE_CURRENT", channel)
        else:
            compliance = instrument.set_channel(
                "COMPLIANCE_CURRENT", channel, compliance_current
            )
            return "Compliance: {:.2g}A".format(compliance)


@cli.command(
    "set",
    help="Set configuration field FIELD to value VALUE"
    + "(see caendt8033n.py for config table).",
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("value", metavar="VALUE", type=str)
@with_instrument
@print_output
def cli_set(instrument, field, value):
    return instrument.set(field, value)


@cli.command(
    "query",
    help="Query configuration field FIELD (see caendt8033n.py for config table).",
)
@click.option(
    "-C",
    "--channel",
    metavar="CHANNEL",
    type=int,
    default=0,
    help="Select channel",
)
@click.argument("field", metavar="FIELD", type=str)
@with_instrument
@print_output
def cli_query(instrument, field, channel):
    if channel == "ALL":
        return instrument.query(field)
    else:
        return instrument.query_channel(field, channel)


@cli.command("ramp_voltage", help="Ramp to target voltage TARGET (V).")
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("1", "2", "3", "4", "5", "6", "7", "8", "ALL")),
)
@click.argument("target", metavar="TARGET", type=float)
@click.option(
    "-s",
    "--step",
    metavar="STEP",
    type=float,
    default=10.0,
    help="step size (V; defaults to 10)",
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
    "--measure",
    is_flag=True,
    help="Measure (V, I, timestamp, filter) at each step.",
)
@click.option(
    "-n",
    "--no-log",
    is_flag=True,
    help="Disable printing (V, I, timestamp, filter) at each step.",
    default=False,
)
@click.option(
    "-o",
    "--output",
    metavar="FILEPATH",
    type=click.File("a"),
    default=None,
    required=False,
    help="Output file to log values to in tab-separated human-readable format",
)
@click.option(
    "-m",
    "--mqtt",
    metavar="MQTT",
    envvar="ICICLE_MQTT_TOPIC",
    type=str,
    default=None,
    help="Topic (and unique client id) for this monitoring instance for MQTT server.",
)
@click.option(
    "--broker",
    metavar="BROKER",
    type=str,
    default="localhost",
    help="address of MQTT broker",
)
@click.option(
    "--port", metavar="PORT", type=int, default=1883, help="port of MQTT broker"
)
@click.option(
    "--username",
    metavar="USERNAME",
    type=str,
    default=None,
    help="username for MQTT server",
)
@click.option(
    "--password",
    metavar="PASSWORD",
    type=str,
    default=None,
    help="password for MQTT server",
)
@click.option(
    "--ssl",
    envvar="ICICLE_MQTT_SSL",
    is_flag=True,
    default=False,
    help="use SSL/TLS transport-level security for MQTT",
)
@with_instrument
@print_output
def cli_ramp_voltage(
    instrument,
    channel,
    target,
    step=5.0,
    delay=1.0,
    measure=False,
    no_log=False,
    output=None,
    mqtt=None,
    broker="localhost",
    port=1883,
    username=None,
    password=None,
    ssl=False,
):
    if int(instrument.state(channel)) == 0:
        ret = input("WARNING: Output is OFF - are you sure you want to ramp? (y/n): ")
        if ret.upper() not in ("Y", "YES"):
            return "Execution cancelled."
        else:
            if not no_log:
                print("Cannot measure with output off - disabling logging...")
                no_log = True

    log = MonitoringLogger()
    if not no_log:
        log.register_writer(print)

    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    if mqtt:
        client = MQTTClient(mqtt, ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(
            lambda line, end="": instrument.publish(client, line, channel, mqtt)
        )
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)

    if measure:
        return instrument.sweep(
            "VOLTAGE", channel, target, delay, step, measure=True, log_function=log
        )
    else:
        return instrument.sweep(
            "VOLTAGE", channel, target, delay, step, measure=measure, log_function=log
        )


@cli.command("measure", help="Measure voltage (V), current (I) or resistance (R)")
@click.option(
    "-n",
    "--no-log",
    is_flag=True,
    help="Disable printing (V, I, R, keithley_timestamp, status) at each step.",
    default=False,
)
@with_instrument
@print_output
def cli_measure(instrument, no_log=False):
    SPACING = 1
    # if int(instrument.query('OUTPUT')) == 0:
    #    print('Measure not permitted with output off!')
    #    return -1
    if no_log:
        return instrument.measure("ALL")
    else:
        header = instrument.sweep_print_header()
        return (
            f"{header[0].ljust(SPACING+15)}\t"
            + "\t".join(val.ljust(SPACING) for val in header[1:])
            + "\n"
            + "\t".join(
                [
                    time.strftime("%x %X").ljust(SPACING),
                    *[str(val).ljust(SPACING) for val in instrument.measure("ALL")],
                ]
            )
        )


@cli.command(
    "monitor",
    help="Measure voltage (V), current (I) or resistance (R) every DELAY seconds.",
)
@click.option(
    "-r",
    "--repeats",
    metavar="INT",
    type=int,
    default=0,
    help="Number of repeats of the monitoring. infinite if 0",
)
@click.option(
    "-d",
    "--delay",
    metavar="DELAY",
    type=float,
    default=1.0,
    help="delay between measurements (seconds; defaults to 1)",
)
@click.option(
    "-o",
    "--output",
    metavar="FILEPATH",
    type=click.File("a"),
    default=None,
    required=False,
    help="append monitoring output to file",
)
@click.option(
    "-m",
    "--mqtt",
    metavar="MQTT",
    envvar="ICICLE_MQTT_TOPIC",
    type=str,
    default=None,
    help="Topic (and unique client id) for this monitoring instance for MQTT server.",
)
@click.option(
    "--broker",
    metavar="BROKER",
    envvar="ICICLE_MQTT_BROKER",
    type=str,
    default="localhost",
    help="address of MQTT broker",
)
@click.option(
    "--port",
    metavar="PORT",
    envvar="ICICLE_MQTT_PORT",
    type=int,
    default=1883,
    help="port of MQTT broker",
)
@click.option(
    "--username",
    metavar="USERNAME",
    envvar="ICICLE_MQTT_USERNAME",
    type=str,
    default=None,
    help="username for MQTT server",
)
@click.option(
    "--password",
    metavar="PASSWORD",
    envvar="ICICLE_MQTT_PASSWORD",
    type=str,
    default=None,
    help="password for MQTT server",
)
@click.option(
    "--ssl",
    envvar="ICICLE_MQTT_SSL",
    is_flag=True,
    default=False,
    help="use SSL/TLS transport-level security for MQTT",
)
@with_instrument
def cli_monitor(
    instrument,
    repeats,
    delay,
    output,
    mqtt,
    broker,
    port,
    username,
    password,
    ssl,
):
    SPACING = 1
    log = MonitoringLogger(print)

    if mqtt:
        client = MQTTClient(mqtt, ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(
            lambda line, end="": instrument.publish(client, line, "ALL", mqtt)
        )
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    log(f'# MONITORING_START {time.strftime("%x")}')
    header = instrument.sweep_print_header()
    log(
        f"#{header[0].ljust(SPACING+15)}\t"
        + "\t".join(val.ljust(SPACING) for val in header[1:])
    )
    cntr = 0
    while repeats == 0 or cntr < repeats:
        log(
            "\t".join(
                [
                    time.strftime("%x %X").ljust(SPACING),
                    *[str(val).ljust(SPACING) for val in instrument.measure("ALL")],
                ]
            ),
            end="\r",
        )
        time.sleep(delay)
        cntr += 1
