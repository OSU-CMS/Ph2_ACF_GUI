# Noise scan
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The purpose of the Noise Scan is to find and deactivate the noisy pixels. We call pixels noisy when they register hits even when there are no calibration injections and no ionizing particles passing through the sensor. Pixels can be noisy because of various connection problems but they also can become noisy when their threshold is set too low. It is a good idea to mask those pixels (especially the ones that are really problematic) so that they do not produce ``fake'' hits in the data and do not take up the data bandwidth. Also, noisy hits can produce undesired trigger signals if the self-triggering or external HitOR triggering is used. 

## Method

The Noise Scan is basically a [PixelAlive scan](PixelAlive.md) without any injections. It doesn't even have its own class. A number of triggers (`nEvents`$\times$`nTRIGxEvent`) is sent to the module without any injections and the event data is read out. If there are any hits in the event, they are either noise or genuine particle hits. If there is no ionizing particle stream directed towards module, the hits are almost guaranteed to come from noise, since one is not triggering on hits (or at least one shouldn't) but rather sending the trigger at equidistant time intervals.

After the scan, the occupancy of each pixel (calculated w.r.t. the total number of triggers) is compared with the user-defined occupancy threshold, above which the pixel is considered to be noisy and gets masked.

**Scan command:** `noise`

* Analog (or Digital) injection.
* 1 bunch crossing
* 1e7 triggers per pixel

## Configuration parameters

|Name            |Typical value|Notes      |
|----------------|-------------|-----------|
|`nEvents`       |1e7          |Number of trigger sequences to be sent (Usually 10$\times$1/`TargetOcc`)|
|`nEvtsBurst`    |1e4          |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`   |10           |Number of subsequent triggers in one trigger sequence|
|`INJtype`       |0            |Injection type, 0 – no injections|
|`OccPerPixel`   |2e-5         |Occupancy threshold above which the pixel is masked|
|`UpdateChipCfg` |1            |Whether to update the chip configuration txt files after the scan|

## Expected output

Plots produced by the Noise Scan are given below. A particularly low threshold was used to produce these plots so that slightly more noise hits are visible.

### Occupancy

Figure below shows the distribution of per-pixel noise occupancies, which looks empty as most noise occupancies were below $10^{-6}$.

![1D Occupancy](images/noise/Occ1D.png){width=400}

Figure below shows a 2D map of pixel occupancies where some noisy pixels are visible.

![2D Occupancy](images/noise/Occ2D.png){width=400}

### ToT

Pictures below show a 1D distribution and a 2D map of average ToT values measured at each pixel. Since there were no injections and most hits were caused by noise, most pixels measure ToT=0.

![1D ToT](images/noise/ToT1D.png){width=350}![2D ToT](images/noise/ToT2D.png){width=350}

### Masked Pixels

Picture below shows a map of masked pixels, where the pixel that has been masked is marked in yellow (therefore it is difficult to see them).

![Masked Pixels Map](images/noise/Masked2D.png){width=400}

## Tips 

It is possible to make the scan run faster changing the `nTRIGxEvent`. For example given the two following configurations:

* `nEvents  = 1e7`, `nTRIGxEvent = 10`
* `nEvents  = 1e8`, `nTRIGxEvent = 1`

The first one runs faster even though they send the same amount of triggers.