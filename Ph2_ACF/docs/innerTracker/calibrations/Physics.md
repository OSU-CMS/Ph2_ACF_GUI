# Physics
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The purpose of the Physics scan is actual physics data-taking during test beams, tests with radioactive sources, etc. It can mimic the operation of the final experimental system, receiving control, trigger, and clock commands from external equipment, and sending out periodic data to a Data Quality Monitoring (DQM) process. For testing purposes, it is also possible to run Physics without external equipment, with start and stop commands being issued by the Ph2_ACF software, and sending triggers periodically from FSM, or making use of the chip self-trigger.

## Method

In principle, the Physics scan is one of the simplest procedures in Ph2_ACF. It simply collects the data the chip sends out when it gets triggered and fills the histograms. If there is no external machine to send the start and stop signals, it can be managed by Ph2_ACF by setting the running time and the number of triggers one wants to accept before stopping.

From the collected data, the Physics scan produces occupancy and ToT histograms. There is also a possibility to output the binary data received from the chip by setting `SaveBinaryData`=1. The binary data file can then be converted into a ROOT file to analyze the data event by event. In that file, the pixel row, column, and [ToT](TermExplanations.md#time-over-threshold-tot) is provided for each hit, along with the event number, Trigger ID, and bunch crossing ID (if enabled in the settings file).

**Scan command:** `physics`

* Normally no injection, real particle hits are used

## Typical output

Below we proviee an example of the histograms produced by the Physics scan using a radioactive source placed on top of an SCC with HitOR self-trigger enabled.

### Occupancy

Picture below shows the 2D [occupancy](TermExplanations.md#pixel-occupancy) map, where a "blob" of hits produced by particles from a radioactive source can be observed.

![Occupancy](images/physics/Occupancy.png){width=400}

### ToT

Pictures below show a 1D distribution and a 2D map of total collected signal strength ([ToT](TermExplanations.md#time-over-threshold-tot)) produced by the hits during the data-taking.

![ToT 1D](images/physics/ToT1D.png){width=350}![ToT 2D](images/physics/ToT2D.png){width=350}

## Operation

Many different modes of operation are possible for the Physics scan, depending on the desired measurement equipment in use. Here, we will only outline the relevant settings for running a Physics scan with no external control. The command to run the Physics scan is as follows:
```
CMSITminiDAQ -f your_hw_description_file.xml -c physics -t time_in_seconds
```
where `time_in_seconds` is the time in seconds for which the calibration will run. If the running time is not given after the `-t` flag, the calibration will run until `triggers_to_accept` triggers get sent.

Table below contains the most general relevant parameters for the Physics scan together with a description of what each parameter does.

|Name                |Typical value|Description|
|--------------------|-------------|-----------|
|`triggers_to_accept`|10000        |Number of triggers to accept before stopping the scan|
|`nTRIGxEvent`       |10           |Number of subsequent triggers sent for each triggering event|
|`SaveBinaryData`    |1            |Whether to save all the binary event information in a separate file|
|`DataOutputDir`     |             |Location to save the binary files|
|`-t`                |60           |Terminal command flag to set the running time in seconds|

There are other relevant parameters that depend on the setup and the type of measurement. Two important examples are given in corresponding tables below.

### HitOR self-trigger

Table below contains the relevant parameters for the Physics scan **using HitOR self-trigger functionality**.

|Name                |Typical value|Description|
|--------------------|-------------|-----------|
|`SelfTriggerEn`     |1            |Whether to enable the self-trigger|
|`SelfTriggerDelay`  |30           |Delay applied to the HitOR pulse (in bunch crossings)|
|`TriggerConfig`     |42           |Trigger latency (in bunch crossings), should be $\approx$`SelfTriggerDelay`+12|
|`EnOutputDataChipId`|SCC$\rightarrow$0, Module$\rightarrow$1|Whether to output the chip ID number in the Aurora 64-bit block|
|`HitOrPatternLUT`   |0xFFFE       |Each bit represents a unique combination of the four HitOR lanes, 0xFFFE = OR of all combinations|
|`HIT_SAMPLE_MODE`   |0            |0 – asynchronous, 1 – synchronous sampling mode|
|`HITOR_MASK_0`      |0            |16-bit number to **disable** core columns 15:0|
|`HITOR_MASK_1`      |0            |16-bit number to **disable** core columns 31:16|
|`HITOR_MASK_2`      |0            |16-bit number to **disable** core columns 47:32|
|`HITOR_MASK_3`      |0            |6-bit number to **disable** core columns 53:48|
|`trigger_source`    |2            |Choosing the trigger source (2 – an OR of self-trigger and FSM)|
|`tp_fsm_trigger_en` |0            |Enable (1) or disable (0) triggering from FSM (in `fast_cmd_reg_2`)|
|`self_trigger_en`   |1            |Whether to enable the self-trigger (in `Aurora_block`)|

### External HitOR trigger (from FMC)

!!! warning This mode works only on Single-Chip Cards (SCCs).

Table below contains the relevant parameters for the Physics scan **using external HitOR trigger**. 

|Name                |Typical value|Description|
|--------------------|-------------|-----------|
|`SelfTriggerEn`     |0            |Whether to enable the self-trigger|
|`TriggerConfig`     |39           |Trigger latency (in bunch crossings)|
|`GP_LVDS_ROUTE_0`   |1821         |Information to direct through GP LVDS lanes 1:0|
|`GP_LVDS_ROUTE_1`   |1951         |Information to direct through GP LVDS lanes 3:2|
|`EnOutputDataChipId`|1            |Whether to output the chip ID number in the Aurora 64-bit block|
|`HitOrPatternLUT`   |0xFFFE       |Each bit represents a unique combination of the four HitOR lanes, 0xFFFE = OR of all combinations|
|`HIT_SAMPLE_MODE`   |0            |0 – asynchronous, 1 – synchronous sampling mode|
|`HITOR_MASK_0`      |0            |16-bit number to **disable** core columns 15:0|
|`HITOR_MASK_1`      |0            |16-bit number to **disable** core columns 31:16|
|`HITOR_MASK_2`      |0            |16-bit number to **disable** core columns 47:32|
|`HITOR_MASK_3`      |0            |6-bit number to **disable** core columns 53:48|
|`trigger_source`    |6            |Choosing the trigger source (6 – HitOR)|
|`HitOr_enable_l12`  |0            |Set the miniDP slot used for HitOR (0b0001 = leftmost slot)|

An external HitOR trigger can be used by connecting a second DisplayPort-to-Mini-DisplayPort (DP-miniDP) cable to the DP2 slot on the SCC and a bottom row of the miniDP slots on the FMC, as shown in the pictures below (the HitOR cable is shown in orange). The number of the miniDP slot in use must be correctly set in `HitOr_enable_l12` – 0b0001 enables the leftmost slot, 0b0010 enables the second from the left, and so on.

![HitOR Cable SCC](images/physics/HitOR_SCC_colored.jpg){width=350}![HitOR Cable FMC](images/physics/HitOR_FMC_colored.jpg){width=350}

There are 4 general-purpose LVDS lanes through which the HitOR data can be sent. The HitOR information is itself transferred in 4 lanes. To direct the information from the 4 HitOR lanes into the 4 LVDS lanes, one can set `GP_LVDS_ROUTE_0` = 1821 and `GP_LVDS_ROUTE_1` = 1951. However, other numeric combinations also allow the HitOR information to be sent through the LVDS lanes.

`GP_LVDS_ROUTE_0` is a stack of two 6-bit numbers: `GP_LVDS(1)` and `GP_LVDS(0)`. Similarly, `GP_LVDS_ROUTE_1` consists of `GP_LVDS(3)` and `GP_LVDS(2)`. Each of the four `GP_LVDS(*)` (where `*` is `0`, `1`, `2`, or `3`) registers can take a value from 0 to 63 to select a signal source for each LVDS output lane. Numbers 28..31 correspond to HitOR lanes 3..0, and 32 corresponds to a HitOR combination from self-trigger. Thus, one can direct each HitOR lane separately to a different general-purpose LVDS output lane or send the combined value through a single lane.

If we want to send the HitOR lane no. 3 to `GP_LVDS(1)` and HitOR lane no. 2 to `GP_LVDS(0)`, we need to combine the two 6-bit numbers corresponding to 28 and 29 into a single 12-bit number. 28 in binary is `0b011100`, and 29 is `0b011101`. Combining them is `0b011100011101`, which is 1821 in decimal. Similarly, combining 30 (`0b011110`, corresponds to HitOR lane no. 1) and 31 (`0b011111`, corresponds to HitOR lane no. 0) gives 1951 (`0b011110011111`).

Another simple way to find the correct number to enter into `GP_LVDS_ROUTE_*` (where `*` is `0` or `1`) would be multiplying the first number by 64 ($2^6$) and adding the second number: 28$\times$64+29=1821, 30$\times$64+31=1951. The default value for `GP_LVDS_ROUTE_*` in Ph2_ACF is 1495, or a number 23 repeated twice. As explained in the [CROC manual](https://cds.cern.ch/record/2665301), this mode corresponds to "Goes low when low power mode is enabled". This number should be used in case some of the HitOR lanes are not intended to be used or if one decides to use the combination of all HitOR lanes (32) in a single LVDS lane. In that case, one can set, e.g., `GP_LVDS_ROUTE_0`=1504 (23$\times$64+32) and `GP_LVDS_ROUTE_1`=1495 (23$\times$64+23).

A summary of this is given in the tables below.

|Selected signal  |`GP_LVDS(*)` value|
|-----------------|------------------|
|HitOR [0]        |31                |
|HitOR [1]        |30                |
|HitOR [2]        |29                |
|HitOR [3]        |28                |
|HitOR combination|32                |

|Register name    |Explanation                        | |
|-----------------|-----------------------------------|-|
|`GP_LVDS_ROUTE_0`|64$\times$`GP_LVDS(1)`+`GP_LVDS(0)`|![LVDS ROUTE 0](images/physics/LVDS_ROUTE_0.png){width=150}|
|`GP_LVDS_ROUTE_1`|64$\times$`GP_LVDS(3)`+`GP_LVDS(2)`|![LVDS ROUTE 1](images/physics/LVDS_ROUTE_1.png){width=150}|