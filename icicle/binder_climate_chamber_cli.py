"""Binder CLI module. See ``binder --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the binder module.
"""

import click
import logging
import time
import random

from .binder_climate_chamber import Binder
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
    default="192.168.14.100",
    help="IP address (default: 192.168.14.100)",
)
@click.pass_context
def cli(ctx, resource, verbose, cls=Binder):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource)


@cli.command("on", help="Disable idle mode.")
@with_instrument
@print_output
def cli_on(instrument):
    return instrument.toggle_idle(enable=False)


@cli.command("off", help="Set to idle mode.")
@with_instrument
@print_output
def cli_off(instrument):
    return instrument.toggle_idle(enable=True)


@cli.command("switch_on", help="Turn on a custom switch (1-4)")
@click.argument("switch", metavar="SWITCH", type=click.Choice(("1", "2", "3", "4")))
@with_instrument
@print_output
def cli_switch_on(instrument, switch):
    shift = 1 + int(switch)
    shift = 1 << shift
    return instrument.toggle_switch(bit=shift, on=True)


@cli.command("switch_off", help="Turn off a custom switch (1-4)")
@click.argument("switch", metavar="SWITCH", type=click.Choice(("1", "2", "3", "4")))
@with_instrument
@print_output
def cli_switch_off(instrument, switch):
    shift = 1 + int(switch)
    shift = 1 << shift
    return instrument.toggle_switch(bit=shift, on=False)


@cli.command("set_temp", help="Set a specific temperature")
@click.argument("temperature", metavar="CELSIUS", type=float)
@with_instrument
def cli_set_temp(instrument, temperature):
    return instrument.set_temperature(temperature)


@cli.command("set_humid", help="Set a target relative humidity")
@click.argument("humidity", metavar="PERCENT", type=float)
@with_instrument
def cli_set_humid(instrument, humidity):
    return instrument.set_humidity(humidity)


@cli.command("status", help="Get status information about climate chamber.")
@with_instrument
@print_output
def cli_status(instrument):
    output = []
    status = {"1": "Idle", "0": "Active"}
    output.append(f"status: {status[str(instrument.check_idle())]}")
    for keys, values in instrument.getVals().items():
        output.append(f"{keys}: {values}")
    return "\n".join(output)


@cli.command(
    "measure", help="Get the temperature and humidity values from the climate chamber."
)
@with_instrument
@print_output
def cli_model(instrument):
    return instrument.getVals()


@cli.command(
    "probe_temperature", help="probe the temperature and wait for it to reach a target"
)
@click.option(
    "-t",
    "--target",
    metavar="CELSIUS",
    type=float,
    default=20,
    help="Target temperature to probe for",
)
@click.option(
    "-i",
    "--interval",
    metavar="SECONDS",
    type=int,
    default=1,
    required=False,
    help="Interval in which to probe (in seconds)",
)
@click.option(
    "-v",
    "--verbose",
    metavar="BOOL",
    is_flag=True,
    default=True,
    help="Determines, if the probed temperature should be printed out",
)
@with_instrument
@print_output
def cli_probe(instrument, target, interval, verbose):
    return instrument.probe_temperature(target, interval, verbose)


@cli.command(
    "monitor",
    help="Measure target and set temperature and rel. humidity every DELAY seconds.",
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
    is_flag=True,
    default=False,
    help="send measured values to MQTT server.",
)
# TODO: make clientID uniqueness not rely on randomness
@click.option(
    "--clientid",
    metavar="CLIENTID",
    type=str,
    default=(f"binder{random.randint(1000, 9999)}"),
    help="unique client ID of MQTT client",
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
@with_instrument
def cli_monitor(
    instrument, delay, output, mqtt, clientid, broker, port, username, password
):
    SPACING = 17
    log = MonitoringLogger(print)

    if mqtt:
        client = MQTTClient(clientid)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(lambda line, end="": instrument.publish(client, line))
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    log(f'# MONITORING_START {time.strftime("%x")}')
    log("\t".join(val.ljust(SPACING) for val in ["#"] + instrument.MONITOR_KEYS))
    while True:
        log(
            "\t".join(
                [
                    time.strftime("%x %X").ljust(SPACING),
                    *[
                        str(val).ljust(SPACING)
                        for key, val in instrument.getVals().items()
                    ],
                ]
            ),
            end="\r",
        )
        time.sleep(delay)


# ======== LOW-LEVEL COMMANDS - Take care if using these =========

# To implement
# @cli.command('command', help=('Send command. [LOW-LEVEL COMMAND; use only if you '
#   'really know what you\'re doing.]'))
# @click.argument('string', metavar='STRING', type=str, required=True)
# @with_instrument
# @print_output
# def cli_comamnd(instrument, string):
#    return instrument._instrument.query(string)
