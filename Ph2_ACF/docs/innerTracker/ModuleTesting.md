# Module testing

When dealing with modules you need to configure the `XML` file in a way to map the actual
hardware you have.
In particular you need to set the **chip ID** and the **lane** for every chip.
The `Number of active data` printed out on screen during the initialisation
process indicates which chips are sending the default signal pattern to the FC7. It doesn’t
necessary mean that you properly matched **chip ID** and **lane**, i.e. you might have set a
wrong **chip ID** and yet you might still get that all **lanes** are active.
The **chip ID** is only needed to address the chip in the programming process

## Lane mapping

If you don’t know the correspondence between **chip ID** and lane of your module you might
want to proceed in the following way:

1. Set the **chip ID** to 16 (8) for RD53B (RD53A), which is the `"broadcast"` address
2. Check if all **lanes** are active. If not, then you might need to adjust the internal voltage by
playing with the `VOLTAGE_TRIM` register
3. Once you get that all lanes are active then you can manually scan the **chip ID** from 0 to 15 (7) for RD53B (RD53A) in order to find out which is the right address (it is suggested to do so one chip at a time)

### Typical mapping for RD53B quad modules

| TBPX	                | TFPX                  |	TEPX                |
| --------------------- | --------------------- | --------------------- |
| chip ID 0 <-> lane 0  | chip ID 14 <-> lane 0	| chip ID 15 <-> lane 0 |
| chip ID 1 <-> lane 1	| chip ID 13 <-> lane 1	| chip ID 14 <-> lane 1 |
| chip ID 2 <-> lane 2	| chip ID 12 <-> lane 2	| chip ID 13 <-> lane 2 |
| chip ID 3 <-> lane 3	| chip ID 15 <-> lane 3	| chip ID 12 <-> lane 3 |

### Typical mapping for RD53A quad modules

| TBPX	                | TFPX                  |	TEPX               |
| --------------------- | --------------------- | -------------------- |
| chip ID 4 <-> lane 0  | chip ID 4 <-> lane 0	| chip ID 0 <-> lane 0 |
| chip ID 5 <-> lane 1	| chip ID 2 <-> lane 1	| chip ID 1 <-> lane 1 |
| chip ID 6 <-> lane 2	| chip ID 7 <-> lane 2	| chip ID 2 <-> lane 2 |
| chip ID 7 <-> lane 3	| chip ID 5 <-> lane 3	| chip ID 3 <-> lane 3 |