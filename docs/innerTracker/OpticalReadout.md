# Optical Readout Configuration

The portcard with 3 LpGBT should be powered at **10 V** and at most should require **200 mA**.

## LpGBT addresses

| LpGBT   | Address |
|---------|---------|
| LpGBT-1 | 0x70    |
| LpGBT-2 | 0x71    |
| LpGBT-3 | 0x73    |

![LpGBT Connection 1](images/LpGBT1.png){width=450}
![LpGBT Connection 2](images/LpGBT2.png){width=450}
![LpGBT Connection 3](images/LpGBT3.png){width=450}
![Optical Connection 1](images/OpticalReadout.png){width=400} ![Optical Connection 2](images/OpticalReadout2.png){width=128}

## XML Settings
```xml
<OpticalGroup Id="0" enable="1" FMCId="L12">
      <NTCProperties type="Sensor" ADC="ADC4" lookUpTable = "${PH2ACF_BASE_DIR}/settings/NTCFiles/ntc_1k.csv"/>
      <NTCProperties type="VTRx"   ADC="ADC7" lookUpTable = "${PH2ACF_BASE_DIR}/settings/NTCFiles/vtrx_ntc_1k.csv"/>
      <lpGBT_Files path="${PWD}" />
      <lpGBT_ConfigFile fileName="${PH2ACF_BASE_DIR}/settings/lpGBTFiles/lpgbt_calibration.csv" />
      <lpGBT Id="0" version="1" configFile="CMSIT_LpGBTv1.txt" ChipAddress="0x70" RxDataRate="1280" RxHSLPolarity="0" TxDataRate="160" TxHSLPolarity="0">
        <Settings
            />
      </lpGBT>

      <Hybrid Id="0" enable="1">
        <RD53_Files path="${PWD}/" />
        <!-- Available Comment fields are: 50x50, 25x100origR0C0, 25x100origR1C0, unknown, combined with RD53A, RD53B, dual, quad -->
        <RD53Bv1 Id="15" enable="1" Lane="0" eFuseCode = "0" configFile="CMSIT_RD53Bv1.txt" RxGroups="NNN1" RxPolarity="0" TxGroup="2" TxChannel="0" TxPolarity="0" Comment="RD53B 50x50">
          <LaneConfig isPrimary="1" masterLane="0" outputLanes="0001" singleChannelInputs="0000" dualChannelInput="0000" />
          <!-- Overwrite .txt configuration file settings -->
          <Settings
            ... 
            />
```
Here, <code><t style="color: CornflowerBlue;">OpticalGroup</t> Id</code> maps the SFP connector.

The code can handle both LpGBT-v0 and LpGBT-v1, transparently to the user. The user just needs to specify the version (`version`) and the appropriate register file (`configFile`) for this LpGBT chip in the following line:

<code><t style="color: CornflowerBlue;">&lt;lpGBT</t> Id=<t style="color: MediumSeaGreen;">"0"</t> <b>version=</b><b style="color: MediumSeaGreen;"><u>"1"</u></b>  <b>configFile=</b><b style="color: MediumSeaGreen;"><u>"CMSIT_LpGBTv1.txt"</u></b>...<t style="color: CornflowerBlue;">></t></code>

`RxGroups` maps the groups to the chip lanes (meaningful only for primary chips).
E.g., if you wrote <code>outputLanes=<t style="color: MediumSeaGreen;">"0012"</t></code>, and you want to associate `RxGroup` 3 to lane 1 and `RxGroup` 6 to lane 2, then you need to write <code>RxGroups=<t style="color: MediumSeaGreen;">"NN63"</t></code>.
Alternatively, if you wrote <code>outputLanes</t>=<t style="color: MediumSeaGreen;">"0021"</t></code>, then you still need to write: <code>RxGroups=<t style="color: MediumSeaGreen;">"NN63"</t></code>.
<code style="color: MediumSeaGreen;">"N"</code> indicates that the specific lane is not associated to any RxGroup.

Further details on how to configure the various adapter boards (FC7 $\leftrightarrow$ portcard $\leftrightarrow$ adapter board $\leftrightarrow$ RD53A/RD53B) can be found [here](https://indico.cern.ch/event/1391267/).

## VTRx+ Programming

* The VTRx+ is an LpGBT-slave chip
* LpGBT communicates to VTRx+ via I2C protocol

```xml
<OpticalGroup Id="0" enable="1" FMCId="L12">
      ...
      <lpGBT Id="0" version="1" configFile="CMSIT_LpGBTv1.txt" ChipAddress="0x70" RxDataRate="1280" RxHSLPolarity="0" TxDataRate="160" TxHSLPolarity="0">
        <Settings
            EPRX00ChnCntr_phase = "7"
            EPRX10ChnCntr_phase = "7"
            EPRX20ChnCntr_phase = "7"
            EPRX30ChnCntr_phase = "7"
            />
      </lpGBT>

```

Inside this <code><t style="color: CornflowerBlue;">Settings</t></code> section, the user can now put the registers referring to VTRx+ (they are treated as if they were LpGBT registers). A write/read test can be easily performed just by putting in the XML file any of the registers shown below (every write is associated to a read back check):
```
*-------------------------------------------------------------------------------------------------------
* Fake registers
*-------------------------------------------------------------------------------------------------------
* RegName                 Addr          Defval            Value                         BitSize
*-------------------------------------------------------------------------------------------------------
_I2CVTRxRegGCR            0x300         0d00              0d00                          8
_I2CVTRxRegSDA            0x302         0d00              0d00                          8

_I2CVTRxRegCH0BIAS        0x303         0d00              0d00                          8
_I2CVTRxRegCH0MOD         0x304         0d00              0d00                          8
_I2CVTRxRegCH0EMP         0x305         0d00              0d00                          8

_I2CVTRxRegCH1BIAS        0x306         0d00              0d00                          8
_I2CVTRxRegCH1MOD         0x307         0d00              0d00                          8
_I2CVTRxRegCH1EMP         0x308         0d00              0d00                          8

_I2CVTRxRegCH2BIAS        0x309         0d00              0d00                          8
_I2CVTRxRegCH2MOD         0x30A         0d00              0d00                          8
_I2CVTRxRegCH2EMP         0x30B         0d00              0d00                          8

_I2CVTRxRegCH3BIAS        0x30C         0d00              0d00                          8
_I2CVTRxRegCH3MOD         0x30D         0d00              0d00                          8
_I2CVTRxRegCH3EMP         0x30E         0d00              0d00                          8

_I2CVTRxRegStatus         0x314         0d00              0d00                          8
_I2CVTRxRegID             0x315         0d00              0d00                          8

_I2CVTRxRegUID0           0x316         0d00              0d00                          8
_I2CVTRxRegUID1           0x317         0d00              0d00                          8
_I2CVTRxRegUID2           0x318         0d00              0d00                          8
_I2CVTRxRegUID3           0x319         0d00              0d00                          8

_I2CVTRxRegSEU0           0x31A         0d00              0d00                          8
_I2CVTRxRegSEU1           0x31B         0d00              0d00                          8
_I2CVTRxRegSEU2           0x31C         0d00              0d00                          8
_I2CVTRxRegSEU3           0x31D         0d00              0d00                          8

_I2CMasterID              0x320         0d02              0d02                          8
_I2CFreq                  0x321         0d02              0d02                          8
_I2CSlaveAddress          0x322         0d80              0d80                          8
_I2CRegAddress            0x323         0d00              0d00                          8

```
