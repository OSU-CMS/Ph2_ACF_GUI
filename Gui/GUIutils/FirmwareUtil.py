
import logging

# Customize the logging configuration
logging.basicConfig(
   level=logging.INFO,
   format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
   filename='my_project.log',  # Specify a log file
   filemode='w'  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

"""
  FirmwareUtil.py
  brief                 utility functions for firmware 
  author                Kai Wei
  version               0.1
  date                  03/11/20
  Support:              email to wei.856@osu.edu
"""
import subprocess
from subprocess import Popen, PIPE
from datetime import datetime


def firmwarePingCheck(firmware, fileName):
    fAddress = firmware.getIPAddress()
    outputFile = open(fileName, "a+")
    subprocess.run(
        ["echo", "\n[{0}]\n".format(datetime.now().strftime("%d/%m/%Y %H:%M:%S"))],
        stdout=outputFile,
        stderr=outputFile,
    )
    returnCode = subprocess.run(
        ["ping", "-c", "1", "-W", "1", fAddress], stdout=outputFile, stderr=outputFile
    ).returncode
    return returnCode


# Fixme, firmware check to be decided and implemented
def fpgaConfigCheck(firmware, fileName, **kwargs):
    returnCode = 0
    return returnCode


def fwStatusParser(firmware, fileName, **kwargs):
    verboseInfo = {
        "Ping test": " ",
        "FPGA configuration": " ",
        "Fw test3": " ",
        "Fw test4": " ",
        "Fw test5": " ",
    }
    pingReturnCode = firmwarePingCheck(firmware, fileName)
    if pingReturnCode == 2:
        verboseInfo["Ping test"] = "Failed"
        return "Ping failed", "color:red", verboseInfo
    elif pingReturnCode == 0:
        verboseInfo["Ping test"] = "Success"
    else:
        verboseInfo["Ping test"] = "Unknown"
        return "Ping failed", "color:red", verboseInfo

    fpgaReturnCode = fpgaConfigCheck(firmware, fileName)
    if fpgaReturnCode == 1:
        verboseInfo["FPGA configuration"] = "Failed"
        return "FPGA configuration failed", "color:red", verboseInfo
    elif fpgaReturnCode == 0:
        verboseInfo["FPGA configuration"] = "Success"
    else:
        verboseInfo["FPGA configuration"] = "Unknown"
        return "FPGA configuration failed", "color:red", verboseInfo

    ## Dummy test
    test3ReturnCode = fpgaConfigCheck(firmware, fileName)
    if fpgaReturnCode == 1:
        verboseInfo["Fw test3"] = "Failed"
        return "Fw test3 failed", "color:red", verboseInfo
    elif fpgaReturnCode == 0:
        verboseInfo["Fw test3"] = "Success"
    else:
        verboseInfo["Fw test3"] = "Unknown"
        return "Fw test3 failed", "color:red", verboseInfo

    test4ReturnCode = fpgaConfigCheck(firmware, fileName)
    if fpgaReturnCode == 1:
        verboseInfo["Fw test4"] = "Failed"
        return "Fw test4 failed", "color:red", verboseInfo
    elif fpgaReturnCode == 0:
        verboseInfo["Fw test4"] = "Success"
    else:
        verboseInfo["Fw test4"] = "Unknown"
        return "Fw test4 failed", "color:red", verboseInfo

    test5ReturnCode = fpgaConfigCheck(firmware, fileName)
    if fpgaReturnCode == 1:
        verboseInfo["Fw test5"] = "Failed"
        return "Fw test5 failed", "color:red", verboseInfo
    elif fpgaReturnCode == 0:
        verboseInfo["Fw test5"] = "Success"
    else:
        verboseInfo["Fw test5"] = "Unknown"
        return "Fw test5 failed", "color:red", verboseInfo
    ## Dummy test(END)

    return "Connected", "color: green", verboseInfo


FwStatusCheck = {
    "": """Please run frimware check  first """,
    "Ping failed": """Please check:
								 1. FC7 board is connected
								 2. FC7 is connected to PC via Ethernet cable
								 3. The assigned IP address is correct
								 4. rarpd service is running
									""",
    "FPGA configuration failed": """Please check:
								 	1. the microSD is inserted to the slot
								 	2. FPGA configuration file available
									""",
    "Fw test3 failed": """ Please check:
									1. First
									2. Second
							  """,
    "Fw test4 failed": """ Please check:
									1. First
									2. Second
							  """,
    "Fw test5 failed": """ Please check:
									1. First
									2. Second
							  """,
    "Connected": """Good""",
}

FEPowerUpVD = {
    "SLDO": [1.75, 1.82],
    "Direct": [1.15, 1.25],
}

FEPowerUpVA = {"SLDO": [1.75, 1.82], "Direct": [1.15, 1.25]}

FEPowerUpAmp = {"SLDO": [0.5, 1.3], "Direct": [0.5, 1.3]}
