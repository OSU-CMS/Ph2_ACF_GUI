# Scurve - Threshold scan
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The purpose of the S-Curve scan is to determine the response efficiency of a pixel with respect to injected charge. The function of efficiency w.r.t. charge is called an "S-Curve". It closely resembles the error function (integral of a Gaussian function):

$\text{erf}(z) = \dfrac{2}{\sqrt{\pi}} \int_0^z e^{-t^2} dt.$

The threshold of the pixel is defined as the the inflection point (efficiency = 0.5) of the S-Curve. The width of the curve shows the irreducible "noise". An ideal S-Curve with no noise would be a step function. So, the S-Curve scan can be used to determine the threshold and noise of each pixel.

## Method

The S-Curve scan works by injecting a charge into each pixel `nEvents` times and measuring the [efficiency](TermExplanations.md#pixel-occupancy) of the pixel. The same measurement is repeated multiple times with different [$\Delta V_{\text{Cal}}$](TermExplanations.md#calibration-voltage-vcal) values to obtain the efficiency as a function of injected charge for each pixel. Different $\Delta V_{\text{Cal}}$ values are obtained by changing the `VCAL_HIGH` register value from `VCalHStart` to `VCalHStop` in `VCalHnsteps` steps. The result is then fitted to the error function for each pixel to determine the center of inflection (threshold) and its width (noise).

**Scan command:** `scurve`

* Analog injection

## Configuration parameters

|Name            |Typical value|Description|
|----------------|-------------|-----------|
|`nEvents`       |100          |Number of injections per pixel|
|`nEvtsBurst`    |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`   |10           |Number of triggers for each injection|
|`INJtype`       |1            |Injection type, 1 – analog|
|`VCalHStart`    |100          |The minimum [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) value for the scan|
|`VCalHStop`     |1000         |The maximum [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) value for the scan|
|`VCalHnsteps`   |50           |Number of steps for the scan|
|`DoOnlyNGroups` |0            |How many subsets of pixels (432 pixels per subset) to use (0 for all pixels)|

## Expected output

### S-Curves

Picture below shows superimposed S-Curves of all pixels.

![S-Curves](images/scurve/SCurves2D.png){width=400}

### 3D S-Curves

Picture below shows a 3D histogram, containing S-Curves for each pixel. Looking at it in this form is not very informative. However, its projections can be used to check individual S-Curves for each pixel.

![3D S-Curves](images/scurve/SCurves3D.png){width=400}

### "Threshold"

Pictures below show a 1D distribution of thresholds and a 2D map of threshold values for each pixel.

![Threshold 1D](images/scurve/Threshold1D.png){width=350}![Threshold map](images/scurve/Threshold2D.png){width=350}

### "Noise"

Pictures below show 1D and 2D pixel noise (S-Curve width) distributions.

![Threshold 1D](images/scurve/Noise1D.png){width=350}![Threshold map](images/scurve/Noise2D.png){width=350}

### ToT

Picture below shows a 2D map of sums of ToT values of all measured points for a given pixel.

![ToT map](images/scurve/ToT2D.png){width=400}

## Tips

One should pay close attention to the `VCalHStart` and `VCalHStop` values, which should be adjusted so that the expected threshold is between  `VCalHStart`-`VCAL_MED` and `VCalHStop`-`VCAL_MED`. Otherwise, the fits will likely fail and the reported threshold/noise values will be incorrect. Always check that the Threshold distribution looks like a Gaussian and is not close to the edges of the histogram range. If it is, adjust the `VCalHStart` and `VCalHStop` values accordingly. Initial checks for the scan range can be done with `DoOnlyNGroups`=1.