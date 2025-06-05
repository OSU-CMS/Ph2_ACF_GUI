# Monitoring

To enable the periodic readout of quantities like voltages, currents and temperatures you need to enable the monitoring process via these lines in the configuration file:

```xml
<MonitoringSettings>
    <Monitoring type="RD53B" enable="1" silentRunning="0">
      <MonitoringSleepTime> 1000 </MonitoringSleepTime>
      <MonitoringElement device="RD53"  register="VINA"                 enable="1"/>
      <MonitoringElement device="RD53"  register="VDDA"                 enable="1"/>
      <MonitoringElement device="RD53"  register="ANA_IN_CURR"          enable="1"/>
      <MonitoringElement device="RD53"  register="VIND"                 enable="1"/>
      <MonitoringElement device="RD53"  register="VDDD"                 enable="1"/>
      <MonitoringElement device="RD53"  register="DIG_IN_CURR"          enable="1"/>
      <MonitoringElement device="RD53"  register="Iref"                 enable="1"/>
      <MonitoringElement device="RD53"  register="POLY_TEMPSENS_TOP"    enable="1"/>
      <MonitoringElement device="RD53"  register="POLY_TEMPSENS_BOTTOM" enable="1"/>
      <MonitoringElement device="RD53"  register="TEMPSENS_ANA_SLDO"    enable="1"/>
      <MonitoringElement device="RD53"  register="TEMPSENS_DIG_SLDO"    enable="1"/>
      <MonitoringElement device="RD53"  register="TEMPSENS_CENTER"      enable="1"/>
      <MonitoringElement device="RD53"  register="INTERNAL_NTC"         enable="1"/>

      <MonitoringElement device="LpGBT" register="ADC4"                 enable="0"/>
      <MonitoringElement device="LpGBT" register="ADC7"                 enable="0"/>
      <MonitoringElement device="LpGBT" register="TEMP"                 enable="0"/>
      <MonitoringElement device="LpGBT" register="VDDTX"                enable="0"/>
      <MonitoringElement device="LpGBT" register="VDDRX"                enable="0"/>
      <MonitoringElement device="LpGBT" register="VDDA"                 enable="0"/>
      <MonitoringElement device="LpGBT" register="VDD"                  enable="0"/>
      <MonitoringElement device="LpGBT" register="PUSMStatus"           enable="0"/>
    </Monitoring>
  </MonitoringSettings>
```
Here, <code style="color:CornflowerBlue;">MonitoringSleepTime</code> sets the monitoring period in μs.
To avoid printout on the screen, you can set <code>silentRunning=<t style="color: MediumSeaGreen;">"0"</t></code>.

## Output plots

Ad-hoc "observable vs time" ROOT historical plots can be produce produced. E.g.:
![LpGBT Eye Opening Diagram](images/Monitoring.png){width=500}

## RD53A monitoring

To readout all quantities of RD53A you need to put the following on your Single Chip Card (SCC):

* 0 Ohm resistor on R51 (R17) to have the right connection on the display port for the "Bonn" card ("Zurich" card)
* 0 Ohm resistor on R57 (R23) to have proper GND connection for the "Bonn" card ("Zurich" card)
* 10 kOhm resistor on R45 (R32) for the "Bonn" card ("Zurich" card)
* Jumper on I-MUX for current read back

See also tables in [RD53A](https://cds.cern.ch/record/2287593)/[RD53B](https://cds.cern.ch/record/2665301) manuals.

## NTC Monitoring

#### LpGBT ADC
If you intend to readout the NTCs with the LpGBT ADC (see settings that have <code>device=<t style="color: MediumSeaGreen;">"LpGBT"<t></code> above), then you need to provide also the `lookUpTable` for the NTCs, e.g.:
```xml
<OpticalGroup Id="0" enable="1" FMCId="L12">
      <NTCProperties type="Sensor" ADC="ADC4" lookUpTable="${PH2ACF_BASE_DIR}/settings/NTCFiles/ntc_1k.csv"/>
      <NTCProperties type="VTRx"   ADC="ADC7" lookUpTable="${PH2ACF_BASE_DIR}/settings/NTCFiles/vtrx_ntc_1k.csv"/>
...
```

Here:

* <code style="color: MediumSeaGreen;">"Sensor"</code> means that the NTC is near the sensor
* <code style="color: MediumSeaGreen;">"VTRx"</code> means that the NTC is near the VTRx

#### Internal ADC
In order to use the internal ADC set the following jumper:
![MUX Jumper](images/MUXjumper.png){width=450}

#### NTC1
NTC1 can be measured through the FMC ADC (just by enabling the *Monitoring* block of the DAQ), and it’s used as reference for temperature sensor calibration.
**In order to do so, you need to short the CONF1 and CONF2 jumpers on the back of the board:**
![CONF1/CONF2 Jumpers](images/CONF12.png){width=350}

NTC1 can also be read out through the NTC1 Pins:
![NTC1 Pins](images/NTC1.png){width=500}

#### NTC2
NTC2 can be measured through the CROC ADC via the *Monitoring* block of the DAQ, called <code style="color: MediumSeaGreen;">"INTERNAL_NTC"</code>.

Not strictly necessary:

* One can also verify/calibrate the NTC2 by reading it with an external multimeter through these pins:

![Vmux Pins](images/VmuxPins.png){width=400}

* One can also calibrate the CROC ADC by reading its output by one of the two setups (pins or LEMO, see below) and connect them to a multimeter:

![Vmux Pins](images/VmuxLemo.png){width=450}