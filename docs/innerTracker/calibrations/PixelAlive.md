# PixelAlive
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The purpose of PixelAlive is to test for unresponsive pixels and mask them. It also gives information about where the unresponsive pixels are located. Additionally, the average [time-over-threshold](TermExplanations.md#time-over-threshold-tot) for a given test signal is reported for each pixel.

## Method

PixelAlive is one of the main base calibrations upon which many others are built. The basic version of PixelAlive injects a test charge into each pixel in a predefined pattern, waits for `TriggerConfig` bunch-crossings, and then sends `nTRIGxEvent` subsequent triggers to read out the chip's response. The whole procedure is repeated `nEvents` times for each pixel and an average pixel [efficiency](TermExplanations.md#pixel-occupancy).

The size of the test charge can be chosen by changing `VCAL_HIGH` and `VCAL_MED` registers (more details [here](TermExplanations.md#calibration-voltage-vcal)). The size of the test charge should be chosen to be well above the average threshold of the pixels so that only the unresponsive pixels have their average occupancy below 1. Then, all the pixels with average occupancy below `occPerPixel` are masked.

The PixelAlive calibration may also try to get the unresponsive pixels "unstuck" by reducing their per-pixel threshold trimming DAC (TDAC) value to zero, effectively increasing the threshold of that pixel. This option can be chosen by setting the `UnstuckPixels` parameter to 1.

**Scan command:** `pixelalive`

* Analog or digital injection

## Configuration Parameters

|Name             |Typical Value|Description|
|-----------------|-------------|-----------|
|`nEvents`        |100          |Number of injections|
|`nEventsBurst`   |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`    |10           |Number of subsequent triggers sent for each injection|
|`TriggerConfig`  |0            |Trigger latency in 25 ns bunch crossings|
|`INJtype`        |1            |Injection type, 1 – analog, 2 – digital|
|`VCAL_HIGH`      |2000         |Defines the injected charge, more details [here](TermExplanations.md#calibration-voltage-vcal)|
|`VCAL_MED`       |100          |Defines the injected charge, more details [here](TermExplanations.md#calibration-voltage-vcal)|
|`SEL_CAL_RANGE`  |0            |Whether to use high dynamic range for the injection circuit|
|`ROWstart`       |0            |Specifies a subset of rows of pixels: start|
|`ROWstop`        |335          |Specifies a subset of rows of pixels: stop|
|`COLstart`       |0            |Specifies a subset of columns of pixels: start|
|`COLstop`        |431          |Specifies a subset of columns of pixels: stop|
|`OccPerPixel`    |0.9          |Occupancy below which pixels are masked|
|`DoDataIntegrity`|0            |Whether to perform the data integrity test before measurement|
|`UnstuckPixels`  |0            |Whether the scan should try to unstuck the pixels|
|`UpdateChipCfg`  |1            |Whether to update the chip configuration txt files after the scan|
|`"EnLv1Id`       |0            |Whether to enable level 1 trigger ID counting|
|`"EnBCID`        |0            |Whether to enable Bunch-crossing ID counting|

## Expected Output

### Occupancy

Pictures below show a 1D distribution of per-pixel occupancies and a 2D map of occupancies for each pixel. A few pixels with their occupancy $>1$ can be seen in the latter plot, meaning that they are noisy and have registered more hits than
there were injections (it is possible since there are multiple triggers sent per each injection, and not all pixels are injected at the same time).

![1D Occupancy](images/pixelalive/Occ1D.png){width=350}![2D Occupancy](images/pixelalive/Occ2D.png){width=350}

### ToT

Pictures below show a 1D distribution and a 2D map of average ToT values measured at each pixel.

![1D ToT](images/pixelalive/ToT1D.png){width=350}![2D ToT](images/pixelalive/ToT2D.png){width=350}

### Masked Pixels

Pictures below show the number of masked pixels per row and column respectively. Knowing if there is a problem with a specific row or column of pixels could be helpful in finding out what the problem is.

![Masked Pixels Row](images/pixelalive/RowMasked.png){width=350}![Masked Pixels Column](images/pixelalive/ColMasked.png){width=350}

### Readout Errors

The picture below contains a 2D map of readout errors, which there were none of in this case.

![Readout Errors](images/pixelalive/ReadoutErrors.png){width=400}

### Trigger ID

Picture below shows the difference between trigger ID values of two nearest readouts that contain hits. These histograms are empty by default and get filled only if trigger ID counting is enabled in the XML file. Normally, the trigger and bunch-crossing ID differences should be equal to `nTRIGxEvent`, as can be seen in this case.

![TrigId](images/pixelalive/TriggerID.png){width=400}

### Bunch-Crossing ID

Picture below shows the difference between bunch-crossing ID values of two nearest readouts that contain hits. These histograms are empty by default and get filled only if bunch-crossing ID counting is enabled in the XML file.

![BCID](images/pixelalive/BCID.png){width=400}

## Other operation modes

### Data Integrity Test

If so desired, the PixelAlive scan, before the first injection, can be preceded by a data integrity test. It can be done by setting the `DoDataIntegrity` parameter to 1. The data integrity test is a simple scan where parts of the pixel matrix are masked in a specific way and a number of triggers is sent without any injections. If the data can be decoded correctly, the unmasked part of the chip is considered to be working properly. Otherwise, the part under test is considered to be faulty and gets masked. The masking pattern is chosen by setting `DoDataIntegrity` to a specific non-zero value (zero means no data integrity test). `DoDataIntegrity`=1 runs tests the core columns, `DoDataIntegrity`=2 tests individual pixels, and `DoDataIntegrity`=3 tests individual pixels but masks the corresponding core columns. The data integrity test is useful when testing a new module for the first time, as it can help to identify core column issues.

### Crosstalk (X-talk) Test

The PixelAlive scan can also be used to test for crosstalk between pixels. This is useful when checking for disconnected bumps between the sensor and the readout chip. The crosstalk test works by injecting a charge into a certain group of pixels and then masking those pixels for the readout to see the response only of their neighboring pixels. If the injected (large) charge does not migrate from one pixel to another, it can mean that (at least) one of the pixels has a disconnected bump. There are two modes for the crosstalk test: one, chosen by setting `INJtype`=5, checks the crosstalk between coupled pixels, and the other, chosen by setting `INJtype`=6, checks the crosstalk between neighboring decoupled pixels. An illustration of coupled and decoupled pixels is shown in the picture below. In the end, if there are no disconnected bumps, the crosstalk test results should look almost the same as the [regular PixelAlive results](#expected-output).

One can also perform more detailed crosstalk tests which are described [here](XTalk.md).

![Illustration of coupled and decoupled pixels](images/pixelalive/xtalk.png){width=300}

Pixels marked with grey and red arrows are a coupled pair, whereas the ones marked with grey and green arrows are decoupled.

The suggested values for relevant parameters in a cross-talk measurement are given in table below. Generally, it is suggested to inject a very large charge to see the cross-talk effect clearly. To be able to inject larger charge, one can enable the high dynamic range mode of the injection circuit by setting `SEL_CAL_RANGE`=1. In that case, 1 $\Delta V_{\text{Cal}}$ unit corresponds to 2 times larger charge than in the fine-tuning (default) mode, equal to ~10 electrons instead of ~5. Then, the maximum charge that can be injected is ~40000 electrons, which is enough to see the cross-talk effect very clearly.

| Name            | Recommended Value | Description |
|-----------------|-------------------|-------------|
|`INJtype`        |5, 6               |Injection type, 5 – x-talk coupled pixels, 6 – decoupled|
|`VCAL_HIGH`      |4000               |Defines the injected charge, more details [here](TermExplanations.md#time-over-threshold-tot)|
|`VCAL_MED`       |100                |Defines the injected charge, more details [here](TermExplanations.md#time-over-threshold-tot)|
|`SEL_CAL_RANGE`  |1                  |Whether to use high dynamic range for the injection circuit|
|`OccPerPixel`    |0.1                |Occupancy below which pixels are masked|
