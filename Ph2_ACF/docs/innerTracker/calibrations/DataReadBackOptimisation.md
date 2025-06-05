# Data Read Back Optimisation
<sub><sup>Last updated: 16.12.2024</sup></sub>

## Purpose

The purpose of the Data Readback Optimization scan is to find the optimal values of TAP0, TAP1, and TAP2 registers. These registers control the current mode logic (CML) drivers used for outputing the data from the RD53 chip. TAP0 generally controls the amplitude difference between the logical 0 and 1, whereas TAP1 and TAP2 control the pre-emphasis of the signal, "dipping" the signal before the transition from 0 to 1, overshooting the signal after the transition, and vice versa (more info can be found in [CROC manual](https://cds.cern.ch/record/2665301)).

## Method

The Data Readback Optimization scan is derived from [BER Test](BERTest.md). It performs three separate scans: one each for TAP0 (`DAC_CML_BIAS_0`), TAP1 (`DAC_CML_BIAS_1`), and TAP2 (`DAC_CML_BIAS_2`). The range of the TAP* scan is determined by `TAP*Start` and `TAP*Stop` settings (here, `*` can be either 0, 1, or 2), with the number of points always being either 10 or `TAP*Stop`-`TAP*Start`, if `TAP*Stop`-`TAP*Start`<10. At each point of the scan, a BER Test is performed. After the scan finishes, the TAP\* value providing the lowest Bit Error Rate is reported as the best. If there are multiple TAP\* values with the lowest BER, (which is veryoften the case as many points have BER=0), the lowest one is chosen.

The optimal TAP* values should be set by changing the  `DAC_CML_BIAS_*` registers in the XML file by hand.

**Scan command:** `datarbopt`

!!! warning "You have to update the config manually in the xml file"
    * TAP{0,1,2}: `DAC_CML_BIAS_{0,1,2}`

## Configuration Parameters

|Name            |Typical value|Description|
|----------------|-------------|-----------|
|`chain2Test`    |0            |Chain under test: 0$\rightarrow$BE-FE, 1$\rightarrow$BE-LpGBT, 2$\rightarrow$LpGBT-FE|
|`byTime`        |1            |Chooses whether the test duration is given in seconds or \#frames|
|`framesORtime`  |10           |Duration of the test: in seconds or \#frames (see `byTime`)|
|`TAP0Start`     |0            |Lowest value for TAP0 scan|
|`TAP0Stop`      |1023         |Highest value for TAP0 scan|
|`TAP1Start`     |0            |Lowest value for TAP1 scan|
|`TAP1Stop`      |511          |Highest value for TAP1 scan|
|`TAP2Start`     |0            |Lowest value for TAP2 scan|
|`TAP2Stop`      |511          |Highest value for TAP2 scan|

The number of steps is `10`or less depending whether `TAPxStop`-`TAPxStart`<10

## Expected Output

Pictures below show BER dependence on TAP0, TAP1, and TAP2 respectively. It can be noticed that BER sharply increases at very low TAP0, or at high TAP1 or TAP2. Consequently, the best TAP1 and TAP2 values suggested by the scan are 51 (although, 0 should be just as good), whereas the suggested TAP0 value is 204.

![TAP0 Scan](images/datarbopt/TAP0scan.png){width=350}![TAP1 Scan](images/datarbopt/TAP1scan.png){width=350}![TAP2 Scan](images/datarbopt/TAP2scan.png){width=350}

Histograms shown below should, in theory, contain the suggested TAP0, TAP1, and TAP2 values respectively. However, as one can see in the first figure, the suggested TAP0 value in this plot is also equal to zero, indicating that there might be some problem when filling the histogram, as the text output recommends TAP0=204.

![Best TAP0](images/datarbopt/TAP0.png){width=350}![Best TAP1](images/datarbopt/TAP1.png){width=350}![Best TAP2](images/datarbopt/TAP2.png){width=350}

Text output example snippets are shown for reference below (chunks of less relevant output are skipped, as indicated by the black dots).

![Data Read Back Optimisation Output](images/datarbopt/datarboptOutput.png){width=750}