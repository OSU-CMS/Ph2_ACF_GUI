from icicle import adc_board
import time

with adc_board.AdcBoard(resource="ASRL/dev/ttyUSB4::INSTR") as dev:
    itnum = 0
    while True:
        now = time.asctime()
        voltages, temps = dev.query_adc()
        print(now, itnum)
        print(
            "Voltage\n"
            + "\n".join(
                [
                    "{: >2d} {: .3f} V".format(i, voltages[i])
                    for i in range(len(voltages))
                ]
            )
        )
        print(
            "Temperature\n"
            + "\n".join(
                ["{: >2d} {: .2f} C".format(i, temps[i]) for i in range(len(temps))]
            )
        )
        itnum += 1
        time.sleep(1.5)
