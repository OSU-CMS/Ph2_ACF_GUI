# November 19 2021:  Edited by Matt Joyce.  Added information for Purdue database to DBNames and DBServerIP

import os
from collections import defaultdict
from Gui.siteSettings import *
import InnerTrackerTests.TestSequences as TestSequences

# List of expert users
ExpertUserList = [
    "mjoyce",
    "cmsfpix_phase2_user",
    "localexpert",
]

FirmwareList = FC7List

"""
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

"""
"""
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
"""

# Note: First element of list will be shown as default value
# Note: The varibale name is same as hostname
dblist = []


class DBServerIP:
    All = "All"
    All_list = ["phase2pixel_test", "DBName2", "DBName3"]

    def __init__(self, DBhostname, DBIP, DBName):
        self.DBhostname = DBhostname
        self.DBIP = DBIP
        self.DBName = DBName


Central_remote = DBServerIP(
    "Central_remote", "0.0.0.0", ["phase2pixel_test", "DBName2", "DBName3"]
)
dblist.append(Central_remote.DBhostname)
local = DBServerIP("local", "127.0.0.1", ["SampleDB", "phase2pixel_test"])
dblist.append(local.DBhostname)
OSU_remote = DBServerIP("OSU_remote", "128.146.38.1", ["SampleDB", "phase2pixel_test"])
dblist.append(OSU_remote.DBhostname)
Purdue_remote = DBServerIP(
    "Purdue_remote", "cmsfpixdb.physics.purdue.edu", ["cmsfpix_phase2"]
)
dblist.append(Purdue_remote.DBhostname)

# Set the IT_uTDC_firmware for test
FPGAConfigList = {
    "fc7.board.1": "IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28",
    "fc7.board.2": "IT-uDTC_L12-KSU-3xQUAD_L8-KSU2xQUAD_x1G28",
}

ModuleType = {
    1: "TFPX SCC",
    2: "TFPX Quad",
    3: "TEPX Quad",
    4: "TBPX Quad",
    5: "TFPX CROC 1x2",
    6: "TFPX CROC Quad",
    7: "TFPX CROC SCC",
}

firmware_image = {
    "TFPX SCC": {
        "v4-21": "SCC_ELE_RD53A_v4-8.bit",
        "v4-13": "SCC_ELE_RD53A_v4-6.bit",
        "v4-14": "SCC_ELE_RD53A_v4-6.bit",
    },
    "TFPX Quad": {
        "v4-21": "QUAD_ELE_RD53A_v4-8.bit",
        "v4-13": "QUAD_ELE_RD53A_v4-6.bit",
        "v4-14": "QUAD_ELE_RD53A_v4-6.bit",
    },
    "TEPX Quad": {
        "v4-21": "QUAD_ELE_RD53A_v4-8.bit",
        "v4-13": "QUAD_ELE_RD53A_v4-6.bit",
        "v4-14": "QUAD_ELE_RD53A_v4-6.bit",
    },
    "TBPX Quad": {
        "v4-21": "QUAD_ELE_RD53A_v4-8.bit",
        "v4-13": "QUAD_ELE_RD53A_v4-6.bit",
        "v4-14": "QUAD_ELE_RD53A_v4-6.bit",
    },
    "TFPX CROC SCC": {
        "v4-21": "SCC_ELE_CROC_v4-8.bit",
        "v4-13": "SCC_ELE_CROC_v4-6.bit",
        "v4-14": "SCC_ELE_CROC_v4-6.bit",
    },
    "TFPX CROC 1x2": {
        "Dev": "QUAD_ELE_CROC_v4-8.bit",
        "v4-13": "QUAD_ELE_CROC_v4-6.bit",
        "v4-14": "QUAD_ELE_CROC_v4-6.bit",
    },
    "TFPX CROC Quad": {
        "v4-21": "QUAD_ELE_CROC_v4-8.bit",
        "v4-13": "QUAD_ELE_CROC_v4-6.bit",
        "v4-14": "QUAD_ELE_CROC_v4-6.bit",
    },
}

ModuleLaneMap = {
    "TFPX Quad": {"0": "4", "1": "2", "2": "7", "3": "5"},
    "TEPX Quad": {"0": "0", "1": "1", "2": "2", "3": "3"},
    "TBPX Quad": {"0": "4", "1": "5", "2": "6", "3": "7"},
    "TFPX SCC": {"0": "0"},
    "TFPX CROC SCC": {"0": "15"},
    "TFPX CROC 1x2": {"0": "12", "2": "13"},
    "TFPX CROC Quad": {"0": "12", "1": "13", "2": "14"},
}

ChipMap = {
    "TFPX CROC 1x2": {
        'VDDD_B': 	'13',
        'VDDA_B': 	'13',
        'VDDA_A': 	'12',
        'VDDD_A': 	'12',
        'VMUX_B': 	'13',
        'IMUX_B': 	'13',
        'VMUX_A': 	'12',
        'IMUX_A': 	'12',
        'GND_A':	'12'},
    }

BoxSize = {
    "TFPX SCC": 1,
    "TFPX Quad": 4,
    "TEPX Quad": 4,
    "TBPX Quad": 4,
    "TFPX CROC SCC": 1,
    "TFPX CROC 1x2": 2,
    "TFPX CROC Quad": 3,
}

# Reserved for updated value for XML configuration
updatedXMLValues = defaultdict(dict)

updatedGlobalValue = defaultdict(lambda: None)
stepWiseGlobalValue = defaultdict(dict)  # key : index
