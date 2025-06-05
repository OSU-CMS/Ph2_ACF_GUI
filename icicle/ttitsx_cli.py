"""TTITSX CLI module. See ``tti --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the tti module.
"""

import click
import logging
import time

from .ttitsx import TTITSX
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
    envvar="ICICLE_RESOURCE",
    metavar="TARGET",
    type=str,
    default="ASRL7::INSTR",
    help="VISA resource address (default: ASRL3::INSTR)",
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
    envvar="ICICLE_TTI_OUTPUTS",
    metavar="TARGET",
    type=int,
    default=1,
    help="Number of outputs to be enabled on TTI (max 3).",
)
@click.pass_context
def cli(ctx, resource, verbose, cls=TTITSX, outputs=1, simulate=False):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, outputs=outputs, sim=simulate)


@cli.command("on", help="Turn output ON")
@with_instrument
def cli_on(instrument):
    return instrument.on()


@cli.command("off", help="Turn output OFF")
@with_instrument
def cli_off(instrument):
    return instrument.off()


@cli.command("status", help="Read output status")
@with_instrument
@print_output
def cli_status(instrument):
    return instrument.status()


@cli.command(
    "voltage",
    help=("Set VOLTAGE (V), or query current value if VOLTAGE not provided."),
)
@click.argument("voltage", metavar="VOLTAGE", type=float, required=False, default=None)
@with_instrument
@print_output
def cli_voltage(instrument, voltage):
    if voltage is None:
        return instrument.query("VOLTAGE")
    else:
        return instrument.set("VOLTAGE", voltage)


@cli.command(
    "current",
    help=("Set CURRENT (A), or query current value if " "CURRENT not provided."),
)
@click.argument("current", metavar="CURRENT", type=float, required=False, default=None)
@with_instrument
@print_output
def cli_current(instrument, current):
    if current is None:
        return instrument.query("CURRENT")
    else:
        return instrument.set("CURRENT", current)


@cli.command("reset", help="Reset instrument. Will safely ramp down and perform reset.")
@with_instrument
def cli_reset(instrument):
    return instrument.reset()


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("ramp_voltage", help="Ramp to target voltage TARGET (V).")
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
@click.option(
    "-m", "--measure", is_flag=True, default=False, help="Measure (V, I) at each step."
)
@click.option(
    "-p",
    "--power-cycle-each-step",
    is_flag=True,
    default=False,
    help="Power cycle at each step. Adds DELAY downtime during power cycle.",
)
@with_instrument
@print_output
def cli_ramp_voltage(
    instrument,
    target,
    step=5.0,
    delay=1.0,
    measure=False,
    power_cycle_each_step=False,
):
    return instrument.sweep(
        "VOLTAGE",
        target,
        delay,
        step,
        measure=measure,
        power_cycle_each_step=power_cycle_each_step,
        log_function=print,
    )


@cli.command("ramp_current", help="Ramp to target current TARGET (A).")
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
@click.option(
    "-m", "--measure", is_flag=True, default=False, help="Measure (V, I) at each step."
)
@click.option(
    "-p",
    "--power-cycle-each-step",
    is_flag=True,
    default=False,
    help="Power cycle at each step. Adds DELAY downtime during power cycle.",
)
@with_instrument
@print_output
def cli_ramp_current(
    instrument,
    target,
    step=5.0,
    delay=1.0,
    measure=False,
    power_cycle_each_step=False,
):
    return instrument.sweep(
        "CURRENT",
        target,
        delay,
        step,
        measure=measure,
        power_cycle_each_step=power_cycle_each_step,
        log_function=print,
    )


@cli.command(
    "measure",
    help=("Measure voltage (in V) and current (in A) currently outputting"),
)
@with_instrument
@print_output
def cli_measure(instrument):
    return instrument.measure()


@cli.command(
    "monitor",
    help=("Measure voltage (in V) and  current (in A) currently outputting"),
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
def cli_monitor(instrument, delay, output, mqtt, broker, port, username, password, ssl):
    SPACING = 17
    log = MonitoringLogger(print)
    if mqtt:
        client = MQTTClient(mqtt, ssl=ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(lambda line, end="": instrument.publish(client, line, mqtt))
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    log(f'# MONITORING_START {time.strftime("%x")}')
    log(
        "\t".join(
            val.ljust(SPACING) for val in ("# Time", "Voltage (V)", "Current (A)")
        )
    )
    while True:
        log(
            "\t".join(
                [
                    time.strftime("%x %X").ljust(SPACING),
                    *[str(val).ljust(SPACING) for val in instrument.measure()],
                ]
            ),
            end="\r",
        )
        time.sleep(delay)


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
