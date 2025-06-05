# Tuning of CROC SCCs

## Power trimming on SCC Zurich cards

This procedure must be followed before biasing the module with the
high-voltage power supply. Power trimming is done partly by
adding/removing jumpers on the Zurich card PCB and partly via the
software registers `VOLTAGE_TRIM_ANA` and `VOLTAGE_TRIM_DIG`. Probing
can be done using a multimeter on the monitoring pins located on the
PCB.

Assemblies can be powered in three different modes:

- Direct mode
- Low-DropOut (LDO) mode
- Shunt Low-DropOut (SLDO) mode

In general, the resistor `R7` should never be mounted on the PCB.
Jumpers for `VDD_EFS` and `VOFS_IN-OUT` must always be placed. The
central pins of `VDD_PLL` and `VDD_CML` must always be connected to
`VDDA`.

Different jumper configurations enable specific powering modes:

- Direct powering mode:
    - `PWR_D/A`: central pin connected to `VDDD/A`
    - `VDD_PRE` must be connected
    - Outputs on the low-voltage power supply for the analog and
      digital domains must be such that the voltage on the SCC is not
      higher than 1.25 V, ideally 1.2 V. The analog and digital
      current compliances must be set to 2 A.
- LDO mode:
    - `PWR_D/A`: central pin connected to `VIND/A`
    - `VDD_PRE` and `SHUNT_EN` must be kept disconnected
    - Outputs on the low-voltage power supply for the analog and
      digital domains must initially be set to 1.7 V. The analog and
      digital current compliances must be set to 2 A. Fine tuning of
      the power will take place in a further step.
- SLDO mode:
    - Same as for LDO mode, but connecting `SHUNT_EN`
    - The current compliances for the analog and digital power domains
      must be set to 2 A. The output voltages must not exceed 2 V.
      Fine tuning of the power will take place in a further step.

The first step of the power trimming procedure is the same for all three
powering modes, and it is to adjust the jumpers on `IREF_TRIM` to obtain
a value for `VOFS` as close as possible to 0.5 V.

- For direct mode: after the tuning of `VOFS`, probe `VDDA` (with
    `GNDA_REF`) and `VDDD` (with `GNDD_REF`) from the monitoring pins to
    make sure they are between 1.2 V and 1.25 V.
- For LDO and SLDO modes: after the tuning of `VOFS`, probe `VDDA`
    (with `GNDA_REF`) and `VDDD` (with `GNDD_REF`) from the monitoring
    pins and adjust the values of `VOLTAGE_TRIM_ANA` and
    `VOLTAGE_TRIM_DIG` in the XML file, targeting 1.2 V.
- Measure `VREF_ADC` (with `GNDA_REF`).

![SCC module.](calibrations/images/scc_chip_pin.png){#fig:SCC width="62%"}

## Tuning of the AFE registers

The full procedure as given by L. Gaioni can be found [here](https://indico.cern.ch/event/1247433/).

### Main contributions to the current consumption

- Pre-amplifier =  3 μA per pixel (total additional 435 mA)
- Comparator = 1.1 μA per pixel for low thresholds,  
    2.2 μA per pixel for very high thresholds
   (total additional 160 mA and 320 mA respectively)
- TDAC = 400 nA per pixel (total additional 58 mA)

### Procedure

- Set all `GDAC_LINs` = 900, all `TDACs` = 0.
- Set `COMP_LIN` = 0 and measure the current $I_{A}$. Tune
    `COMP_LIN` to the value `COMP_LIN'`resulting in a total current
    consumption:<br>
    $I_{A}^{(1)} = I_{A} + 319 \mathrm{mA}$ (319 mA corresponds
    to 2.2 µA per pixel).
- Set `PREAMP_BIAS_LIN` = 0 and measure $I_{A}^{(2)}$. Tune
    `PREAMP_BIAS_LIN` to the value resulting in a total current
    consumption:<br>
    $I_{A}^{(3)} = I_{A}^{(2)} + 435 \mathrm{mA}$ (435 mA
    corresponds to 3 µA per pixel).
- Set all `TDACs` = 16 and tune `LDAC_LIN` to the value resulting
    in a total current consumption:<br>
    $I_{A}^{(4)} = I_{A}^{(3)} + 58 \mathrm{mA}$ (58 mA
    corresponds to 400 nA per pixel).
- Update `FC_BIAS_LIN` and `COMP_TA_LIN` to:<br>
    |`FC_BIAS_LIN`| = |`FC_BIAS_LIN`| $\cdot$ |`COMP_LIN'`|/|`COMP_LIN`|<br>
    |`COMP_TA_LIN'`| = |`COMP_TA_LIN`| $\cdot$ |`COMP_LIN'`|/|`COMP_LIN`|<br>
    with `COMP_LIN` being the initial value for
    the register (before the tuning). This is to restore the
    appropriate biasing of the secondary branches of the
    pre-amplifier and the comparator.
- Tune `KRUM_CURR_LIN` to an average ToT = 5.3 or average ToT = 2
    for slow and fast discharge respectively, having tuned the
    matrix to roughly 1000 e-, injecting a charge of 6000 e- with
    `pixelalive` and checking the ToT distribution in the `Results`
    folder.

## Tuning of fresh and irradiated modules

Copy the `CMSIT_RD53B.txt` and `CMSIT_RD53B.xml` files into the main directory.  
Set `VCAL_MED` = 100 and `VCAL_HIGH` to inject the equivalent charge produced by a MIP in 150 μm of silicon (12000 e-):

- `VCAL_HIGH` = 2500 if `SEL_CAL_RANGE` = 0
- `VCAL_HIGH` = 1300 if `SEL_CAL_RANGE` = 1

1. Adjust the global threshold to 2000 e- for fresh modules or 3000 e- for irradiated modules:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c thradj`
    - Update the `GDAC` values in `CMSIT_RD53B.xml`
1. Equalize the threshold setting `DoNSteps` = 0:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c threqu`
1. Find the best latency and the best setting for `CAL_EDGE_FINE_DELAY`:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c injdelay`
    - Update the values for `TriggerConfig` and `CAL_EDGE_FINE_DELAY` in `CMSIT_RD53B.xml`
    - If the newly found best value for `CAL_EDGE_FINE_DELAY` differs more than 4 DAC with respect to the previous one,
      redo the threshold adjustment (and optionally the equalization) and update the `GDAC` values in `CMSIT_RD53B.xml` before running a noise scan.
1. Mask the noisy pixels:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c noise`
1. Check the distribution of the thresholds:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c scurve`
1. Repeat this procedure again for a target threshold of 2000 e- for irradiated modules.

Then, assuming a target threshold of 1000 e-:

1. Adjust the global threshold to 1200 e-:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c thradj`
    - Update the `GDAC` values in `CMSIT_RD53B.xml`
1. Equalize the threshold setting `DoNSteps` = 1:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c threqu`
1. Find the best setting for `CAL_EDGE_FINE_DELAY`:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c injdelay`
    - Update the value for `CAL_EDGE_FINE_DELAY` in `CMSIT_RD53B.xml`
    - If the newly found best value for `CAL_EDGE_FINE_DELAY` differs more than 4 DAC with respect to the previous one,
      redo the threshold adjustment (and optionally the equalization) and update the `GDAC` values in `CMSIT_RD53B.xml`
      before running a noise scan.
1. Mask the noisy pixels:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c noise`
1. Check the distribution of the thresholds:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c scurve`
1. Adjust the global threshold to 1000 e-:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c thradj`
    - Update the `GDAC` values in `CMSIT_RD53B.xml`
1. Without re-running the threshold equalization, mask the noisy pixels:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c noise`
1. Run a `pixelalive` setting the appropriate rate for `OccPerPixel` to mask the stuck pixels
    (e. g. `OccPerPixel` = 0.9 to mask all pixels that detect less than 90 % of the injected signals):
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c pixelalive`
1. Check the distribution of the thresholds:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c scurve`
1. Check the in-time threshold by setting `nTRIGxEvent` = 1:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c scurve`
1. Once the target threshold is reached, run a `gain scan`:
    - `CMSITminiDAQ -f CMSIT_RD53B.xml -c gain`

If the aim is to tune to a higher threshold, replace the tuning procedure in steps
7 to 10 (1200 e-) with the desired target threshold and skip steps 12, 13 and 15.
