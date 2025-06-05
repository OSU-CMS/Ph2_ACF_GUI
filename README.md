ICICLE - Instrument Control Interface and Commands Library developed @ETHZ
==========================================================================

[![Version: 2.1.0](https://img.shields.io/badge/version-2.1.0-green)](https://gitlab.cern.ch/icicle/icicle/-/tree/master)
[![Pipeline: master](https://gitlab.cern.ch/icicle/icicle/badges/master/pipeline.svg)](https://gitlab.cern.ch/icicle/icicle/-/tree/master)
[![Docs: sphinx](https://img.shields.io/badge/docs-sphinx-blue)](https://gitlab.cern.ch/icicle/icicle/-/jobs/artifacts/master/browse?job=docs)
[![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/psf/black)

This library is designed to provide a lightweight yet robust set of classes and
command line tools for DCS and instrument control.

Read the full documentation at https://icicle.docs.cern.ch/.

The following devices are currently supported:
- ADC Board V2 for probe cards (ETHZ custom device): `adcboard`.
- Binder MKF climatic chamber: `binder`.
- CAEN DT8033N desktop power supply module: `caendt8033n`.
- Rhode&Schwarz/Hameg HMP4040 LV power supply: `hmp4040`.
- HP34401A multimeter: `hp34401a`.
- Keithley 2000 multimeter: `keithley2000`.
- Keithley 6500 multimeter: `keithley6500`.
- Keithley 24X0-class source-measure unit: `keithley2410`.
- Keysight E3633A LV power supply: `keysighte3633a`.
- ITk Module Testing DCS/Interlock GUI (Oxford): `itkdcsinterlock`.
- Gulmay MP1 xray controller: `mp1`.
- Huber CC508 (and similar) water chillers: `hubercc508`.
- TTI MX100TP-class LV power supply: `tti`.
- TTI TSX1820P-class LV power supply: `ttitsx`.
- Timepix4 slow-control for SPIDR4 readout system: `tpx4sc`.
- ETHZ SLDO probe card controller board (arduino; a. la. Vasilije Perovic):
  `relay_board`.
- Lauda-type chillers: `lauda`.
- `libximc`-compatible motor controllers and position stages: `ximc_instrument`.
- TRICICLE generic-PID-Controller GUI instances: `pidcontroller`.
- PSI Coldbox, temperature control setup for CMS Pixel developed at PSI: `psi_coldbox`

A super-device implementation for a standard module testing setup consisting of
LV, HV, relay board, and multimeter (with interlock features) is provided:
`instrument_cluster`.

A dummy/placeholder instrument is available: `dummy_instrument`.


## Instrument Last-Known-Tested Versions

Documented here is the last version that instruments have been explicitly tested
on (since many developers don't have access to all types of instruments and
cannot test possibly breaking changes to all). Patch-level changes should not
break existing instruments or functionality.

| Instrument             | LKT Version | Tested By        | Date       |
| ---------------------- | ----------- | ---------------- | ---------- |
| `AdcBoard`             | 2.0.0       | T. Harte         | 2024-05-24 |
| `BinderClimateChamber` | <2.0.0      | T. Harte         | -          |
| `CaenDT8033N`          | 2.0.0       | T. Harte         | 2024-08-06 |
| `HMP4040`              | 1.0.0       | D. Bacher        | -          |
| `HP34401A`             | 2.0.0       | M. Joyce         | 2024-10-17 |
| `HuberCC508`           | 2.0.0       | S. Koch          | 2024-10-16 |
| `ITkDCSInterlock`      | 2.0.0       | S. Koch          | 2024-10-16 |
| `Keithley2000`         | 1.0.0       | D. Bacher        | -          |
| `Keithley6500`         | 2.0.0       | M. Joyce         | -          |
| `Keithley2410`         | 2.0.0       | S. Koch          | 2024-10-01 |
| `KeysightE3633A`       | 2.0.0       | T. Harte         | -          |
| `Lauda`                | 1.2.0       | F. Guescini      | 2022-06-30 |
| `MP1`                  | 1.1.0       | S. Koch          | 2022-04-04 |
| `PIDController`        | 2.0.0       | S. Koch          | 2024-10-16 |
| `Relay\_Board`         | 1.0.0       | D. Ruini         | -          |
| `TTI`                  | 1.0.0       | D. Bacher        | -          |
| `TTITSX`               | 1.0.0       | S. Gennai        | 2025-03-22 |
| `TPX4SC`               | 2.1.0       | S. Koch          | 2024-10-01 |
| `XimcInstrument`       | 2.0.0       | S. Koch          | 2023-04-18 |
|                        |             |                  |            |
| `InstrumentCluster`    | 2.0.0       | T. Harte         | -          |
| `PSI Coldbox`          | 2.1.0       | P. Sander        | 2025-04-04 |


## Installation

Python >= 3.7 is required. Python 3.9 or higher is recommended (and is required
for other parts of the dirigent module testing suite).

It is recommended to install into a virtual environment - for example:
```
python3 -m venv python3-env
. python3-env/bin/activate
```

Installation is performed using the `setup.py` script:
```
python3 -m pip install "."
```

If you would like to be able to modify the source files, add a `-e` flag.

To install also the VISA-instrument simulation alternative backend,
replace `"."` with `".[sim]"`.

To use the `XimcInstrument` device and associated `ximc` CLI-interface, please
make sure to install libximc from https://files.xisupport.com/Software.en.html,
then execute the `pyximc` install script:
```
cd deps
./get_pyximc.sh
```

To install the code formatting libraries required for developing and
contributing, please install with the `[dev]` optional dependency. If you need
to build documentation, you can install the correct `sphinx` version and
dependencies via the `[docs]` optional dependency.


## Building API Documentation

Building API docs requires both the `icicle` package and the `sphinx` python
package to be installed (API docs are generated by reading python sources),
which can be done via the `[docs]` optional dependency:
```
python3 -m pip install [-e] ".[docs,...]"
```

Then build using the makefile (you can subsitute latex for html to generate
latex sources):
```
cd docs
make html
```

Then point your browser at index.html in the `docs/build` directory, and enjoy.


## Command Line Interface - Usage

For each device type, simply type `<device> --help` for a full list of commands
and arguments. For example:
```
$ hmp4040 --help

Usage: hmp4040 [OPTIONS] COMMAND [ARGS]...

Options:
  -v, --verbose          Verbose output (-v = INFO, -vv = DEBUG)  [x>=0]
  -R, --resource TARGET  VISA resource address (default:
                         ASRL/dev/ttyACM0::INSTR)
  --help                 Show this message and exit.

Commands:
  activate      Activate output for channel CHANNEL (1, 2, 3, or 4)
  activated     Read output activation status for channel CHANNEL (1, 2,...
  current       Read current currently set for channel CHANNEL (without...
  deactivate    Deactivate output for channel CHANNEL (1, 2, 3 or 4)
  identify      Identify instrument
  measure       Measure voltage (in V) and current (in A) currently...
  monitor       Measure voltage (in V) and current (in A) currently...
  off           Turn general output OFF
  on            Turn general output ON
  ovp           Read overvoltage protection currently set for channel...
  query         Query configuration field FIELD (see hmp4040.py for...
  ramp_current  Ramp channel CHANNEL to target current TARGET (A).
  ramp_voltage  Ramp channel CHANNEL to target voltage TARGET (V).
  reset         Reset instrument.
  scpi_query    Send SCPI command COMMAND, and immediately read response.
  scpi_read     Attempt to read on SCPI serial line.
  scpi_write    Send SCPI command COMMAND.
  set           Set configuration field FIELD to value VALUE (see...
  status        Read general output state (ON/OFF)
  voltage       Read voltage currently set for channel CHANNEL (without...
```

Individual commands can also be combined with `--help` for further information:
```
$ hmp4040 ovp --help
Usage: hmp4040 ovp [OPTIONS] [CHANNEL] OVP

  Read overvoltage protection currently set for channel CHANNEL (without OVP
  argument), or set OVP (V) for channel CHANNEL (1, 2, 3, or 4)

Options:
  --help  Show this message and exit.
```


## Graphical Monitoring

The library offers a graphical display of PSU outputs (currently only TTI
MX100TP-class and Keithley24X0-class) using Eclipse-Mosquitto, Telegraf,
Influxdb and Grafana, all contained in Docker containers.

### Prerequisites

To use the graphical monitoring, Docker and Docker-Compose need to be installed.
For installation guides, see:
- [Docker](https://docs.docker.com/engine/install/)
- [Docker-Compose](https://docs.docker.com/compose/install/)

### Setup

Change into the `monitoring` directory and run docker-compose using the
`docker-compose.yml` configuration file:
```
cd monitoring
docker-compose -f docker-compose.yml up -d
```

This will start Docker containers of Eclipse-Mosquitto, Telegraf, IndluxDB and
Grafana. The Grafana docker can be accessed in your browser under:
`http://localhost:3000`

Running `<device> monitor -m` on the same machine should confirm the connection
to the MQTT broker and will send the measurements to the InfluxDB database, from
where they can be accessed with Grafana. For example:
```
$ tti monitor -m ALL
Connected to MQTT broker!
# MONITORING_START 01/01/22
# Time              Voltage1 (V)        Current1 (A)        Voltage2 (V)        Current2 (A)
01/01/22 12:34:56   1.234               0.4567              -9.876              -4.321
```

For more information on how to configure the monitoring and Grafana, see the
full documentation at
[https://icicle.docs.cern.ch/](https://icicle.docs.cern.ch/).


## Simulating devices

This behaviour is currently only supported for `Keithley2410` for a very
limited command set.

To run the simulation in the provided script, add the `-S` flag to the start
of `keithley2410` commands, and replace the resource address with an appropriate
ASRL number from the `icicle/sim/visa_instrument.yaml` simulation file.

To run a simulation via the class, pass the `sim=True` argument to the
Keithley2410 constructor, and an appropriate resource.

### Implementation and adding new simulated devices

The `VisaInstrument` superclass accepts a `sim=True` argument that swaps the
backend from `@py` (pyvisa-py) to `@sim` (pyvisa-sim), and passes the simulation
file at `icicle/sim/visa_instrument.py`.

To add simulated commands to a new class, modify the constructor to accept and
pass the `sim=True` argument to its `SCPIInstrument` or `VisaInstrument` super-
class via the `super.__init__(sim=sim,...)` invocation. Then, add a new yaml
file with an instrument definition satisfying the format and specifications of
[pyvisa-sim](https://pyvisa.readthedocs.io/projects/pyvisa-sim/en/latest/)
for your device to `icicle/sim/<your_instrument_package>.yaml`. Finally, add
a simulated resource for this instrument to `icicle/sim/visa_instrument.yaml`.


## Running tests

Before running tests, install the `[sim,test]` dependency sets for ICICLE:
```
python3 -m pip install -e ".[sim,test]"
```

In order to run the MP1 tests, `pyvisa-sim` needs to be patched using the
provided script in the `deps` directory:
```
cd deps/
bash patch-pyvisa-sim.sh
```

The CLI tests are run from the `test/cli` directory using `make`:
```
cd test/cli
make test
```

In order to run a single test, each test file can be run using the `*.cli-test`
target:
```
make keithley2410.cli-test
```

For details on the test file format and writing new test files, see the
"Testing" section of the
[ICICLE documentation](https://icicle.docs.cern.ch/testing.html) for v1.3+.

## Developing/Contributing

***Note:** currently, running a development environment requires at least Python 3.9 (as older versions are officially unsupported and hence no longer supported by the usual linting and formatting tools).*

Plese make sure to install the package in editable mode with the `[dev]`
optional dependency, and enable the pre-commit hooks in the git repository
after installation via:
```
pre-commit install
```

This will run the [Black](https://black.readthedocs.io/en/stable/) automatic
code format tool, as well as `pydocformatter` and a few other simple
reformatting tools on commit. We are currently using the tagged version
`black==25.1.0` (which is installed via the `[dev]` requirement).

Please use a local PEP8-compliant linter while coding - we recommend
[Flake8](https://flake8.pycqa.org/en/latest/), which can be integrated into
most IDEs, and has a project-specific ruleset defined in
`pyproject.toml`. We are currently using the following version and modules
(which are installed via the `[dev]` requirement):
```
    flake8==7.2.0
    flake8-bugbear==24.12.12
    flake8-pyproject==1.2.3
```

A more formal set of MR requirements and approval/review hierarchy will be added
soon.
