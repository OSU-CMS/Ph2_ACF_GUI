# Injection delay scan
<sub><sup>Last updated: 16.12.2024</sup></sub>

## Purpose

We define **Injection Delay** as the time between the beginning of a bunch crossing and when a test charge is injected onto the bump. For CROC chips, the Injection Delay is measured in $0.78125$ ns time steps ($\sim\!1.5$ ns for RD53A).

Due to how the signal is shaped in the front-end, the efficiency of a pixel's response depends on the timing of the test charge injection within the bunch-crossing. It is possible that the signal that is larger than the threshold is not registered (in the correct bunch-crossing) because it either does not peak at the correct time or decays too quickly. Therefore, it is important to know the optimal injection delay so that we can minimize efficiency loss due to the timing of test charge injections in calibrations. The perfect injection delay would be the one that would make the signal peak right at the clock edge. The Injection Delay Scan finds the optimal value for this delay to be used in further calibrations.

## Method

The Injection Delay scan is derived from [PixelAlive](PixelAlive.md). It works by first setting injection delay to 0. Then it uses [Latency Scan](LatencyScan.md) to find the best latency value $N$, (scanning from `LatencyStart` to `LatencyStop`). After that, it sets the latency to $N-1$ and tries to delay the injection time so that the signal peaks in the next bunch-crossing in an optimal way. It scans the injection delay from 0 to 63 in steps of 1, repeating the PixelAlive scan for each delay value. At the end, the best injection delay is chosen as the smallest value that produces the highest average pixel occupancy (there may be multiple such values).

The optimal injection delay value should be entered into the XML file by hand, by editing the `CAL_EDGE_FINE_DELAY` parameter.

**Scan command:** `injdelay`

* Analog (or digital) injection
* **RD53A:** Only Linear FE / Fully configurable from config file

!!! warning "You have to update the config manually in the xml file"
	* Latency:
        * RD53B: `TriggerConfig`
        * RD53A: `LATENCY_CONFIG`
    * Injection delay: `CAL_EDGE_FINE_DELAY`

!!! note "Update of .txt files"
    In `v4-13` and older versions of `Ph2_ACF`, the received optimal values from this scan are not yet written into the .txt file describing the ROC configurations. This for instance also prevents `dirigent` from being able to update these values to the .xml file. This should be fixed in newer versions

## Configuration parameters

|Name           |Typical value|Description|
|---------------|-------------|-----------|
|`nEvents`      |100          |Number of injections per pixel|
|`nEvtsBurst`   |=`nEvents`   |Number of events readout from FPGA in one instance|
|`nTRIGxEvent`  |1            |Number of triggers for each injection (resolution of the latency scan)|
|`INJtype`      |1            |Injection type, 1 – analog|
|`LatencyStart` |110          |The minimum latency for the latency scan|
|`LatencyStop`  |150          |The maximum latency for the latency scan|
|`DoOnlyNGroups`|1            |How many subsets of pixels (432 pixels per subset) to use (0 for all pixels)|
|`VCAL_HIGH`    |             |Defines the size of injected charge, see [here](TermExplanations.md#calibration-voltage-vcal). Should be chosen so that $\Delta V_{\text{Cal}}$ is close to expected particle signal|

## Expected output

### Efficiency scan at different injection delays

Picture below shows how occupancy varies with the injection delay. The first maximum value in this plot indicates the best value for the injection delay of the chip.

![Injection Delay Scan](images/injdelay/InjectionDelayScan.png){width=400}

### Optimal injection delay

Histogram shown below is used to store the best injection delay value for the chip and hence shows only one non-empty bin at the corresponding delay.

![Injection Delay](images/injdelay/InjectionDelay.png){width=400}

## Tips

Generally, it is suggested to choose the size of the injected charge (`VCAL_HIGH`-`VCAL_MED`, see [here](TermExplanations.md#calibration-voltage-vcal)) to at least be close to the expected charge deposit of the particles that are going to be detected by the module.

After running the injection delay scan you should run [`SCurve`](SCurve.md) to measure the in-time threshold. Generally, one should see that the threshold becomes lower once the optimal injection delay is set.