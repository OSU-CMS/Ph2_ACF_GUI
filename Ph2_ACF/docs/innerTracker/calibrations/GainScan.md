# Gain scan
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The purpose of the Gain Scan is determining the [ToT](TermExplanations.md#time-over-threshold-tot) response (gain) of the front-end. The ToT response to the amount of charge collected in the linear front-end is approximated like this:

$T(\Delta V_{\text{Cal}}) = a \; \Delta V_{\text{Cal}} + b \, ,$

where $T$ is the ToT, measured in 25 ns time interval units, [$\Delta V_{\text{Cal}}$](TermExplanations.md#calibration-voltage-vcal) determines the injected charge, $a$ is the slope (also called *the gain*), and $b$ is the line intercept. The linearity of this equation is a feature of the linear analog front-end. Having done the Gain Scan, one knows the conversion between ToT and the amount of signal charge (in $\Delta V_{\text{Cal}}$ which can be converted to the number of electrons) for each pixel. If a dual-slope mode is enabled, two slopes
and two intercepts are determined, one of each for ToT<8, and another for ToT$\geqslant$8.

The gain slope depends directly on the Krummenacher current $I_{\text{Krum}}$ that discharges the capacitor in the preamplifier circuit. The gain intercept depends on the Krummenacher current too (as the slope and offset are anticorrelated), but it is also directly proportional to the threshold (which defines the ToT=0 point).

## Method

The Gain Scan injects each pixel with the same amount of charge, sends a trigger, and measures the ToT of each pixel. Then, the same step is repeated `VCalnsteps` times with a different charge amount each time. The charge values at each step are defined by the `VCalHStart` and `VCalHStop` registers, taking `VCalnsteps` equidistant steps from that range. The actual amount of injected charge also depends on `VCalMED` register (same value each time) which is subtracted from the number mentioned before.

After `VCalnsteps` measurements are finished for each pixel, all the superimposed gain curves are drawn as a 2D plot. A linear fit (or two linear fits if a dual slope mode is enabled) is performed on the ToT-vs-$\Delta V_{\text{Cal}}$ curve, for each pixel, according to the equation given above. The slope $a$ and intercept $b$ for each pixel are extracted from the fit and plotted as 2D maps as well as 1D distributions. Fit parameter uncertainties, $\chi^2$, and the number of degrees of freedom are also plotted in separate figures. After that, an average ToT given by the injected `targetCharge` is extracted from the fit and reported on the screen.

**Scan command:** `gain`

* Analog injection

## Configuration parameters

|Name            |Typical value|Description|
|----------------|-------------|-----------|
|`nEvents`       |100          |Number of injections per [$\Delta V_{\text{Cal}}$](TermExplanations.md#calibration-voltage-vcal) step|
|`nEvtsBurst`    |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`   |10           |Number of triggers for each injection|
|`INJtype`       |1            |Injection type, 1 – analog|
|`ToT6to4Mapping`|0            |Whether to use the ToT dual-slope mode (6-to-4 bit compression)|
|`VCalHStart`    |100          |Starting [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) register value for the scan|
|`VCalHStop`     |4000         |Final [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) register value for the scan (<4096)|
|`VCalnsteps`    |20           |Number of points to test between VCalHStart and VCalHStop|
|`targetCharge`  |3300         |Charge value (in $\Delta V_{\text{Cal}}$) at which the average [ToT](TermExplanations.md#time-over-threshold-tot) is reported|

## Expected output

### Gain curves 

Pictures below show superimposed gain curves of each pixel in single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![Gain curves single-slope](images/gain/Gain1.png){width=350}|![Gain curves dual-slope](images/gain/Gain2.png){width=350}|

### 3D gain maps

Pictures below show gain maps for single and dual-slope modes respectively. These are 3D histograms with pixel row, column, and injected charge on the three axes, while the bin contents represented by color correspond to pixel occupancy. While impossible to read by themselves, these histograms can be used to make projections and extract individual gain curves of each pixel.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![Gain maps single-slope](images/gain/GainMap1.png){width=350}|![Gain maps dual-slope](images/gain/GainMap2.png){width=350}|

### Gain slope (low charge)

Pictures below show 1D distributions of slopes of the linear gain fit for single and dual-slope modes respectively. For the dual-slope mode, the distribution corresponds to the low-charge linear fit that goes up to ToT=8.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![1D Gain single-slope lowQ](images/gain/SlopeLowQ1D1.png){width=350}|![1D Gain dual-slope lowQ](images/gain/SlopeLowQ1D2.png){width=350}|

Pictures below show 2D maps of low-charge gain slopes for single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![2D Gain single-slope lowQ](images/gain/SlopeLowQ2D1.png){width=350}|![2D Gain dual-slope lowQ](images/gain/SlopeLowQ2D2.png){width=350}|

### Gain intercept (low charge)

Pictures below show 1D distributions of linear fit intercepts for single and dual-slope modes respectively (ToT<8 for the dual-slope mode).

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![1D Intercept single-slope lowQ](images/gain/InterceptLowQ1D1.png){width=350}|![1D Intercept dual-slope lowQ](images/gain/InterceptLowQ1D2.png){width=350}|

Pictures below show 2D maps of low-charge gain intercepts for single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![2D Gain single-slope lowQ](images/gain/InterceptLowQ2D1.png){width=350}|![2D Gain dual-slope lowQ](images/gain/InterceptLowQ2D2.png){width=350}|

### Gain slope (high charge)

Pictures below show distributions of slopes of the linear fit for high charge (ToT$\geqslant$8), for single and dual-slope modes respectively. Since the former uses single-slope mode, there is no high-charge fit performed and the distribution contains only zeroes.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![1D Gain single-slope highQ](images/gain/SlopeHighQ1D1.png){width=350}|![1D Gain dual-slope highQ](images/gain/SlopeHighQ1D2.png){width=350}|

Pictures below show 2D maps of high-charge gain slopes for single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![2D Gain single-slope highQ](images/gain/SlopeHighQ2D1.png){width=350}|![2D Gain dual-slope highQ](images/gain/SlopeHighQ2D2.png){width=350}|

### Gain intercept (high charge)

Pictures below show distributions of linear fit intercepts for high charge (ToT$\geqslant$8), for single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![1D Intercept single-slope highQ](images/gain/InterceptHighQ1D1.png){width=350}|![1D Intercept dual-slope highQ](images/gain/InterceptHighQ1D2.png){width=350}|

Pictures below show 2D maps of high-charge gain intercepts for single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![2D Gain single-slope highQ](images/gain/InterceptHighQ2D1.png){width=350}|![2D Gain dual-slope highQ](images/gain/InterceptHighQ2D2.png){width=350}|

### Fit $\chi^2/N_{\text{D.o.F.}}$

Pictures below show distributions of fit $\chi^2/N_{\text{D.o.F.}}$ values for single and dual-slope modes respectively. For the single slope mode, most fits have fairly low $\chi^2/N_{\text{D.o.F.}}$ values.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![1D chi2 single-slope](images/gain/Chi2DoF1D1.png){width=350}|![1D chi2 dual-slope](images/gain/Chi2DoF1D2.png){width=350}|

Pictures below show 2D maps of per-pixel fit $\chi^2/N_{\text{D.o.F.}}$ values for single and dual-slope modes respectively.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![2D chi2 single-slope](images/gain/Chi2DoF2D1.png){width=350}|![2D chi2 dual-slope](images/gain/Chi2DoF2D2.png){width=350}|

### Fit Errors

Pictures below show 2D maps of fit errors for single and dual-slope modes respectively. The pixels shown in yellow had the fit failing to converge. It can be seen that for the dual-slope mode, over 2/3 of the pixels failed to achieve a proper fit, suggesting that better tuning of the chip is needed (threshold, Krummenacher current, etc.). This is also the reason why other 2D maps for the dual-slope mode contain a lot of empty bins.

|Single-slope mode|Dual-slope mode (6-to-4 bit conversion)|
|-----------------|---------------------------------------|
|![2D fit errors single-slope](images/gain/FitErrors1.png){width=350}|![2D fit errors dual-slope](images/gain/FitErrors2.png){width=350}|