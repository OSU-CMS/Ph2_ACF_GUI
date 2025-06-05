# Chip Internal Voltage Tuning
<sub><sup>Last updated: 16.12.2024</sup></sub>

## Purpose

The purpose of the Voltage Tuning calibration is to find the optimal digital and analog voltage trimming settings required for chip operation.

## Method

Voltage Tuning makes use of chip internal voltage (VDDA and VDDD) monitoring and tries to make the voltages as close to the target as possible (1.2 [1.3] V for both VDDA and VDDD on CROC [RD53A] chips). It scans through the voltage trimming register `VOLTAGE_TRIM`, splitting it into
`VOLTAGE_TRIM_ANA` and `VOLTAGE_TRIM_DIG` that trim VDDA and VDDD respectively.

The scan begins by measuring the digital voltage at the initial `VOLTAGE_TRIM_DIG` value and checking whether it is below or above the target voltage. Then, a sequence of `VOLTAGE_TRIM_DIG` values is generated for the scan, starting from the middle point and going up by 1, if the initial voltage was below the target, or going down by 1 otherwise. The scan stops when the target voltage gets overshot by more than a tolerance value, provided by the user, or the maximum/minimum possible register value is reached. When the scan stops, the register value that provides the voltage closest to the target is given. If the best value is different from the target by more than the given tolerance, the target is then reduced by 2$\times$tolerance and the
procedure is repeated.

The same procedure is then followed for the analog voltage. In that case, `VOLTAGE_TRIM_ANA` is tuned to the target VDDA value.

**Scan command:** `voltagetuning`

!!! warning "You have to update the config manually in the xml file"
	* VDDD: `VOLTAGE_TRIM_DIG`
	* VDDA: `VOLTAGE_TRIM_ANA`

## Configuration parameters

|Name               |Typical value         |Description|
|-------------------|----------------------|-----------|
|`VDDDTrimTarget`   |RD53B: 1.2, RD53A: 1.3|Voltage target in volts for VDDD|
|`VDDATrimTarget`   |RD53B: 1.2, RD53A: 1.3|Voltage target in volts for VDDA|
|`VDDDTrimTolerance`|0.01                  |Acceptable tolerance in volts for reaching VDDD target|
|`VDDATrimTolerance`|0.01                  |Acceptable tolerance in volts for reaching VDDA target|

## Expected output

Histograms shown below contain the suggested VDDD and VDDA trim values, respectively.

![Best VDDD](images/voltagetuning/VDDD.png){width=350}![Best VDDA](images/voltagetuning/VDDA.png){width=350}

A snippet of typical text output is shown in a picture below. It can be seen that in this case, the VDDD target had to be reduced as it initially could not be tuned to the desired tolerance.

![Text output](images/voltagetuning/VoltageTuningOutput.png){width=700}

## Tips

### RD53A

Although it is not strictly needed for a first test, in principle you need to tune:

|Name     |Target value    |Description|
|---------|----------------|-----------|
|$I_{ref}$|$4\;\mu\text{A}$|Tuned through external jumpers and measuring the voltage drop across a resistor (typically R45, $10\;\text{k}\Omega$)|
|$V_{ref}$|$0.9\;\text{V}$ |Tuned through the register `MONITOR_CONFIG_BG` in the XML file to have the right conversion factor for $\Delta V_{\text{Cal}}$ to electrons|

### RD53B

Although it is not strictly needed for a first test, in principle you need to tune:

|Name     |Target value    |Description|
|---------|----------------|-----------|
|$I_{ref}$|$4\;\mu\text{A}$|Tuned through external jumpers and measuring the voltage $V_{\text{offs}}$ to be $0.5\;\text{V}$|
|$V_{ref}$|$0.9\;\text{V}$ |Tuned through the register `MON_ADC_TRIM` in the XML file to have the right conversion factor for $\Delta V_{\text{Cal}}$ to electrons|
<!---
Double check if this is true 
--->