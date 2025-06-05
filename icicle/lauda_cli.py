"""Lauda CLI module. See ``lauda --help`` for description, or read Click decorators.

This module is not explicitly documented here, as it is expected the CLI self-
documentation should be sufficient.

See also the lauda module.
"""

import click
import logging
import time

from .lauda import Lauda
from .cli_utils import with_instrument, print_output, verbosity, MonitoringLogger

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
    default="ASRL11::INSTR",
    help="VISA resource address (default: ASRL11::INSTR)",
)
@click.option(
    "-S",
    "--simulate",
    is_flag=True,
    help="Use pyvisa_sim backend as simulated instrument.",
)
@click.pass_context
def cli(ctx, resource, verbose, simulate, cls=Lauda):
    logging.basicConfig(level=verbosity(verbose))
    ctx.obj = cls(resource=resource, sim=simulate)


@cli.command("identify", help="Identify instrument")
@with_instrument
@print_output
def cli_identify(instrument):
    return instrument.identify()


@cli.command("on", help="Start the chiller.")
@with_instrument
@print_output
def cli_on(instrument):
    return instrument._instrument.query("START")


@cli.command("off", help="Stop the chiller.")
@with_instrument
@print_output
def cli_off(instrument):
    return instrument._instrument.query("STOP")


@cli.command("status", help="Get the chiller status.")
@with_instrument
@print_output
def cli_status(instrument):
    output = []
    status = {0: "OK", -1: "error"}
    output.append(f'status: {status[instrument.query("STATUS")]}')
    operation = {0: "on", 1: "off"}
    output.append(f'operation: {operation[instrument.query("OPERATION")]}')
    return "\n".join(output)


@cli.command("diagnostics", help="Get the chiller diagnostics information.")
@with_instrument
@print_output
def cli_diagnostics(instrument):
    diagnostics = [
        "error:",
        "alarm:",
        "warning:",
        "overtemperature:",
        "low level:",
        "<unknown>:",
        "external control missing:",
    ]
    return "\n".join(
        [
            f"{diagnostics[i]:<25}" + "\t" + str(bool(int(x)))
            for i, x in enumerate(str(instrument.query("DIAGNOSTICS")))
        ]
    )


@cli.command("temperature", help="Get or set the bath/external/target temperature.")
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("bath", "external", "target")),
    required=False,
    default=None,
)
@click.argument("value", metavar="VALUE", type=float, required=False, default=None)
@with_instrument
@print_output
def cli_temperature(instrument, channel, value):
    reference = {"bath": 0, "external": 1, "ext_analog": 2, "ext_serial": 3}
    output = ""
    if channel == "bath" or channel is None:
        if value is None:
            output += "bath temperature =\t"
            output += f'{float(instrument.query("TEMPERATURE_BATH")):>7.2f} C'
            if instrument.query("TEMPERATURE_REFERENCE") == reference["bath"]:
                output += "\t [reference]"
            if channel is None:
                output += "\n"
        else:
            output += str(instrument.set("TEMPERATURE_REFERENCE", reference[channel]))
            output += "  "
            instrument.set("TEMPERATURE_REFERENCE", reference[channel])
            output += str(instrument.set("TEMPERATURE_TARGET", value))
    if channel == "external" or channel is None:
        if value is None:
            output += "external temperature =\t"
            output += f'{float(instrument.query("TEMPERATURE_EXTERNAL")):>7.2f} C'
            if instrument.query("TEMPERATURE_REFERENCE") == reference["external"]:
                output += "\t[reference]"
            if channel is None:
                output += "\n"
        else:
            output += str(instrument.set("TEMPERATURE_REFERENCE", reference[channel]))
            output += "  "
            instrument.set("TEMPERATURE_REFERENCE", reference[channel])
            output += str(instrument.set("TEMPERATURE_TARGET", value))
    if channel == "target" or channel is None:
        if value is None:
            output += "target temperature =\t"
            output += f'{float(instrument.query("TEMPERATURE_TARGET")):>7.2f} C'
        else:
            output += str(instrument.set("TEMPERATURE_TARGET", value))
    return output


@cli.command("reference", help="Get or set the temperature reference.")
@click.argument(
    "channel",
    metavar="CHANNEL",
    type=click.Choice(("bath", "external", "ext_analog", "ext_serial")),
    required=False,
    default=None,
)
@with_instrument
@print_output
def cli_reference(instrument, channel):
    if channel is None:
        reference = {0: "bath", 1: "external", 2: "ext_analog", 3: "ext_serial"}
        return (
            "temperature reference: "
            f'{reference[instrument.query("TEMPERATURE_REFERENCE")]}'
        )
    else:
        reference = {"bath": 0, "external": 1, "ext_analog": 2, "ext_serial": 3}
        return instrument.set("TEMPERATURE_REFERENCE", reference[channel])


@cli.command("set", help="Set the target temperature.")
@click.argument(
    "temperature", metavar="[temperature]", type=float, required=True, default=None
)
@with_instrument
@print_output
def cli_set(instrument, temperature):
    return instrument.set("TEMPERATURE_TARGET", temperature)


@cli.command("model", help="Get the chiller model.")
@with_instrument
@print_output
def cli_model(instrument):
    return instrument.query("TYPE")


@cli.command(
    "monitor",
    help="Measure bath, external and target temperatures every DELAY seconds.",
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
@with_instrument
def cli_monitor(instrument, delay, output=None):
    LONG_SPACING = 19
    TEMPERATURES = ("TEMPERATURE_BATH", "TEMPERATURE_EXTERNAL", "TEMPERATURE_TARGET")

    log = MonitoringLogger(print)
    if output is not None:
        log.register_writer(lambda line, end="": output.write(line.strip() + "\n"))
        log.register_flusher(output.flush)
    log(f'# MONITORING_START {time.strftime("%x")}')
    log(
        "\t".join(
            val.ljust(LONG_SPACING)
            for val in ("# Time", "Bath [C]", "External [C]", "Target [C]")
        )
    )
    while True:
        measurements = [instrument.query(temp) for temp in TEMPERATURES]
        log(
            "\t".join(
                [time.strftime("%x %X").ljust(LONG_SPACING)]
                + [
                    f"{float(measurement):>7.2f}".ljust(LONG_SPACING)
                    for measurement in measurements
                ]
            )
            + "\t",
            end="\r",
        )
        time.sleep(delay)


# ======== LOW-LEVEL COMMANDS - Take care if using these =========


@cli.command(
    "command",
    help=(
        "Send command. [LOW-LEVEL COMMAND; use only if you really know what you're "
        "doing.]"
    ),
)
@click.argument("string", metavar="STRING", type=str, required=True)
@with_instrument
@print_output
def cli_command(instrument, string):
    return instrument._instrument.query(string)
