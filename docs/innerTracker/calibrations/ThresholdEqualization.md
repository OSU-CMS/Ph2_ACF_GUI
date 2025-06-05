# Threshold Equalization
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The purpose of the Threshold Equalization scan is to reduce the threshold spread between different pixels. This is done by trimming the threshold of each individual pixel to get all pixels as close as possible to the mean threshold value. Threshold Equalization finds the best threshold-trimming DAC (`TDAC`) value for each pixel that makes the individual occupancy of each pixel closest to 50%. Optionally, it can find the optimal value for the global `TDAC` gain register (`DAC_LDAC_LIN`) that leads to the optimal threshold trimming strength (one that allows reducing the spread of thresholds the most).

As a rough idea, we can imagine the threshold $V_{\text{thr}}^i$ of $i$-th pixel being determined as follows:

$V_{\text{thr}}^i = V_{\text{mean}}(\texttt{GDAC}) + V_{\text{rnd}}^i - V_{\text{trim}}^i(\texttt{TDAC}^i, \texttt{LDAC}).$

Here, `GDAC` is short for `DAC_GDAC_*_LIN`, and `LDAC` is short for `DAC_LDAC_LIN`. $V_{\text{mean}}$ is the mean threshold value, determined by `GDAC` like this:

$V_{\text{mean}} (\texttt{GDAC}) = a + b \cdot \texttt{GDAC},$

where $a$ and $b$ are some DAC conversion constants. $V_{\text{rnd}}^i$ is a random component that is different for each pixel due to normal production imperfections. $V_{\text{trim}}^i$ is the per-pixel threshold trimming component that reduces the pixel threshold by some amount that can be written like this:<a name="vtrim-equation"> </a>

$V_{\text{trim}}^i (\texttt{TDAC}^i, \texttt{LDAC}) = \texttt{TDAC}^i \cdot (c + d \cdot \texttt{LDAC}),$

where $c$ and $d$ are some other DAC conversion constants. The main idea of this scan is to find such (5-bit for CROC) integer `TDAC` (and, optionally, `LDAC`) values that

$V_{\text{trim}}^i (\texttt{TDAC}^i, \texttt{LDAC}) \approx V_{\text{rnd}}^i \,,$

and thus

$V_{\text{thr}}^i \approx V_{\text{mean}} (\texttt{GDAC}).$

## Method

This scan is derived from [PixelAlive](PixelAlive.md). If a starting `TDAC` value is not specified in the XML file (`ResetTDAC`=-1), the scan just uses the current `TDAC` values of each pixel for the first iteration. The default way is starting with `TDAC`=16 for all pixels. First, the average threshold value is determined by performing a [binary search](https://en.wikipedia.org/w/index.php?title=Binary_search&oldid=1250479543) in `VCAL_HIGH` between the values `VCalHStart` and `VCalHStop`. When the threshold is found, the corresponding `VCAL_HIGH` value is used for the rest of the measurement. After that, a PixelAlive measurement is done and the occupancy for each pixel is determined. Then, a binary search for the best `TDAC` is done for each pixel: if the occupancy is larger than 50%, its `TDAC` is increased by 16/8/4/2/1, depending on the iteration, and vice-versa. After the scan is done, the final `TDAC` value that gets the pixel's occupancy closest to 50% is saved in the txt settings file.

![TDAC Irratiation](images/threqu/irradiation.png){align=right width=400}

Alternatively, if the `DoNSteps` parameter is set to a non-zero value (say, $n$), a search of $n$ steps spaced by 1 `TDAC` unit is done for each pixel, instead of the binary search. This can be useful if you had already equalized the pixel threshold, but then changed the global threshold and do not want to perform a full re-equalization (as it would
be an overkill) or if the TDAC values have shifted due to irradiation.

The Threshold Equalization scan can also tune the `DAC_LDAC_LIN` parameter, which determines the `TDAC` gain/strength (see [$V_{\text{trim}}^i$ definition](#vtrim-equation) above). If the setting `TDACGainNSteps`$\neq$0, the whole Threshold Equalization procedure will be repeated `TDACGainNSteps` times, with a different `DAC_LDAC_LIN` value each time, taken from an interval between `TDACGainStart` and `TDACGainStop`. Then, the best `DAC_LDAC_LIN` value is determined by choosing the one that minimizes the threshold dispersion.

**Scan command:** `threqu`

* Analog injection

## Configuration parameters

|Name           |Typical value|Description|
|---------------|-------------|-----------|
|`nEvents`      |100          |Number of injections per pixel for each measurement step|
|`nEvtsBurst`   |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`  |10           |Number of subsequent triggers sent for each injection |
|`INJtype`      |1            |Injection type, 1 – analog|
|`VCalHStart`   |100          |The lowest [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) value used for determining the threshold|
|`VCalHStop`    |1000         |The highest [`VCAL_HIGH`](TermExplanations.md#calibration-voltage-vcal) value used for determining the threshold|
|`TDACGainStart`|140          |The lowest value for `DAC_LDAC_LIN` scan|
|`TDACGainStop` |140          |The highest value for `DAC_LDAC_LIN` scan|
|`TDACGaiNSteps`|0            |Number of `DAC_LDAC_LIN` scan steps|
|`DoNSteps`     |0            |Number of steps for incremental search (=0 for full binary search)|
|`ResetTDAC`    |-1 or 16     |Starting TDAC value for all pixels (-1 keeps the previous values). Use 16 for starting from scratch, and -1 for `DoNSteps`$\neq$0|

## Expected output

### TDAC

Pictures below show the 1D distribution of TDAC values for each pixel and a 2D map of TDAC values for each pixel. A healthy 1D `TDAC` distribution should look like a Gaussian centered around 15-16, and have no high-valued bins at the tails (`TDAC`=0 and 31), as is the case with this one (although it could be perfected even more as `TDAC`=31 bin has slightly too many values).

![1D TDAC](images/threqu/TDAC1D.png){width=360}![2D TDAC](images/threqu/TDAC2D.png){width=360}

### Occupancy

Pictures below show the 1D distribution of per-pixel [occupancies](TermExplanations.md#pixel-occupancy) and a 2D map of occupancies for each pixel. Similarly to the `TDAC`
distribution, the occupancy distribution after a perfect threshold equalization should look like a (ideally narrow) Gaussian centered around 0.5, as is the case with this plot.

![1D Occupancy](images/threqu/Occ1D.png){width=360}![2D Occupancy](images/threqu/Occ2D.png){width=360}

### ToT

Pictures below show the 1D distribution and 2D per-pixel map of [ToT](TermExplanations.md#time-over-threshold-tot) values, respectively. Since the injection size equals the average threshold, most pixels either see no hit or measure ToT=0.

![1D ToT](images/threqu/ToT1D.png){width=360}![2D ToT](images/threqu/ToT2D.png){width=360}

### `LDAC_LIN`

If `TDACGainStart`, `TDACGainStop`, and  `TDACGainNSteps` are appropriately chosen, a plot showing the standard deviation of the threshold distribution for each tested `DAC_LDAC_LIN` value is produced, as shown in picture below.

![TDAC Gain Scan](images/threqu/TDACGainScan.png){width=400}

The best `DAC_LDAC_LIN` value (one that provides the smallest std. dev.) is saved in a separate plot, which can be seen in a picture below.

![Best TDAC Gain](images/threqu/TDACGain.png){width=400}

### Calibration result

Pictures below show the threshold distribution from [S-Curve](SCurve.md) measurement at different stages of chip threshold tuning, following the recommended tuning sequence.

|1. Untuned thresholds: GDAC=470|2. After threshold equalization: `DoNSteps`=0|
|--------------------------------|---------------------------------------------|
|![Untuned thresholds](images/threqu/Thresholds1.png){width=360}|![Equalized thresholds](images/threqu/Thresholds2.png){width=360}|
|**3. After lowering the global threshold to GDAC=432**|**4. After threshold equalization: `DoNSteps`=2**|
|![Lowered Thresholds](images/threqu/Thresholds3.png){width=360}|![Re-equalized thresholds](images/threqu/Thresholds4.png){width=360}|

## Tips

When initially configuring the chip, it is suggested to first run the Threshold Equalization at a high global threshold (say, 3000 electrons), then reduce the global threshold to the desired value, and, finally, re-tune the thresholds with `DoNSteps`=1 or 2.

**Important!** When running the Threshold Equalization at high thresholds, ensure that the `VCalHStop` value is high enough to include the thresholds of all pixels. If a pixel has a higher threshold than the `VCalHStop` value, its threshold will be set equal to `VCalHStop`, and the average threshold will be underestimated, leading to sub-optimal `TDAC` values, and a skewed threshold distribution as a result. The same applies if there are many pixels with thresholds below the `VCalHStart` value. One should always check the resulting `TDAC` and occupancy distributions and make sure that they have nice Gaussian shapes and no skew or tails. If the `TDAC` distribution, however, is centered around 15-16, but has high "tails" at 0 and 31, it is a sign that the full trimming range is not enough to equalize some of the thresholds and `TDAC` gain (`DAC_LDAC_LIN`) should be increased. A good starting value is `DAC_LDAC_LIN`=140.