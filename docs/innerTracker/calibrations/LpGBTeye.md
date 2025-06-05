# LpGBT Eye Opening
<sub><sup>Last updated: 07.02.2025</sup></sub>

## Purpose

The purpose of LpGBT Eye Opening Scan is to measure the [eye diagram](https://en.wikipedia.org/wiki/Eye_pattern) of the LpGBT. The eye diagram is created by overlaying multiple signal waveforms on top of each other and is a graphical representation of the quality of the signal transmitted by the LpGBT. It is used to determine the quality of the optical signal coming into the LpGBT (the "downstream" data) and to identify any issues that may be present in the signal.

## Method

The LpGBT Eye Opening Scan makes use of the Eye Opening Monitor (EOM) block inside the LpGBT. As explained in the [LpGBT documentation](https://cds.cern.ch/record/2809058), the EOM allows the user to make an on-chip Eye-Diagram measurement of the incoming 2.56 Gb/s high speed data. The data is compared with a static voltage (`Vof`) using a differential comparator inside the EOP. The comparator is sampled with a phase-interpolated clock (`clkPi`). If the static voltage is within the data transmission voltage limits, the output of the comparator will toggle. If it is below, the output will have a static `0` and if it is above, the output will be a static `1`. The output of the comparator is fed to a 16-bit counter which can be read.

The LpGBT Eye Opening Scan scans the voltage (`Vof`) from 0 to 30 and the clock phase (`clkPi`) from 0 to 64 and reads the value of the counter at each point. The counter value is then filled into a 2D histogram which results in the eye diagram. 

**Scan command:** `eye`

## Configuration Parameters

|Name              |Typical Value|Description|
|------------------|-------------|-----------|
|`LpGBTattenuation`|0            |Whether to attenuate the input signal in the equalizer by -9.5 dB (0),  -3.5 dB (1, 2), or 0 dB (3) |

!!! warning "Attenuation of 0 dB should be used when using VTRX+"

## Expected Output
#### The eye diagram
![LpGBT Eye Opening Diagram](images/lpgbteye/LpGBTeye.png){width=400}
