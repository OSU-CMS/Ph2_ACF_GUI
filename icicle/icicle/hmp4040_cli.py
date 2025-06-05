"""HMP4040 CLI module. See ``hmp4040 --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the hmp4040 module.
"""

import click
import logging
import time

from .hmp4040 import HMP4040
from .cli_utils import with_instrument, print_output, verbosity, MonitoringLogger
from .mqtt_client import MQTTClient

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
    default="ASRL2::INSTR",
    help="VISA resource address (default: ASRL2::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.option(
    "-O",
    "--outputs",
    envvar="ICICLE_HMP4040_OUTPUTS",
    metavar="TARGET",
    type=int,
    default=4,
    help="Number of outputs to be enabled on HMP4040 (max 4).",
)
@click.pass_context
def cli(ctx, resource, verbose, cls=HMP4040, outputs=4, simulate=False):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, outputs=outputs, sim=simulate)


@cli.command("on", help="Turn general output ON")
@with_instrument
@print_output
def cli_on(instrument):
    return instrument.set("OUTPUT_GENERAL", 1)


@cli.command("off", help="Turn general output OFF")
@with_instrument
@print_output
def cli_off(instrument):
    return instrument.set("OUTPUT_GENERAL", 0)


@cli.command("status", help="Read general output state (ON/OFF)")
@with_instrument
@print_output
def cli_status(instrument):
    return (
        "\t\t".join(("OUTPUT", "Ch1", "Ch2", "Ch3", "Ch4"))
        + "\n"
        + "\t".join(
            ["ON\t" if int(instrument.query("OUTPUT_GENERAL")) else "off\t"]
            + [
                (
                    "ACTIVATED"
                    if int(instrument.query_channel("OUTPUT_SELECT", int(channel)))
                    else "deactivated"
                )
                for channel in (1, 2, 3, 4)
            ]
        )
    )


@cli.command("activate", help="Activate output for channel CHANNEL (1, 2, 3, or 4)")
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@with_instrument
def cli_activate(instrument, channel):
    return instrument.set_channel("OUTPUT_SELECT", int(channel), 1)


@cli.command("deactivate", help="Deactivate output for channel CHANNEL (1, 2, 3 or 4)")
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@with_instrument
@print_output
def cli_deactivate(instrument, channel):
    return instrument.set_channel("OUTPUT_SELECT", int(channel), 0)


@cli.command(
    "activated", help="Read output activation status for channel CHANNEL (1, 2, 3 or 4)"
)
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("1", "2", "3", "4")),
    default=None,
    required=False,
)
@with_instrument
@print_output
def cli_activated(instrument, channel):
    if channel:
        return instrument.query_channel("OUTPUT_SELECT", int(channel))
    else:
        return (
            "\t\t".join(("OUTPUT", "Ch1", "Ch2", "Ch3", "Ch4"))
            + "\n"
            + "\t".join(
                ["ON\t" if int(instrument.query("OUTPUT_GENERAL")) else "off\t"]
                + [
                    (
                        "ACTIVATED"
                        if int(instrument.query_channel("OUTPUT_SELECT", int(channel)))
                        else "deactivated"
                    )
                    for channel in (1, 2, 3, 4)
                ]
            )
        )


@cli.command(
    "voltage",
    help=(
        "Read voltage currently set for channel CHANNEL (without VOLTAGE argument), "
        "or set VOLTAGE (V) for channel CHANNEL (1, 2, 3, or 4)"
    ),
)
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@click.argument(
    "voltage", metavar="[VOLTAGE]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_voltage(instrument, channel, voltage):
    if voltage is None:
        return instrument.query_channel("VOLTAGE", int(channel))
    else:
        return instrument.set_channel("VOLTAGE", int(channel), voltage)


@cli.command(
    "ovp",
    help=(
        "Read overvoltage protection currently set for channel CHANNEL (without OVP "
        "argument), or set OVP (V) for channel CHANNEL (1, 2, 3, or 4)"
    ),
)
@click.argument("channel", metavar="[CHANNEL]", type=click.Choice(("1", "2", "3", "4")))
@click.argument("ovp", metavar="OVP", type=float, required=False, default=None)
@with_instrument
@print_output
def cli_ovp(instrument, channel, ovp):
    if ovp is None:
        return instrument.query_channel("OVP", int(channel))
    else:
        return instrument.set_channel("OVP", int(channel), ovp)


@cli.command(
    "current",
    help=(
        "Read current currently set for channel CHANNEL (without CURRENT argument), "
        "or set CURRENT (A) for channel CHANNEL (1, 2, 3, or 4)"
    ),
)
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@click.argument(
    "current", metavar="[CURRENT]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_current(instrument, channel, current):
    if current is None:
        return instrument.query_channel("CURRENT", int(channel))
    else:
        return instrument.set_channel("CURRENT", int(channel), current)


@cli.command("reset", help="Reset instrument. Will safely turn off and perform reset.")
@with_instrument
def cli_reset(instrument):
    return instrument.reset()


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("ramp_voltage", help="Ramp channel CHANNEL to target voltage TARGET (V).")
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@click.argument("target", metavar="TARGET", type=float)
@click.option(
    "-s",
    "--step",
    metavar="STEP",
    type=float,
    default=1.0,
    help="step size (V; defaults to 1)",
)
@click.option(
    "-d",
    "--delay",
    metavar="DELAY",
    type=float,
    default=1.0,
    help="settling time between steps (seconds; defaults to 1)",
)
@click.option("-m", "--measure", is_flag=True, help="Measure (V, I) at each step.")
@with_instrument
@print_output
def cli_ramp_voltage(instrument, channel, target, step=5.0, delay=1.0, measure=False):
    if not instrument.set("INSTRUMENT", f"{channel}"):
        raise RuntimeError(f"Could not select channel {channel} for sweep.")
    return instrument.sweep(
        "VOLTAGE",
        target,
        delay,
        step,
        measure=measure,
        measure_args={"channel": int(channel)},
    )


@cli.command("ramp_current", help="Ramp channel CHANNEL to target current TARGET (A).")
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@click.argument("target", metavar="TARGET", type=float)
@click.option(
    "-s",
    "--step",
    metavar="STEP",
    type=float,
    default=0.1,
    help="step size (A; defaults to 0.1)",
)
@click.option(
    "-d",
    "--delay",
    metavar="DELAY",
    type=float,
    default=1.0,
    help="settling time between steps (seconds; defaults to 1)",
)
@click.option("-m", "--measure", is_flag=True, help="Measure (V, I) at each step.")
@with_instrument
@print_output
def cli_ramp_current(instrument, channel, target, step=5.0, delay=1.0, measure=False):
    if not instrument.set("INSTRUMENT", f"{channel}"):
        raise RuntimeError(f"Could not select channel {channel} for sweep.")
    return instrument.sweep(
        "CURRENT",
        target,
        delay,
        step,
        measure=measure,
        measure_args={"channel": int(channel)},
    )


@cli.command(
    "measure",
    help=(
        "Measure voltage (in V) and current (in A) currently outputting on channel "
        "CHANNEL (1, 2, 3, or 4)"
    ),
)
@click.argument("channel", metavar="CHANNEL", type=click.Choice(("1", "2", "3", "4")))
@with_instrument
@print_output
def cli_measure(instrument, channel):
    return instrument.measure(int(channel))


@cli.command(
    "monitor",
    help=(
        "Measure voltage (in V) and current (in A) currently outputting on all "
        "channels every DELAY seconds"
    ),
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
    instrument, delay, mqtt, broker, port, username, password, ssl, output=None
):
    LONG_SPACING = 17
    log = MonitoringLogger(print)
    if mqtt:
        client = MQTTClient(mqtt, ssl=ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(
            lambda line, end="": instrument.publish(
                client, line, channel="ALL", topic=mqtt
            )
        )
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    log(f'# MONITORING_START {time.strftime("%x")}')
    log(
        "\t".join(
            val.ljust(LONG_SPACING)
            for val in (
                "# Time",
                "Ch1 V\tCh1 A",
                "Ch2 V\tCh2 A",
                "Ch3 V\tCh3 A",
                "Ch4 V\tCh4 A",
            )
        )
    )
    while True:
        measurements = []
        for channel in (1, 2, 3, 4):
            measurements.append(instrument.measure(int(channel)))
        log(
            "\t".join(
                [time.strftime("%x %X").ljust(LONG_SPACING)]
                + [
                    f"{str(volt)}\t{str(curr)}".ljust(LONG_SPACING)
                    for volt, curr in measurements
                ]
            )
            + "\t",
            end="\r",
        )
        time.sleep(delay)


# ======== LOW-LEVEL COMMANDS - Take care if using these =========


@cli.command(
    "set",
    help=(
        "Set configuration field FIELD to value VALUE (see hmp4040.py for config "
        "table)."
    ),
)
@click.argument("field", metavar="FIELD", type=str)
@click.argument("value", metavar="VALUE", type=str)
@with_instrument
@print_output
def cli_set(instrument, field, value):
    return instrument.set(field, value)


@cli.command(
    "query", help="Query configuration field FIELD (see hmp4040.py for config table)."
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
