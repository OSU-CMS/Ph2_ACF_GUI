# Threshold Minimization
<sub><sup>Last updated: 16.12.2024</sup></sub>

## Purpose

The purpose of the Threshold Minimization calibration is lowering the hit detection threshold as much as possible while keeping the noise levels controlled.

## Method

Threshold Minimization is derived from [PixelAlive](PixelAlive.md). It tries to determine the lowest possible global threshold register `DAC_GDAC_*_LIN` (where `*` is `L`, `R`, and `M`) value that keeps the noise occupancy at the desired value `targetOccupancy`. The working principle is very similar to the [Threshold Adjustment](ThresholdAdjust.md) calibration. A binary search in `DAC_GDAC_*_LIN` is performed between `ThrStart` and `ThrStop` values, performing a PixelAlive measurement each time and choosing the threshold value that gets the average occupancy closest to the `targetOccupancy`. The main difference between Threshold Adjustment and Threshold Minimization is that the latter does not use calibration injections, so the pixel occupancy is coming from the noise. Also, Threshold Minimization tries masking individual pixels with occupancy higher than the `targetOccupancy` but only if the percentage of masked pixels is kept at or below `MaxMaskedPixels`. After the calibration finishes, the best `DAC_GDAC_*_LIN` value is printed on the screen and should be entered into the XML file by hand.

**Scan command:** `thrmin`

* Analog injection

!!! warning "You have to update the config manually in the xml file"
    * RD53B: `DAC_GDAC_{L,R,M}_LIN`
    * RD53A: `Vthreshold_LIN`

## Configuration parameters

|Name            |Typical value|Description|
|----------------|-------------|-----------|
|`nEvents`       |1e7          |Number of trigger sequences for each  `DAC_GDAC_*_LIN` step|
|`nEvtsBurst`    |1e4          |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`   |10           |Number of subsequent triggers in one trigger sequence|
|`INJtype`       |0            |Injection type, 0 – no injections|
|`TargetOcc`     |1e-6         |Target average noise occupancy|
|`MaxMaskedPixel`|1            |Maximum allowed percentage of masked pixels|
|`ThrStart`      |340          |The lowest `DAC_GDAC_*_LIN` value for the scan|
|`ThrStop`       |440          |The highest `DAC_GDAC_*_LIN` value for the scan|
|`nClkDelays`    |10           |Delay between two trigger sequences (in 100 ns time units)|

## Expected output

### Suggested threshold values

Histogram below contains only the suggested `DAC_GDAC_*_LIN` register value.

![Suggested GDAC values](images/thrmin/Threshold.png){width=400}

### Final pixel occupancy

Picture below shows the distribution of noise occupancies at the suggested `DAC_GDAC_*_LIN` value, which has only a few entries at very low occupancy, as the occupancy target was set to $10^{-6}$.

![1D occupancy](images/thrmin/Occ1D.png){width=400}

Picture below shows a 2D map of pixel occupancies at the suggested `DAC_GDAC_*_LIN` value, where most occupancies are expectedly either zero or close to zero.

![2D occupancy](images/thrmin/Occ2D.png){width=400}

### Masked pixels

Pictures below show the number of masked pixels in each pixel column and row, respectively.

![Masked pixels per column](images/thrmin/ColMasked.png){width=350}![Masked pixels per row](images/thrmin/RowMasked.png){width=350}

Picture below shows a 2D map of masked pixels, where the pixels that have been masked are marked in yellow (therefore it is difficult to see them).

![Masked pixels 2D](images/thrmin/Masked2D.png){width=400}

### Calibration result

Pictures below show the threshold distribution from S-Curve measurement before and after the threshold minimization. The top row shows threshold distributions without [TDAC tuning](ThresholdEqualization.md), and the bottom row shows the threshold distributions with TDAC tuning.

|Untuned thresholds: GDAC=400               |Threhold minimization: GDAC=380            |
|-------------------------------------------|-------------------------------------------|
|![](images/thrmin/thremin_1.png){width=350}|![](images/thrmin/thremin_2.png){width=350}|
|**Equalized thresholds: GDAC=380**         |**Threhold minimization: GDAC=357**        |
|![](images/thrmin/thremin_3.png){width=350}|![](images/thrmin/thremin_4.png){width=350}|