#November 19 2021:  Edited by Matt Joyce.  Added information for Purdue database to DBNames and DBServerIP

import os
from collections import defaultdict


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
defaultUSBPortHV = ["ASRL/dev/ttyUSB1::INSTR"]
# default model for HV power supply
defaultHVModel = ["Keithley 2410 (RS232)"]
# default USB port for LV power supply
defaultUSBPortLV = ["ASRL/dev/ttyUSB0::INSTR"]
# default model for LV power supply
defaultLVModel = ["KeySight E3633 (RS232)"]
# default model for LV power supply
defaultLVModel = ["KeySight E3633 (RS232)"]
#default BaudRate for Arduino sensor
defaultSensorBaudRate = 115200
#default DBServerIP
defaultDBServerIP = '127.0.0.1'
#default DBName
defaultDBName = 'SampleDB'
#List of expert users
ExpertUserList = [
	'mjoyce',
	'kwei'
]

FirmwareList = {}

if os.path.isfile(os.environ.get('GUI_dir')+"/fc7_ip_address.txt"):
	IPfile = open(os.environ.get('GUI_dir')+"/fc7_ip_address.txt")
	iplines = IPfile.readlines()
	for line in iplines:
		if line.startswith("#"):
			continue
		firmwareName = line.strip().split()[0]
		ip_address = line.strip().split()[1]
		if len(ip_address.split('.')) != 4:
			raise ValueError('{} is not valid ip address'.format(ip_address))
		FirmwareList[firmwareName] = ip_address

else:
	FirmwareList =  {
	'fc7.board.1'			 :  '192.168.1.80',
	'fc7.board.2'			 :  '127.0.0.1',#'192.168.1.81',
	}

DBServerIP = {
	'Central-remote'		 :  '0.0.0.0',
	'local'					 :  '127.0.0.1',
	'OSU-remote'			 :  '128.146.38.1',
	'Purdue-remote'			 :  'cmsfpixdb.physics.purdue.edu',
}


# Note: First element of list will be shown as default value
DBNames = {
	'All'					 :  ['phase2pixel_test', 'DBName2', 'DBName3'],
	'Central-remote'		 :  ['phase2pixel_test', 'DBName2', 'DBName3'],
	'local'					 :  ['SampleDB','phase2pixel_test'],
	'OSU-remote'			 :  ['SampleDB','phase2pixel_test'],
	'Purdue-remote'			 :  ['cmsfpix_phase2'],
}

# Set the IT_uTDC_firmware for test
FPGAConfigList =  {
	'fc7.board.1'			 :  'IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28',
	'fc7.board.2'			 :  'IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28'
}

ModuleType = {
	1	:	"SingleSCC",
	2	:	"TFPX Quad",
	3	:	"TEPX Quad",
	4	:	"TBPX Quad",
}

ModuleLaneMap = {
	"TFPX Quad": {"0":"4","1":"2","2":"7","3":"5"},
	"TEPX Quad": {"0":"0","1":"1","2":"2","3":"3"},
	"TBPX Quad": {"0":"4","1":"5","2":"6","3":"7"},
	"SingleSCC": {"0":"0"},
}

BoxSize = {
	"SingleSCC" : 1,
	"TFPX Quad"	: 4,
	"TEPX Quad" : 4,
	"TBPX Quad"	: 4,
}

HVPowerSupplyModel = {
	"Keithley 2410 (RS232)"    :  "Gui.python.Keithley2400RS232",
}

LVPowerSupplyModel = {
	"KeySight E3633 (RS232)"   :  "Gui.python.KeySightE3633RS232",
}

PowerSupplyModel_Termination = {
	"Keithley 2410 (RS232)"    :  '\r',
	"KeySight E3633 (RS232)"   :  '\r\n',
}

PowerSupplyModel_XML_Termination = {
	"Keithley 2410 (RS232)"    :  "CR",
	"KeySight E3633 (RS232)"   :  "CRLF",
}


ConfigFiles = {
	'Latency'                :  '/Configuration/Defaults/CMSIT.xml',
	'PixelAlive'             :  '/Configuration/Defaults/CMSIT.xml',
	'NoiseScan'              :  '/Configuration/Defaults/CMSIT.xml',
	'SCurveScan'             :  '/Configuration/Defaults/CMSIT.xml',
	'GainScan'               :  '/Configuration/Defaults/CMSIT.xml',
	'ThresholdEqualization'  :  '/Configuration/Defaults/CMSIT.xml',
	'GainOptimization'       :  '/Configuration/Defaults/CMSIT.xml',
	'ThresholdMinimization'  :  '/Configuration/Defaults/CMSIT.xml',
	'ThresholdAdjustment'    :  '/Configuration/Defaults/CMSIT.xml',
	'InjectionDelay'         :  '/Configuration/Defaults/CMSIT.xml',
	'ClockDelay'             :  '/Configuration/Defaults/CMSIT.xml',
	'BitErrorRate'           :  '/Configuration/Defaults/CMSIT.xml',
	'DataRBOptimization'     :  '/Configuration/Defaults/CMSIT.xml',
	'ChipIntVoltageTuning'   :  '/Configuration/Defaults/CMSIT.xml',
	'GenericDAC-DAC'         :  '/Configuration/Defaults/CMSIT.xml',
	'Physics'                :  '/Configuration/Defaults/CMSIT.xml',
	'AllScan'                :  '/Configuration/Defaults/CMSIT.xml',
}

Test = {
	'AllScan'                :  'noise',
	'StandardStep1'          :  'noise',
	'StandardStep2'          :  'threqu',
	'StandardStep3'          :  'scurve',
	'StandardStep4'          :  'injdelay',
	'StandardStep5'          :  'scurve',
	'Latency'                :  'latency',
	'PixelAlive'             :  'pixelalive',
	'NoiseScan'              :  'noise',
	'SCurveScan'             :  'scurve',
	'GainScan'               :  'gain',
	'ThresholdEqualization'  :  'threqu',
	'GainOptimization'       :  'gainopt',
	'ThresholdMinimization'  :  'thrmin',
	'ThresholdAdjustment'    :  'thradj',
	'InjectionDelay'         :  'injdelay',
	'ClockDelay'             :  'clockdelay',
	'BitErrorRate'           :  'bertest',
	'DataRBOptimization'     :  'datarbopt',
	'ChipIntVoltageTuning'   :  'voltagetuning',
	'GenericDAC-DAC'         :  'gendacdac',
	'Physics'                :  'physics',
}

TestName2File = {
	'Latency'                :  'Latency',
	'PixelAlive'             :  'PixelAlive',
	'NoiseScan'              :  'NoiseScan',
	'SCurveScan'             :  'SCurve',
	'GainScan'               :  'Gain',
	'ThresholdEqualization'  :  'ThrEqualization',
	'GainOptimization'       :  'GainOptimization',
	'ThresholdMinimization'  :  'ThrMinimization',
	'ThresholdAdjustment'    :  'ThrAdjustment',
	'InjectionDelay'         :  'InjectionDelay',
	'ClockDelay'             :  'ClockDelay',
	'BitErrorRate'           :  'BitErrRate',
	'DataRBOptimization'     :  'DataRBOpt',
	'ChipIntVoltageTuning'   :  'VoltageTuning',
	'GenericDAC-DAC'         :  'GenDACDAC',
	'Physics'                :  'Physics',
}

SingleTest = ['Latency','PixelAlive','NoiseScan','SCurveScan','GainScan',
	'ThresholdEqualization','GainOptimization','ThresholdMinimization',
	'ThresholdAdjustment','InjectionDelay','ClockDelay','BitErrorRate','DataRBOptimization','ChipIntVoltageTuning','GenericDAC-DAC','Physics']

CompositeTest = ['AllScan','StandardStep1','StandardStep2','StandardStep3','StandardStep4']
CompositeList = {
	'AllScan': ['NoiseScan','PixelAlive','ThresholdAdjustment',
				'ThresholdEqualization','SCurveScan', 'NoiseScan','ThresholdAdjustment',
				'SCurveScan','GainScan','GainOptimization',
				'InjectionDelay','SCurveScan'],
	'StandardStep1': ['NoiseScan','PixelAlive','ThresholdAdjustment'],
	'StandardStep2': ['ThresholdEqualization','SCurveScan', 'NoiseScan','ThresholdAdjustment'],
	'StandardStep3': ['SCurveScan','GainScan','GainOptimization'],
	'StandardStep4': ['InjectionDelay'],
	'StandardStep5': ['SCurveScan']
}
firstTimeList = ['AllScan', 'StandardStep1', 'PixelAlive']

# Reserved for updated value for XML configuration
updatedXMLValues = defaultdict(dict)

header = ['Source', 'Module_ID', 'User', 'Test', 'Time', 'Grade', 'DQMFile'] #Stop using


