#November 19 2021:  Edited by Matt Joyce.  Added information for Purdue database to DBNames and DBServerIP

import os
from collections import defaultdict
from Gui.siteSettings import *

#List of expert users
ExpertUserList = [
	'mjoyce',
	'kwei',
	'localexpert'
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
	
'''
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
'''

# Note: First element of list will be shown as default value
# Note: The varibale name is same as hostname
dblist = []
class DBServerIP:
	All = "All"
	All_list = ['phase2pixel_test', 'DBName2', 'DBName3']
	def __init__(self,DBhostname,DBIP,DBName):
		self.DBhostname = DBhostname
		self.DBIP = DBIP
		self.DBName = DBName
Central_remote= DBServerIP('Central_remote','0.0.0.0',['phase2pixel_test', 'DBName2', 'DBName3'])
dblist.append(Central_remote.DBhostname)
local = DBServerIP('local','127.0.0.1',['SampleDB','phase2pixel_test'])
dblist.append(local.DBhostname)
OSU_remote = DBServerIP('OSU_remote','128.146.38.1',['SampleDB','phase2pixel_test'])
dblist.append(OSU_remote.DBhostname)
Purdue_remote = DBServerIP('Purdue_remote','cmsfpixdb.physics.purdue.edu',['cmsfpix_phase2']) 
dblist.append(Purdue_remote.DBhostname)

# Set the IT_uTDC_firmware for test
FPGAConfigList =  {
	'fc7.board.1'			 :  'IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28',
	'fc7.board.2'			 :  'IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28'
}

ModuleType = {
	1 :	"SingleSCC",
	2 :	"TFPX Quad",
	3 :	"TEPX Quad",
	4 :	"TBPX Quad",
	5 :	"Yellow Module (Purdue)",
	6 : "CROC 1x2",
	7 : "TFPX CROC Quad",
	8 : "CROC SCC",
}

firmware_image = {
	"SingleSCC" : 
            {"v4-11":"SCC_ELE_RD53A_v4-5.bit",
			 "v4-13":"SCC_ELE_RD53A_v4-6.bit",

             "v4-14":"SCC_ELE_RD53A_v4-6.bit",
			 },
	"Yellow Module (Purdue)" : 
             {"v4-11":"QUAD_ELE_RD53A_v4-5.bit",
			  "v4-13":"QUAD_ELE_RD53A_v4-6.bit",
              "v4-14":"QUAD_ELE_RD53A_v4-6.bit",
              "v4-02":"QUAD_ELE_RD53A_v4-2.bit",
			  },
	"TFPX Quad" : 
             {"v4-11":"QUAD_ELE_RD53A_v4-5.bit",
			  "v4-13":"QUAD_ELE_RD53A_v4-6.bit",
              "v4-14":"QUAD_ELE_RD53A_v4-6.bit",
			  },
	"TEPX Quad" : 
             {"v4-11":"QUAD_ELE_RD53A_v4-5.bit",
			  "v4-10":"QUAD_ELE_RD53A_v4-5.bit",
			  "v4-13":"QUAD_ELE_RD53A_v4-6.bit",
              "v4-14":"QUAD_ELE_RD53A_v4-6.bit",
			  },

	"TBPX Quad" : 
             {"v4-11":"QUAD_ELE_RD53A_v4-5.bit",
			  "v4-13":"QUAD_ELE_RD53A_v4-6.bit",
			  "v4-14":"QUAD_ELE_RD53A_v4-6.bit",
			  },
	"CROC SCC"  :
			{"v4-11":"SCC_ELE_CROC_v4-5.bit",
			 "v4-13":"SCC_ELE_CROC_v4-6.bit",
			 "v4-14":"SCC_ELE_CROC_v4-6.bit",
			},  
	"CROC 1x2"  :
			{"v4-11":"QUAD_ELE_CROC_v4-5.bit",
			 "v4-13":"QUAD_ELE_CROC_v4-6.bit",
			 "v4-14":"QUAD_ELE_CROC_v4-6.bit",
			},
	"TFPX CROC Quad"  :
			{"v4-11":"QUAD_ELE_CROC_v4-5.bit",
			 "v4-13":"QUAD_ELE_CROC_v4-6.bit",
			 "v4-14":"QUAD_ELE_CROC_v4-6.bit",
			},
}

ModuleLaneMap = {
	"TFPX Quad": {"0":"4","1":"2","2":"7","3":"5"},
	"TEPX Quad": {"0":"0","1":"1","2":"2","3":"3"},
	"TBPX Quad": {"0":"4","1":"5","2":"6","3":"7"},
	"SingleSCC": {"0":"0"},
	"Yellow Module (Purdue)" : {"0":"6","1":"5","3":"7"},
	"CROC SCC":  {"0":"15"},
	"CROC 1x2" : {"0":"12","2":"13"},
	"TFPX CROC Quad" : {"0":"12","1":"13","2":"14"},
}

BoxSize = {
	"SingleSCC" : 1,
	"TFPX Quad" : 4,
	"TEPX Quad" : 4,
	"TBPX Quad" : 4,
	"Yellow Module (Purdue)": 3,
	"CROC SCC"  : 1,
	"CROC 1x2"  : 2,
	"TFPX CROC Quad" : 3,
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
	'AllScan_Tuning'         :  'noise',
	'AllScan'                :  'noise',
	'QuickTest'              :  'noise',
	'IVCurve'                :  'ivcurve',
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
	'IVCurve'                :  'IVCurve',
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

SingleTest = ['IVCurve','Latency','PixelAlive','NoiseScan','SCurveScan','GainScan',
	'ThresholdEqualization','GainOptimization','ThresholdMinimization',
	'ThresholdAdjustment','InjectionDelay','ClockDelay','BitErrorRate','DataRBOptimization','ChipIntVoltageTuning','GenericDAC-DAC','Physics']

CompositeTest = ['AllScan_Tuning','AllScan','QuickTest']

CompositeList = {
	'AllScan': ['IVCurve','PixelAlive','NoiseScan','ThresholdAdjustment',
				'ThresholdEqualization','SCurveScan', 'NoiseScan','GainScan','GainOptimization',
				'InjectionDelay','SCurveScan'],
	'QuickTest': ['IVCurve','PixelAlive','NoiseScan']
}

pretuningList = ['IVCurve','PixelAlive','NoiseScan']
tuningList = ['ThresholdAdjustment','ThresholdEqualization','SCurveScan']
posttuningList = ['NoiseScan','SCurveScan']
#posttuningList = ['GainScan','GainOptimization','InjectionDelay','SCurveScan']

# Reserved for updated value for XML configuration
updatedXMLValues = defaultdict(dict)

updatedGlobalValue = defaultdict(lambda:None)
stepWiseGlobalValue = defaultdict(dict) #key : index

header = ['Source', 'Module_ID', 'User', 'Test', 'Time', 'Grade', 'DQMFile'] #Stop using
