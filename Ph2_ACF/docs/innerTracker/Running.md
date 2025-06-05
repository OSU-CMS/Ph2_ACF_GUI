# Running Ph2ACF

All functionality for running IT modules is contained in the `CMSITminiDAQ` binary.

Invocation `CMSITminiDAQ -f MAIN_XML_FILE [further options]`

## Reset

!!! warning "A reset must be performed after loading a new firmware by `fpgaconfig`"

`CMSITminiDAQ -f MAIN_XML_FILE -r`

Clears the FC7 buffers and resets the FC7 AURORA cores.
Can get the FC7 out of various failure modes.

!!! info "RD53A modules must usually be power cycled after a reset in order to establish an AURORA link"

## Configuration and register dump

* `CMSITminiDAQ -f MAIN_XML_FILE -p`  
   Just program the system components

* `CMSITminiDAQ -f MAIN_XML_FILE`  
   Startup DAQ, try to establish link to all configured hybrids and configure them

* `CMSITminiDAQ -f MAIN_XML_FILE -d`  
   Dump the registers of all components.
!!! warning "This will not (re-)establish the AURORA links."
!!! info "This is not the run configuration if `DisableChannelsAtExit` is set to 1"

## Running a scan

`CMSITminiDAQ -f MAIN_XML_FILE [-s SETTINGS_FILE] -c SCAN_NAME`

Running a scan will configure all enabled modules, run the scan using paramaters from the main XML file or (if defined, since v4-12) the settings file.
Resulting plots along with the configuration files, used at start of the scan will be saved in the subfolder `./Results` relative to the current folder.

The current run number is saved in `./RunNumber.txt` and used for the next run.
!!! warning "Run numbers in EUDAQ mode"
    Run numbers in EUDAQ mode are set by the run control, but still saved in the run number file.
    As there is no check for existing files, results can be overwritten if EUDAQ runs reset the number to an already used one.

**Available calibrations**

```
latency pixelalive noise scurve gain threqu gainopt thrmin 
thradj injdelay clkdelay datarbopt datatrtest physics eudaq
bertest voltagetuning gendacdac vtrx eye
```

Calibrations and parameters are described under [IT Calibrations](calibrations/index.md).

### Physics mode

A special scan is `physics` or "source scan" mode which can be used either with random (`FSM`) or external triggers (`HitOR`, `TLU`, `external`).
The run time for this scan as defined in the `<Settings>` section can be overridden by

```
--runtime <value>, -t <value>
    Set running time for physics mode (in seconds)
```
More details can be found in the [Physics Scan](calibrations/Physics.md) page.

## EUDAQ mode

!!! info "TODO: description"

```
--eudaqRunCtr <value>
    EUDAQ-IT run control address (e.g. tcp://localhost:44000)

-n <value>, --prodName <value>
    Name of the EUDAQ producer in run controler
```

## Expert options

```
-b <value>, --binary <value>
    Binary file to decode

--capture <value>
    Capture communication with board (extension .bin)

--replay <value>
    Replay previously captured communication (extension .bin)
```
