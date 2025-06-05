# Change Log
A history of changes in the current and past ICICLE releases.

This project (mostly) adheres to [semantic versioning](http://semver.org/).

## [2.1.0] - 2025-04-25 - New devices

ICICLE 2.1.0 brings new devices and several infrastructure fixes.

Main contributors to this release: T. G. Harte, M. Joyce, P. M. Sander, S. Gennai
C. Z. Tee, S. Rohletter, S. F. Koch

### Added

New devices:
- [PSI "Tessie" Coldbox (v1/preliminary support) - `coldbox`](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/31)
- [Timepix4 slow-control for SPIDR4 readout - `tpx4sc`](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/58)
- [TTI TSX single-channel power supply - `ttitsx`](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/54)

Device functionality:
- [Added `VOFS` to `adc_board` implementation](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/52)
- [Add SSL to monitoring for `hmp4040`](https://gitlab.cern.ch/icicle/icicle/-/commit/d3efe1c42325d85b44118d9e3bc301a87604366d)

### Changed

- [Applied naming convention to `caendt8033n` and `ttitsx` devices](https://gitlab.cern.ch/icicle/icicle/-/commit/de7d5eb31e9cec2e3d8f1309139dfa1b7013195a)

### Fixed

- [`pidcontroller` command naming issue](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/59)
- [Pipelines keep failing due to docker hub rate limiting](https://gitlab.cern.ch/icicle/icicle/-/issues/25) solved using CERN pass-through proxy for docker-hub
- [Pre-commit configuration altered to remove DocFormatter](https://gitlab.cern.ch/icicle/icicle/-/issues/28) as this package seems to be no longer well-supported.
- [Update current setting in `keysighte3633a`](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/49) to 16A.
- [Aquire Lock Loop in toggle switch function of `binder` climate chamber](https://gitlab.cern.ch/icicle/icicle/-/merge_requests/26)

## [2.0.0] - 2024-11-13 - High-Level Interface and new devices

ICICLE 2.0.0 brings many new devices, a new set of high-level interfaces, and
many tweaks and improvements to existing devices and interfaces.

Main contributors to this release: T. G. Harte, M. Joyce, P. M. Sander, C. Lange,
R. De Los Santos, S. F. Koch

### Added

New high-level interfaces:
- The `Channel` concept for device-agnostic monitoring and control. Current
  implementations: `MeasureChannel`, `PowerChannel`, `TemperatureChannel` (beta),
  and `MeasureChannelWrapper` (beta)
- Channel instantiation currently via a factory function on the
  `Instrument`-derived class: `Instrument.channel(...)`.

New devices:
- `AdcBoard` (ETH Zurich design)
- `BinderClimateChamber`
- `CaenDT8033N` power supply
- `HP34401A` multimeter
- `HuberCC508` chiller
- `ITkDCSInterlock` and `PIDController` software-device interfaces
- `XimcInstrument` XIMC-type linear stage control
- `RelayBoard` updated to v2 for CROCs

New CLI interfaces:
- `powerchannel` and `measurechannel` CLI commands provide direct access to
  channel interface

New structure and code quality improvements:
- Tests introduced for most devices and device types
- Code format now imposed via `Black` and checked with `flake8` among other tools
- `pre-commit` hooks added
- General improvements to uniformity between devices and code semantics

### Changed

- Most devices no longer have setup/location-specific default resources - these have been replaced by the simulation endpoints.
- Command lists for devices such as the `Keithley2410`, `TTI`, and `HMP4040` have been standardised. Old endpoints are still provided, but are marked as deprecated.
- RD53A `RelayBoard` is no longer supported.
- `InstrumentCluster` has been fundamentally reworked and is no longer cross-compatible.

### Fixed

- Many. See commit history for full log.

## [1.2.0] - 2022-07-13 - Lauder chiller

Main contributors to this release: F. Guescini

### Added

- Lauda Chiller support in `lauda.py`/`lauda_cli.py`.

## [1.1.1] - 2022-07-08

Main contributors to this release: C. Lange, D. Bacher

### Added

- Build PyPI package for Gitlab Package Registry in pipeline.

### Fixed

- Bugfix in channel validation in `TTI` class.

## [1.1.0] - 2022-04-05 - Gulmay `mp1` x-ray controller

Main contributors to this release: S. F. Koch

### Added

- Gulmay `MP1` x-ray controller (in use in Oxford OPMD)

## [1.0.0]

First official release.

Main contributors to this release: V. Perovic, D. Bacher, D. Ruini, B. Ristic,
S. F. Koch
