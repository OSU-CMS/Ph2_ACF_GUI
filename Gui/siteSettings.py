######################################################################
# To be edited by expert as default setting for Hardware configuration
######################################################################
# default FC7 boardName
defaultFC7 = "fc7.board.1"
# default IP address of IP address
defaultFC7IP = '192.168.1.80'
# default fpga config
defaultFPGAConfig = 'IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28'
# default FMC board number
defaultFMC = '0'
# default USB port for HV power supply
defaultUSBPortHV = ["ASRL/dev/ttyUSBHV::INSTR"]
# default model for HV power supply
defaultHVModel = ["Keithley 2410 (RS232)"]
# default USB port for LV power supply
defaultUSBPortLV = ["ASRL/dev/ttyUSBLV::INSTR"]
# default model for LV power supply
defaultLVModel = ["KeySight E3633 (RS232)"]
# default model for LV power supply
defaultLVModel = ["KeySight E3633 (RS232)"]
# default setting for module ( "SingleSCC","TFPX Quad","TEPX Quad","TBPX Quad","Yellow Module (Purdue)"):
defaultModuleType = "SingleSCC"
# default mode for LV powering (Direct,SLDO,etc)
defaultPowerMode = "Direct"
defaultVoltageMap = {
    "Direct" : 1.28,
    "SLDO"   : 1.90
}
#default BaudRate for Arduino sensor
defaultSensorBaudRate = 115200
#default DBServerIP
defaultDBServerIP = '127.0.0.1'
#default DBName
defaultDBName = 'SampleDB'

##### The following settings are for SLDO scans developed for Purdue.#####
##### Do not modify these settings unless you know what you are doing.####
#default settings for SLDO scan.
defaultSLDOscanVoltage = 0.0
defaultSLDOscanMaxCurrent = 0.0


### Setting for Peltier Controller
usePeltier = True
defaultPeltierPort = '/dev/ttyUSBPeltier'
defaultPeltierBaud = 9600
defaultPeltierSetTemp = 20
defaultPeltierWarningTemp = 40
