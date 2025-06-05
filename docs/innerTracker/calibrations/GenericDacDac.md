# DAC-DAC scan
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The Generic DAC-DAC Scan allows for the measurement of pixel occupancy as a function of any two firmware or front-end registers, thus making it possible to perform nearly any type of measurement. It also returns the optimum value for both chosen registers where the received occupancy is the highest.

## Method

The Generic DAC-DAC Scan is derived from [PixelAlive](PixelAlive.md). The user is able to choose any two registers they want to scan by setting the register names using `RegNameDAC1` and `RegNameDAC2` settings. The scan range is determined by `StartValueDAC1`, `StopValueDAC1` and `StepDAC1` for the first register, and `StartValueDAC2`, `StopValueDAC2` and `StepDAC2` for the second one. `StartValueDAC*` and `StopValueDAC*` (where `*` is `1` or `2`) determine the minimum and maximum register values for the scan, while `StepDAC*` determines the step size. Then, the scan simply runs PixelAlive many times, each time at a different combination of the two register values. The PixelAlive scan measures the pixel occupancy which is put into a 2D plot showing the occupancy as a function of the two registers. Finally, the parameter combination that provides the maximum occupancy is reported on the screen.

With default settings, the scan is performed with the following parameters:
* `user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse` from 28 to 50 with step size = 1
* [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) from 300 to 1000 with step size = 20
 
Though, it should be noted, that there are vast possibilities for this scan as any two parameters can be used and their values should be chosen according to the specifics of each measurement. Therefore, the table contains only a single example. Possibilities of this scan can be expanded even further by e.g., switching off the injections and looking at the noise occupancy.

**Scan command:** `gendacdac`

* Analog or digital injection

## Configuration parameters

|Name            |Typical value|Description|
|----------------|-------------|-----------|
|`nEvents`       |100          |Number of injections for each scan step|
|`nEvtsBurst`    |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`   |10           |Number of triggers for each injection|
|`INJtype`       |1            |Injection type, 1 – analog|
|`RegNameDAC1`   |`user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse`|Name of the 1st register|
|`StartValueDAC1`|25           |Lowest value of the 1st register|
|`StopValueDAC1` |50           |Highest value of the 1st register|
|`StepDAC1`      |1            |Step size for the 1st register|
|`RegNameDAC1`   |[`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal)|Name of the 2nd register|
|`StartValueDAC1`|250          |Lowest value of the 2nd register|
|`StopValueDAC1` |500          |Highest value of the 2nd register|
|`StepDAC1`      |50           |Step size for the 2nd register|

## Expected output

Pictures below show all the plots produced by the Generic DAC-DAC Scan. In this case, the two scanned registers were `VCAL_HIGH` and `user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse`. The former was scanned from 300 to 1000 with a step size of 50, while the latter was scanned from 28 to 50 with a step size of 2.


Picture below shows the 2D dependence of occupancy on the two parameters. It can be noticed that only a single `user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse` value was able to produce a non-zero occupancy.

![DAC DAC Scan](images/gendacdac/GenDacDacScan.png){width=400}

Histograms below show the suggested values of the first register (in this case,
`VCAL_HIGH`) and the second register (in this case, `user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse`), respectively.

![DAC 1](images/gendacdac/GenDac1.png){width=350}![DAC 2](images/gendacdac/GenDac2.png){width=350}

## Tornado plot

![Tornado plot](images/gendacdac/TornadoPlot.png){width=400}

The generic DAC-DAC scan also allows to produce the “tornado plot”. It returns the optimal value as the highest efficiency with lowest injection delay and [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal).

Scan settings:
* `CAL_EDGE_FINE_DELAY` from 0 to 61 with step 1
* `VCAL_HIGH` from 300 to 1300 with step 20

**Important**: First, run a [latency scan](LatencyScan.md) ➜ get proper latency to start with.
