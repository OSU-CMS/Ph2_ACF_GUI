# Term Explanations
<sub><sup>Last updated: 06.12.2024</sup></sub>

## Pixel Occupancy

Pixel occupancy (also called efficiency) tells how frequently a pixel is hit during a measurement. It is a measure of the pixel's efficiency and is used to identify dead or noisy pixels. The occupancy of a pixel is calculated by counting the number of times the pixel is hit during a measurement and dividing it by the total number of events:

$\text{Occupancy} = \dfrac{\text{# of registered hits}}{\texttt{nEvents}}.$

## Calibration Voltage (VCAL)

The calibration injection circuit of the CROC chip uses a differential voltage circuit to not rely on the local ground drops of the chip. The calibration voltage is then defined by two values, `VCAL_HIGH` and `VCAL_MED`, with the signal strength being defined as

$\Delta V_{\text{Cal}} = \texttt{VCAL_HIGH} - \texttt{VCAL_MED},$

with $V_{\text{Cal}}$ (or `VCAL`) meaning "Calibration Voltage", and the actual injected charge $Q$ (in coulombs) being equal to

$Q = x \; \Delta V_{\text{Cal}} \; C_{\text{Inj}},$

where $x$ is the DAC conversion factor and $C_{\text{Inj}}$ is the injection capacitance.
For the CROC chip, $C_{\text{Inj}} \approx 8\! \cdot\! 10^{-15}$ F, and $x \approx 10^{-4}$ V, when `SEL_CAL_RANGE`=0 (high-precision mode of the injection circuit), or $x \approx 2\cdot 10^{-4}$ V, when `SEL_CAL_RANGE`=1 (high-dynamic-range mode). This equates 1 $\Delta V_{\text{Cal}}$ unit to $\sim$5 electrons of charge for `SEL_CAL_RANGE`=0, and $\sim$10 electrons for `SEL_CAL_RANGE`=1.

## Time-over-Threshold (ToT)

Time-over-Threshold (ToT) is a 4-bit integer equal to the number of 25 ns bunch-crossings the voltage input to the comparator exceeds the threshold and is a measure of the charge deposited on the bump/sensor. ToT depends linearly on the charge, and a higher ToT indicates that a higher charge was deposited on the sensor. ToT values can range from 0 to 14, with 15 meaning that no hit observed (the hit detection threshold was not surpassed). If the dynamic range offered by the 4-bit ToT is not enough to measure high charge deposits, an optional 6-to-4-bit compression or the so-called "dual-slope" ToT mode can be used. In this mode, the ToT values >8 increase 4 times slower with charge, compared to the single-slope mode and allows measuring 2 times larger maximum charge, at the expense of charge resolution.