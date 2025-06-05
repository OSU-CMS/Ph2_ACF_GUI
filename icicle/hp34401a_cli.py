"""HP34401A CLI module. See ``hp34401a --help`` for description, or read Click
decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the hp34401a module.
"""

import click
import logging
import math
import time

from .hp34401a import HP34401A
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
    default="ASRL5::INSTR",
    help="VISA resource address (default: ASRL5::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.pass_context
def cli(ctx, resource, verbose, simulate, cls=HP34401A):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)


@cli.command("reset", help="Reset instrument. Will safely ramp down and perform reset.")
@with_instrument
def cli_reset(instrument):
    return instrument.reset()


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("selftest", help="Perform instrument self-test")
@with_instrument
@print_output
def cli_selftest(instrument):
    return instrument.selftest()


@cli.command(
    "measure",
    help=(
        "Measure voltage (VOLT:DC / VOLT:AC), voltage ratio (VOLT:DC:RAT), current "
        "(CURR:DC / CURR:AC), resistance (RES), four-wire resistance (FRES), "
        "frequency (FREQ), period (PER), continuinty (CONT), or diode (DIOD). Also "
        "has pre-programmed conversions for RD53B NTC temp (RD53B_TEMP), generic NTC "
        "(ALU_TEMP), and humidity sensor (HUMIDITY)."
    ),
)
@click.argument(
    "what",
    type=click.Choice(
        (
            "VOLT",
            "CURR",
            *HP34401A.MEASURE_TYPES,
            "RD53B_TEMP",
            "ALU_TEMP",
            "HUMIDITY",
        ),
        case_sensitive=False,
    ),
)
@click.option(
    "-c",
    "--cycles",
    metavar="CYCLES",
    type=float,
    default=1.0,
    help="line integration cycles (in range 0.01 to 10; defaults to 1)",
)
@click.option(
    "-r",
    "--repetitions",
    metavar="REPETITIONS",
    type=int,
    default=1,
    help="repetitions of this measurement (defaults to 1)",
)
@click.option(
    "-d",
    "--delay",
    metavar="DELAY",
    type=float,
    default=None,
    help="delay between repetitions (seconds; requires repetitions > 1)",
)
@with_instrument
@print_output
def cli_measure(instrument, what, cycles=1.0, repetitions=1, delay=None):
    if what.upper() in ("VOLT", "CURR"):
        what = f"{what.upper()}:DC"

    if what in ("RD53B_TEMP", "ALU_TEMP"):
        what_ = "RES"
    elif what in ("HUMIDITY",):
        what_ = "VOLT:DC"
    else:
        what_ = what.upper()

    meas = instrument.measure(what_, repetitions, delay, cycles)

    if what in ("RD53B_TEMP",):
        ret = list()
        for resistance in meas:
            a = 8.676453371787721e-4
            b = 2.541035850140508e-4
            c = 1.868520310774293e-7
            logRes = math.log(resistance)
            tK = 1.0 / (a + b * logRes + c * math.pow(logRes, 3))
            ret.append(tK - 273.15)
    elif what in ("ALU_TEMP",):
        ret = list()
        for resistance in meas:
            a = 1.028632127e-3
            b = 2.451005978e-4
            c = 0.8523265167e-7
            logRes = math.log(resistance)
            tK = 1.0 / (a + b * logRes + c * math.pow(logRes, 3))
            ret.append(tK - 273.15)
    elif what in ("HUMIDITY",):
        ret = list()
        for v_out in meas:
            v_supply = 5.0
            offset = 0.16
            slope = 0.0062
            rel_humid = (v_out / v_supply - offset) / slope
            ret.append(rel_humid)
    else:
        ret = meas
    return ret


def humid_converter_factory(v_supply, slope, offset):
    def converter(voltage):
        RH = (voltage / v_supply - offset) / slope
        return RH

    return converter


def shh_converter_factory(a, b, c):
    def converter(resistance):
        logRes = math.log(resistance)
        tK = 1.0 / (a + b * logRes + c * math.pow(logRes, 3))
        return tK - 273.15

    return converter


def null_converter_factory():
    def converter(value):
        return value

    return converter


@cli.command("monitor", help="Measure WHAT every DELAY seconds.")
@click.argument(
    "what",
    type=click.Choice(
        (
            "VOLT",
            "CURR",
            *HP34401A.MEASURE_TYPES,
            "RD53B_TEMP",
            "ALU_TEMP",
            "HUMIDITY",
        ),
        case_sensitive=False,
    ),
)
@click.option(
    "-c",
    "--cycles",
    metavar="CYCLES",
    type=float,
    default=1.0,
    help="line integration cycles (in range 0.01 to 10; defaults to 1)",
)
@click.option(
    "-r",
    "--repetitions",
    metavar="REPETITIONS",
    type=int,
    default=1,
    help="repetitions of this measurement (defaults to 1)",
)
@click.option(
    "-i",
    "--instrument-delay",
    metavar="INSTRUMENT_DELAY",
    type=float,
    default=None,
    help="delay between repetitions (seconds; requires repetitions > 1)",
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
    what,
    repetitions,
    instrument_delay,
    cycles,
    delay,
    output,
    mqtt,
    broker,
    port,
    username,
    password,
    ssl,
):
    SPACING = 17
    log = MonitoringLogger(print)
    if mqtt:
        client = MQTTClient(mqtt, ssl=ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(
            lambda line, end="": instrument.publish(client, line, what, mqtt)
        )
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    log(f'# MONITORING_START {time.strftime("%x")}')

    if what.upper() in ("VOLT", "CURR"):
        what = f"{what}:DC"

    if what in ("RD53B_TEMP",):
        what_ = "RES"
        a = 8.676453371787721e-4
        b = 2.541035850140508e-4
        c = 1.868520310774293e-7
        conv = [shh_converter_factory(a, b, c)]
    elif what in ("ALU_TEMP",):
        what_ = "RES"
        a = 1.028632127e-3
        b = 2.451005978e-4
        c = 0.8523265167e-7
        conv = [shh_converter_factory(a, b, c)]
    elif what in ("HUMIDITY",):
        what_ = "VOLT:DC"
        v_supply = 5.0
        offset = 0.16
        slope = 0.0062
        conv = [humid_converter_factory(v_supply, slope, offset)]
    else:
        conv = [null_converter_factory()]
        what_ = what

    log("\t".join(val.ljust(SPACING) for val in ("# Time", what)))
    while True:
        lval = []
        val = instrument.measure(what_, repetitions, instrument_delay, cycles)
        for c in conv:
            lval.append(f"{c(val[0]):.2f}")
        log(
            "\t".join(val.ljust(SPACING) for val in (time.strftime("%x %X"), *lval)),
            end="\r",
        )
        time.sleep(delay)


# ======== LOW-LEVEL COMMANDS - Take care if using these =========


@cli.command(
    "set",
    help=(
        "Set configuration field FIELD to value VALUE (see hp34401a.py for config "
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
    "query",
    help="Query configuration field FIELD (see hp34401a.py for config table).",
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
