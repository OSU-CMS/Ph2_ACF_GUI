# Basic list of commands for the `fpgaconfig` program

 (run from the `choose_a_name` directory)

- Run the command: `fpgaconfig -c CMSIT_RD53A/B.xml -l` to check which firmware is on the microSD card
- Run the command: `fpgaconfig -c CMSIT_RD53A/B.xml -f firmware_file_name_on_the_PC.bit -i firmware_file_name_on_the_microSD` to upload a new firmware to the microSD card
- Run the command: `fpgaconfig -c CMSIT_RD53A/B.xml -i firmware_file_name_on_the_microSD` to load a new firmware from the microSD card to the FPGA
- Run the command: `fpgaconfig --help` for help
