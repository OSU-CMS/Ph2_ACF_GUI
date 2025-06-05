# XML configuration file

Main configuration file describing the setup, scan and monitoring settings (can be outsourced in another file):

* `Ph2_ACF/settings/CMSIT_RD53A.xml` for RD53A
* `Ph2_ACF/settings/CMSIT_RD53B.xml` for RD53B

## Chip definition

!!! info "TODO: module/chip definition"

The chip is defined as follows in the XML, followed by the chip settings:

* For RD53A:
  ```xml
  <RD53A Id="0" enable="1" Lane="0" configFile="CMSIT_RD53A.txt" RxGroups="0001" RxPolarity="0" TxGroup="2" TxChannel="0" TxPolarity="0" Comment="RD53A 50x50">
  ```
* For RD53Bv1 (CROCv1):
  ```xml
  <RD53Bv1 Id="15" enable="1" Lane="0" eFuseCode = "0" configFile="CMSIT_RD53Bv1.txt" RxGroups="0001" RxPolarity="0" TxGroup="2" TxChannel="0" TxPolarity="0" Comment="RD53B 50x50">
  ```
* For RD53Bv2 (CROCv2):
  ```xml
  <RD53Bv2 Id="15" enable="1" Lane="0" eFuseCode = "0" configFile="CMSIT_RD53Bv2.txt" RxGroups="0001" RxPolarity="0" TxGroup="2" TxChannel="0" TxPolarity="0" Comment="RD53B 50x50">
  ```
The corresponding configuration txt files should be copied from the `Ph2_ACF/settings/RD53Files` directory to the working directory and set to the `configFile` setting accordingly.

### Chip monitoring

The chip **monitoring setup** is described [here](Monitoring.md).

### Data merging

```xml
<LaneConfig primary="1" 
            outputLanes="0001" 
            singleChannelInputs="0000"  <-- [1st ch, 2nd ch, 3d ch, 4th ch]
            dualChannelInput="0000" />
```

* primary (1/0): says whether the chip is the master
* outputLanes (0-4): says which output lanes are enabled and how they are mapped
   * For example:  
      `outputLanes="0001"` ➜ only one lane is enabled and it’s mapped to the rst line  
      `outputLanes="4321"` ➜ all four lanes are enabled and they are mapped in the natural way  
      `outputLanes="1234"` ➜ all four lanes are enabled and they are mapped in the reverse way  
* `singleChannelInputs (1/0)`: says which input channel is enabled as single lane
* `dualChannelInput (1/0)`: says which input channel is enabled as “double-channel” i.e. bonded
   * For example:  
     `dualChannelInput="0101"` ➜ the first and third channels are bonded

!!! info "Running CROC SCCs in KSU hybrid ports 2 and 3"
    As the Aurora lanes are inversed w.r.t. the RD53A SCC, only CROC lanes 2 and 3 are connected for these connectors.
    Set `outputLanes` to `1234` for this module.

## Settings section

!!! info "Outsourcing scan settings"
    The `<settings>` section of the main XML file can be moved to a separate file to disentagle setup and scan configuration. See `-s` flag in `CMSITminiDAQ`

```xml
<Setting name="nEvents">             100 </Setting> 
<Setting name="nEvtsBurst">          100 </Setting>
```

### Injection

```xml
<Setting name="nTRIGxEvent">          10 </Setting>
<Setting name="INJtype">               1 </Setting> --> 0: no injection 
                                                        1: analog 
                                                        2: digital 
                                                        3: custom from txt 
                                                        4: X-talk coupled pixels (analog) 
                                                        5: X-talk decoupled pixels (analog)
```

### Masks and regions

```xml
<Setting name="ResetMask">             0 </Setting>
<Setting name="ResetTDAC">            -1 </Setting>
<Setting name="DoDataIntegrity">       0 </Setting>

<Setting name="ROWstart">              0 </Setting>
<Setting name="ROWstop">             335 </Setting>
<Setting name="COLstart">              0 </Setting>
<Setting name="COLstop">             431 </Setting>
```

### Latency scan

```xml
<Setting name="LatencyStart">          0 </Setting>
<Setting name="LatencyStop">         511 </Setting>
```

### VCAL settings

Used in `scurve`, `gain`, `gainopt` and `thradj` calibrations

```xml
<Setting name="VCalHstart">          100 </Setting>
<Setting name="VCalHstop">          1100 </Setting>
<Setting name="VCalHnsteps">          50 </Setting>
<Setting name="VCalMED">             100 </Setting>
```

### `Gainopt` calibration

```xml
<Setting name="TargetCharge">      10000 </Setting>
<Setting name="KrumCurrStart">         0 </Setting>
<Setting name="KrumCurrStop">        210 </Setting>
```

### Threshold equalisation

```xml
<Setting name="TDACGainStart">       140 </Setting>
<Setting name="TDACGainStop">        140 </Setting>
<Setting name="TDACGainNSteps">        0 </Setting>
<Setting name="DoNSteps">              0 </Setting>

<Setting name="ThrStart">            400 </Setting>  <-- used in thrmin and thdadj
<Setting name="ThrStop">             450 </Setting>  
<Setting name="TargetThr">          2000 </Setting>  <-- used in thradj calibration
<Setting name="TargetOcc">          1e-6 </Setting>  <-- used in thrmin calibration
```

* In a `thrmin` run, whose purpose is to find the lowest threshold based on noise occupancy, the
threshold is lowered within the range `[ThrStart, ThrStop]` in such a way to have an average
occupancy that meets `TargetOcc`.

### `pixelalive` and noise masking

```xml
<Setting name="OccPerPixel">        2e-5 </Setting>  
<Setting name="MaxMaskedPixels">       1 </Setting> 
<Setting name="UnstuckPixels">         0 </Setting>
```

* In a `noise` run, whose purpose is to find noisy
pixels, any pixel which has an occupancy greater
than `OccPerPixel` is consequently masked
* In a `pixelalive` run, whose purpose is to test
the pixels and find out whether there are dead
ones, any pixel which has an occupancy smaller
than `OccPerPixel` is consequently masked

### Voltage tuning (`voltagetuning`)

```xml
<Setting name="VDDDTrimTarget">     1.20 </Setting>
<Setting name="VDDATrimTarget">     1.20 </Setting>
<Setting name="VDDDTrimTolerance">  0.02 </Setting>
<Setting name="VDDATrimTolerance">  0.02 </Setting>
```

### `datarbopt`

```xml
<Setting name="TAP0Start">             0 </Setting>
<Setting name="TAP0Stop">           1023 </Setting>
<Setting name="TAP1Start">             0 </Setting>
<Setting name="TAP1Stop">            511 </Setting>
<Setting name="InvTAP1">               1 </Setting>
<Setting name="TAP2Start">             0 </Setting>
<Setting name="TAP2Stop">            511 </Setting>
<Setting name="InvTAP2">               0 </Setting>
```

### BERT (`bertest`)

```xml
<Setting name="chain2Test">            0 </Setting>
<Setting name="byTime">                1 </Setting>
<Setting name="framesORtime">         10 </Setting>
```

### `datatransmissiontest`

```xml
<Setting name="TargetBER">          1e-5 </Setting>
```

### `genericdac`

```xml
<Setting name="RegNameDAC1"> user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse </Setting>
<Setting name="StartValueDAC1">       28 </Setting>
<Setting name="StopValueDAC1">        50 </Setting>
<Setting name="StepDAC1">              1 </Setting>
<Setting name="RegNameDAC2">   VCAL_HIGH </Setting>
<Setting name="StartValueDAC2">      300 </Setting>
<Setting name="StopValueDAC2">      1000 </Setting>
<Setting name="StepDAC2">             20 </Setting>
```

### Further scan settings

```xml
<Setting name="DoOnlyNGroups">         0 </Setting>  <-- = 0: run on all injection groups, i.e. on all pix.
                                                         > 0: run a subset of groups, i.e. subset of pix.
                                                         useful to speed up scans and calibrations
<Setting name="DisplayHisto">          0 </Setting>  <--  = 0: don't display histograms on screen
                                                          = 1: display histograms on screen
<Setting name="UpdateChipCfg">         1 </Setting>  <--  = 0: don't update chip con g le
                                                          = 1: update chip con g le
<Setting name="DisableChannelsAtExit"> 1 </Setting>
```

### Expert settings

```xml
<Setting name="DataOutputDir">           </Setting>
<Setting name="SaveBinaryData">        0 </Setting>
<Setting name="nHITxCol">              1 </Setting>
<Setting name="InjLatency">           32 </Setting>
<Setting name="nClkDelays">         1300 </Setting>
```

* DataOutputDir:   set the output directory for all data. If not specified files will be saved in ./Results/
* SaveBinaryData = 0: do not save raw data in binary format; SaveBinaryData = 1: save raw data in binary format
* nHITxCol:        number of simultaneously injected pixels per column (it must be a divider of chip rows)
* InjLatency:      controls the latency of the injection in terms of 100ns period (up to 4095)
* nClkDelays:      controls the delay between two consecutive injections in terms of `100 ns` period (up to 4095)

## Trigger and clock settings

```xml
       <Register name="fast_cmd_reg_2">
          <Register name="trigger_source"> 2 </Register>  <-- 1=IPBus
                                                              2=Test-FSM (default)
                                                              3=TTC
                                                              4=TLU (via DIO5 board)
                                                              5=External
                                                              6=Hit-Or
                                                              7=User-defined frequency
          <Register name="HitOr_enable_l12"> 0 </Register>
          <!--  -->
        </Register>
```

* `HitOr_enable_l12`: Enable HitOr port: set trigger_source to proper value then this register, `0b0001` enable HitOr from left-most connector, `0b1000` enable HitOr from right-most connector

### Triggering on the CROC HitOr

!!! info "This works only with SCCs as modules don't have the HitOr connected"

#### Hardware

Connect the 2nd DP plug on the SCC (DP2/rightmost connector on Bonn/Zurich SCCs) to one of the first two (from left) connectors on the lower row of the KSU FMC (1st=J5=HitOr_1, 2nd=J8=HitOr_2)

#### Configuration

1. **Set trigger source to HitOr and turn on the correct input DP on the KSU FMC**  
    Set in the `CMSIT_RD53B.xml` file

    ```xml
     <Register name="fast_cmd_reg_2">
             <Register name="trigger_source"> 6 </Register>     <!-- 6 = Hitor input -->
             <Register name="HitOr_enable_l12"> 1 </Register>   <!-- 1 or 2 = turn on first(HitOr_1, see above) and/or
                                                                second (HitOr_2) connector -->
    ```

   **warning:** BUG: Specifying the input in binary as suggested in the XML file does not work.

2. **Route the HitOr signals to the 2nd DP connector**  
    This is specified in `GP_LVDS_ROUTE_0/1`, which are compound registers (two times two 6 bit registers).
    The assignment table is in the CROC manual in table 29/page 33:  HitOr3 = 28 ... HitOr0 = 31.
    So to turn on all HitOrs, you have to set the following in the chip configuration

    ```xml
        GP_LVDS_ROUTE_0 = 1821      <!-- ((28<<6) + 29) -->
        GP_LVDS_ROUTE_1 = 1951      <!-- ((30<<6) + 31) -->
    ```

3. **Turn on the HitOr per pixel and column**  
    * `CMSIT_RD53B.txt`: Set the HITOR bit for all relevant pixels to 1.
    * `CMSIT_RD53B.xml`: Set the HITOR_MASK_{0..3} to **0** for all columns that you want to **activate**.

4. **Find and adjust the latency (`TriggerConfig`)**  

!!! info "The HitOr is by definition `asynchronous`."
    Noise masking should only done in the asynchronous mode (`HIT_SAMPLE_MODE` = 0).

### DIO5 and TLU settings

```xml
        <Register name="ext_tlu_reg1">
          <Register name="dio5_ch1_thr"> 128 </Register>
          <Register name="dio5_ch2_thr">  40 </Register>
        </Register>

        <Register name="ext_tlu_reg2">
          <Register name="dio5_ch3_thr"> 128 </Register>
          <Register name="dio5_ch4_thr"> 128 </Register>
          <Register name="dio5_ch5_thr"> 128 </Register>

          <Register name="tlu_delay"> 0 </Register> <-- Needed to align TLU trigger tag with FC7 clock
                                                         (values 0-15 in unit of 1/16 of 40 MHz clk cycle)
        </Register>
```

* External triggers must be provided through a lemo cable to the DIO5 FMC board, input
number 2, in TTL standard, 50 Ohm impedance
* If using `trigger_source=4` ➜ terminate with 50 Ohm: busy to TLU (DIO5 FMC board
output number 3) and clk to TLU (DIO5 FMC board output number 1) and use same
length lemo cables

### External clock and triggers for `physics`

```xml

        <Register name="reset_reg">
          <Register name="ext_clk_en"> 0 </Register>  <-- 0: (default) internal clock
                                                          1: external clock
        </Register>

        <Register name="fast_cmd_reg_3">
          <Register name="triggers_to_accept"> 10 </Register>  <--  used in physics: total number of triggers to readout (0 = no limit)
        </Register>

        <Register name="fast_cmd_reg_7">
          <Register name="autozero_freq"> 1000 </Register>
          <!-- In units of 10MHz clk cyles -->
        </Register>
```

* External clock must be provided through a lemo cable to the DIO5 FMC board, input number
5, in TTL standard, 50 Ohm impedance
