"""Keithley2410 CLI module. See ``keithley2410 --help`` for description, or read Click
decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the keithley2410 module.
"""

import click
import logging
import time

from .keithley2410 import Keithley2410
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
    default="ASRL1::INSTR",
    help="VISA resource address (default: ASRL1::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.option(
    "-V",
    "--voltage-range",
    metavar="MIN MAX",
    type=(float, float),
    default=None,
    help="Enforce voltage range limits on device for set voltage operations.",
)
@click.pass_context
def cli(ctx, resource, verbose, simulate, voltage_range, cls=Keithley2410):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)
    if voltage_range:
        ctx.obj.set_voltage_range(*voltage_range)


@cli.command("on", help="Turn output ON")
@with_instrument
def cli_on(instrument):
    return instrument.on()


@cli.command("off", help="Turn output OFF")
@with_instrument
def cli_off(instrument):
    # Safety check - is voltage high (|V| > 5)?
    voltage = float(instrument.query("VOLTAGE"))
    if abs(voltage) > 5 and int(instrument.status()) != 0:
        resp = input(
            f"WARNING: Voltage is high (|{voltage}| > 5V). Are you sure you want to "
            "turn off without ramping down? (y/n): "
        )
        if resp.upper() not in ("Y", "YES"):
            return "Execution cancelled."
    return instrument.off()


@cli.command("status", help="Query output status")
@with_instrument
@print_output
def cli_status(instrument):
    return instrument.status()


@cli.command("panel", help="Set output to FRONt/REAR panel")
@click.argument(
    "panel",
    metavar="FRON|REAR",
    type=click.Choice(["FRON", "REAR"]),
    required=False,
    default=None,
)
@with_instrument
@print_output
def cli_panel(instrument, panel):
    if panel is None:
        return instrument.query("PANEL")
    else:
        return instrument.set("PANEL", panel)


@cli.command(
    "voltage",
    help=(
        "Set VOLTAGE (V) for output, or query current voltage if VOLTAGE not "
        "specified."
    ),
)
@click.argument(
    "voltage", metavar="[VOLTAGE]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_voltage(instrument, voltage):
    if voltage is None:
        return instrument.query("VOLTAGE")
    else:
        return instrument.set("VOLTAGE", voltage)


@cli.command(
    "voltage_range",
    help=(
        "Set VOLTAGE RANGE (V) for output, or query current voltage range if "
        "VOLTAGE_RANGE not specified."
    ),
)
@click.argument(
    "voltage_range", metavar="[VOLTAGE_RAMGE]", type=float, required=False, default=None
)
@with_instrument
@print_output
def cli_voltage_range(instrument, voltage_range):
    if voltage_range is None:
        return instrument.query("VOLTAGE_RANGE")
    else:
        return instrument.set("VOLTAGE_RANGE", voltage_range)


@cli.command(
    "compliance_current",
    help=(
        "Set compliance current to COMPLIANCE_CURRENT (A) for output, or query "
        "current compliance current if COMPLIANCE_CURRENT not specified."
    ),
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
def cli_compliance_current(instrument, compliance_current):
    if compliance_current is None:
        return instrument.query("COMPLIANCE_CURRENT")
    else:
        compliance = instrument.set("COMPLIANCE_CURRENT", compliance_current)
        crange = instrument.set("SENSE_CURRENT_RANGE", compliance_current)
        return f"Compliance: {compliance}A, Sense Current Measurement Range: {crange}"


@cli.command("reset", help="Reset instrument. Will safely ramp down and perform reset.")
@with_instrument
def cli_reset(instrument):
    return instrument.reset()


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("reset", help="Reset the instrument and clear the queue")
@with_instrument
@print_output
def cli_device_reset(instrument):
    return instrument.reset()


@cli.command("device_clear", help="Send DCL (device clear)")
@with_instrument
@print_output
def cli_device_clear(instrument):
    return instrument.device_clear()


@cli.command("selftest", help="Perform instrument self-test")
@with_instrument
@print_output
def cli_selftest(instrument):
    return instrument.selftest()


@cli.command("ramp_voltage", help="Ramp to target voltage TARGET (V).")
@click.argument("target", metavar="TARGET", type=float)
@click.option(
    "-s",
    "--step",
    metavar="STEP",
    type=float,
    default=5.0,
    help="step size (V; defaults to 5)",
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
    help="Measure (V, I, R, keithley_timestamp, filter) at each step.",
)
@click.option(
    "-n",
    "--no-log",
    is_flag=True,
    help="Disable printing (V, I, R, keithley_timestamp, filter) at each step.",
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
    """Ramp power supply to target voltage."""
    # Safety Check - is output on/off?
    if int(instrument.status()) == 0:
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
    if mqtt:
        client = MQTTClient(mqtt, ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(lambda line, end="": instrument.publish(client, line, mqtt))
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)

    instrument.set("SOURCE", "VOLT:DC")
    if measure:
        return instrument.sweep(
            "VOLTAGE", target, delay, step, measure=True, log_function=log
        )
    else:
        return instrument.sweep("VOLTAGE", target, delay, step, log_function=log)


@cli.command("measure", help="Measure voltage (V), current (I) or resistance (R)")
@click.argument(
    "what",
    metavar="V|I|R",
    type=click.Choice(
        ["V", "I", "R", "v", "i", "r", "voltage", "current", "resistance"]
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
    "-a",
    "--averages",
    metavar="N",
    type=int,
    default=10,
    help="average N measurements (in range 1 to 100; defaults to 10)",
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
@click.option(
    "-n",
    "--no-log",
    is_flag=True,
    help="Disable printing (V, I, R, keithley_timestamp, status) at each step.",
    default=False,
)
@with_instrument
@print_output
def cli_measure(
    instrument, what, cycles=1.0, averages=10, repetitions=1, delay=None, no_log=False
):
    SPACING = 17
    # if int(instrument.query('OUTPUT')) == 0:
    #    print('Measure not permitted with output off!')
    #    return -1
    if averages == 1:
        instrument.set("ENABLE_AVERAGING", "OFF")
    else:
        instrument.set("ENABLE_AVERAGING", "ON")
        instrument.set("AVERAGING_COUNT", averages)
    # if what in ['V', 'v', 'voltage']:
    instrument.set("LINE_INTEGRATION_CYCLES", "VOLT:DC", cycles)
    # instrument.set('SENSE_FUNCTION', '"VOLT"')
    # if what in ['I', 'i', 'current']:
    instrument.set("LINE_INTEGRATION_CYCLES", "CURR:DC", cycles)
    # instrument.set('SENSE_FUNCTION', '"CURR"')
    # if what in ['R', 'r', 'resistance']:
    # instrument.set('SENSE_RESISTANCE_INTEGRATION', cycles)
    # instrument.set('SENSE_FUNCTION', '"RES"')

    if no_log:
        return instrument.measure(repetitions, delay)
    else:
        return (
            "\t".join(
                val.ljust(SPACING)
                for val in (
                    "Time",
                    "Voltage (V)",
                    "Current (A)",
                    "Resistance",
                    "PSU Time",
                    "Status Register",
                )
            )
            + "\n"
            + "\t".join(
                [
                    time.strftime("%x %X").ljust(SPACING),
                    *[
                        str(val).ljust(SPACING)
                        for val in instrument.measure(repetitions, delay)
                    ],
                ]
            )
        )


@cli.command(
    "monitor",
    help="Measure voltage (V), current (I) or resistance (R) every DELAY seconds.",
)
@click.argument(
    "what",
    metavar="V|I|R",
    type=click.Choice(
        ["V", "I", "R", "v", "i", "r", "voltage", "current", "resistance"]
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
    "-c",
    "--cycles",
    metavar="CYCLES",
    type=float,
    default=1.0,
    help="line integration cycles (in range 0.01 to 10; defaults to 1)",
)
@click.option(
    "-a",
    "--averages",
    metavar="N",
    type=int,
    default=10,
    help="average N measurements (in range 1 to 100; defaults to 10)",
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
    delay,
    cycles,
    averages,
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
        client = MQTTClient(mqtt, ssl)
        client.connect(broker, port=port, username=username, password=password)
        client.loop_start()
        log.register_writer(lambda line, end="": instrument.publish(client, line, mqtt))
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    if averages == 1:
        instrument.set("SENSE_AVERAGING", "OFF")
    else:
        instrument.set("SENSE_AVERAGING", "ON")
        instrument.set("SENSE_AVERAGING_COUNT", averages)
    # if what in ['V', 'v', 'voltage']:
    instrument.set("SENSE_VOLTAGE_INTEGRATION", cycles)
    # instrument.set('SENSE_FUNCTION', '"VOLT"')
    # if what in ['I', 'i', 'current']:
    instrument.set("SENSE_CURRENT_INTEGRATION", cycles)
    # instrument.set('SENSE_FUNCTION', '"CURR"')
    # if what in ['R', 'r', 'resistance']:
    # instrument.set('SENSE_RESISTANCE_INTEGRATION', cycles)
    # instrument.set('SENSE_FUNCTION', '"RES"')
    log(f'# MONITORING_START {time.strftime("%x")}')
    log(f'# PANEL {instrument.query("PANEL")}')
    log(
        "\t".join(
            val.ljust(SPACING)
            for val in (
                "# Time",
                "Voltage (V)",
                "Current (A)",
                "Resistance",
                "PSU Time",
                "Status Register",
            )
        )
    )
    while True:
        if int(instrument.query("OUTPUT")) == 0:
            # log('# Measure not permitted with output off!', end='\r')
            log(
                "\t".join(
                    [
                        time.strftime("%x %X").ljust(SPACING),
                        "0.0",
                        "0.0",
                        "nan",
                        "nan",
                        "nan",
                    ]
                ),
                end="\r",
            )
        else:
            log(
                "\t".join(
                    [
                        time.strftime("%x %X").ljust(SPACING),
                        *[
                            str(val).ljust(SPACING)
                            for val in instrument.measure_single()
                        ],
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
