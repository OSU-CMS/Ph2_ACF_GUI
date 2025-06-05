# FC7 Nomenclature

The FC7 board is an AMC for generic data acquisition and control applications. It hosts:

- The Xilinx Kintex-7 FPGA, providing a custom platform with a large array of configurable inputs and outputs
- Two on-board FPGA Mezzanine Card (FMC) sockets to access the FPGA
- The optical mezzanine, connecting the optical fibers used to communicate between the FC7 and the module under test.
- The SD card slot. The card can contain a selection of firmware versions to be loaded onto the FC7.
- μTCA Back-plane Connector

The Imperial College μTCA adapter card provides the power and the communication to the FC7.

[FC7 manual not available anymore but some info here.](https://iopscience.iop.org/article/10.1088/1748-0221/10/03/C03036/pdf)

In the image below you see an FC7 board with the names of the different
components:

![FC7](../images/FC7.png)

In the image below you see a QUAD with FMC connector.

![FMC_QUAD](../images/FMC_Quad_SFP-2.jpeg)

A Quad can host up to 4 SFP as the one below.

![FSP](../images/SFP.png)

An Octa is very similar to a Quad but can have up to 8 SFP, in 2 rows of
4 each.

The Dio5 board shown below can be used to see signals or to give
external signals:

![dio5](../images/dio5.png)