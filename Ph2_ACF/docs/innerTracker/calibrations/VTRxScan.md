# VTRx+ Light Yield Scan
<sub><sup>Last updated: 07.02.2025</sup></sub>

## Purpose

The purpose of VTRx+ Light Yield Scan is to help choosing the optimal bias and modulation currents for the VTRx+ lasers that send the optical data to the receiver on the SFP connector (the "upstream" data). The bias current is a constant current applied to the laser diode to maintain it just above its threshold level, ensuring it operates in the linear region. By keeping the laser in this state, it can respond more rapidly to modulation signals, which is essential for high-speed data transmission. In the VTRx+, the bias current corresponds to the logical `1` level in the transmitted signal. Then, the modulation current is superimposed on the bias current to vary the laser's output power, thereby encoding the data onto the optical signal. It represents the difference between the logical `1` and `0` levels. In the VTRx+, the modulation current is defined as this difference, effectively determining the amplitude of the optical signal corresponding to the data being transmitted. 

## Method

The VTRx+ Light Yield Scan performs a 2D measurement by scanning the VTRx+ bias current from `VTRxBiasStart` to `VTRxBiasStop` in steps of `VTRxBiasStep` and the modulation current from `VTRxModulationStart` to `VTRxModulationStop` in steps of `VTRxModulationStep`. At each point, the light intensity is measured by the receiver of the SFP connector and filled into a 2D histogram.

**Scan command:** `vtrx`

## Configuration Parameters

|Name                 |Typical Value|Description|
|---------------------|-------------|-----------|
|`VTRxBiasStart`      |40           |The starting value of the bias current|
|`VTRxBiasStop`       |56           |The final value of the bias current|
|`VTRxBiasStep`       |1            |The step size for the bias current scan|
|`VTRxModulationStart`|24           |The starting value of the modulation current|
|`VTRxModulationStop` |40           |The final value of the modulation current|
|`VTRxModulationStep` |1            |The step size for the modulation current scan|

## Expected Output
#### The light yield diagram
![LpGBT Eye Opening Diagram](images/vtrxscan/VTRxScan.png){width=400}
