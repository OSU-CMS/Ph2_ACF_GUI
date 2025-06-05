# Latency scan
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

We define **Latency** as the time from when the flight of a charged particle through the module induces a charge deposition in the sensor to when it can be read out as a hit in the buffer of the readout chip. For calibration purposes, the charge deposition can be emulated by an injected charge deposition. The Latency Scan finds the latency for each chip.

It is important to know the latency for each chip so that we know when data from a particular bunch crossing can be read out in the buffer and which buffer location corresponds to which bunch crossing. It is also important for future calibrations, e.g., if the latency is inaccurate, we might later think our threshold is too high because we are trying to read out from the buffer at the wrong time.

## Method

The Latency Scan is derived from [PixelAlive](PixelAlive.md). It works by setting the latency (`TriggerConfig` setting in the XML file) to a specific value and performing the PixelAlive scan to measure the average pixel occupancy. This is repeated for a range of latency values from `LatencyStart` to `LatencyStop` in steps of `nTRIGxEvent`. The `nTRIGxEvent` parameter can be thought of as the resolution of the scan.

**Scan command:** `latency`

* Analog or digital injection

!!! warning "You have to update the config manually in the xml file"
	* RD53B: `TriggerConfig`
	* RD53A: `LATENCY_CONFIG`

## Configuration parameters

|Name           |Typical value|Description|
|---------------|-------------|-----------|
|`nEvents`      |100          |Number of injections per pixel|
|`nEvtsBurst`   |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`  |10           |Number of triggers for each injection (resolution of the latency scan)|
|`INJtype`      |1            |Injection type, 1 – analog, 2 – digital|
|`LatencyStart` |110          |The minimum latency for the scan|
|`LatencyStop`  |150          |The maximum latency for the scan|
|`DoOnlyNGroups`|1            |How many subsets of pixels (432 pixels per subset) to use (0 for all pixels)|

## Expected output

Plots produced by the Latency Scan are shown below. In this case, the scan was performed in the full latency range (from 0 to 511) with `nTRIGxEvent`=1 (generally, this detailed scan is not recommended as it takes long time to run).

### Efficiency scan at different latencies

Picture below shows the average pixel [occupancy](TermExplanations.md#pixel-occupancy) for each of the tested latencies. The peak in this plot indicates the best/most common value for the latency of the chip. The low-occupancy noise in the bottom of this plot is exaggerated for the sake of example.

![Latency Scan](images/latency/LatencyScan.png){width=400}

### Optimal latency

Histogram shown below is used to store the best latency value for the chip and hence shows only one non-empty bin at the corresponding latency. 

![Optimal Latency](images/latency/Latency.png){width=400}

## Tips

If the latency of a chip is completely unknown, it is recommended to scan a wide range of latencies (e.g., from 0 to 511 which is the maximum possible range) with larger `nTRIGxEvent` (e.g., 10) to find the approximate location of the correct latency. Then, the scan can be repeated with a smaller range of latencies (e.g., equal to the previously discovered suitable window) and `nTRIGxEvent`=1, to find the correct latency value. It is also generally recommended to run with `DoOnlyNGroups`=1 to speed up the scan as the latency is the same for all pixels.

If used with TLU or external trigger source, make sure to have a relatively high hit occupancy in order to locate the maximum.