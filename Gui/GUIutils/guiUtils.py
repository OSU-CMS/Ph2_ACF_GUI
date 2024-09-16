"""
  gui.py
  brief                 Interface classes for pixel grading gui
  author                Kai Wei
  version               1.0
  date                  04/27/21
  Support:              email to wei.856@osu.edu
"""

import sys
import os
import re
import operator
import math
import hashlib
from queue import Queue, Empty
from threading import Thread
from datetime import datetime, timedelta
from subprocess import Popen, PIPE
from itertools import islice
from textwrap import dedent
from functools import partial

from Gui.GUIutils.settings import (
    updatedGlobalValue,
    updatedXMLValues,
)
#from Gui.GUIutils.DBConnection import *
from Configuration.XMLUtil import (
    HWDescription,
    BeBoardModule,
    OGModule,
    HyBridModule,
    FE,
    MonitoringModule,
    LoadXML,
    GenerateHWDescriptionXML
)
from InnerTrackerTests.GlobalSettings import (
    globalSettings_DictA,
    globalSettings_DictB,
)
from InnerTrackerTests.FESettings import (
    FESettings_DictA,
    FESettings_DictB,
)
from InnerTrackerTests.HWSettings import (
    HWSettings_DictA,
    HWSettings_DictB,
)
from InnerTrackerTests.MonitoringSettings import (
    MonitoringListA,
    MonitoringListB,
)
from InnerTrackerTests.RegisterSettings import RegisterSettings
from InnerTrackerTests.FELaneConfig import FELaneConfig_DictB
from Gui.siteSettings import FC7List
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import CompositeTests, Test_to_Ph2ACF_Map
##########################################################################
##########################################################################


def scaleInvWidth(master, percent):
    scwidth = master.winfo_screenwidth()
    return int(scwidth * percent)


def scaleInvHeight(master, percent):
    scheight = master.winfo_screenheight()
    return int(scheight * percent)


##########################################################################
##########################################################################


def iter_except(function, exception):
    try:
        while True:
            yield function()
    except:
        return


##########################################################################
##########################################################################


def ConfigureTest(Test, Module_ID, Output_Dir, Input_Dir):
    if not Output_Dir:
        test_dir = os.environ.get("DATA_dir") + "/Test_" + str(Test)
        if not os.path.isdir(test_dir):
            try:
                os.makedirs(test_dir)
            except OSError:
                print("Can not create directory: {0}".format(test_dir))
        time = datetime.utcnow()
        timeRound = time - timedelta(microseconds=time.microsecond)
        time_stamp = timeRound.isoformat() + "_UTC"

        Output_Dir = (
            test_dir
            + "/Test_Module"
            + str(Module_ID)
            + "_"
            + str(Test)
            + "_"
            + str(time_stamp)
        )
        print(f"OUTPUT_DIR: {Output_Dir}")
        try:
            os.makedirs(Output_Dir)
        except OSError as e:
            print(f"OutputDir not created: {e}")
            return "", ""

        # FIXME:
        # Get the appropiate XML config file and RD53 file from either DB or local, copy to output file
        # Store the XML config and RD53 config to created folder and DB
        # if isActive(DBConnection):
        # 	header = retriveTestTableHeader(DBConnection)
        # 	col_names = list(map(lambda x: header[x][0], range(0,len(header))))
        # 	index = col_names.index('DQMFile')
        # 	latest_record  = retrieveModuleLastTest(DBConnection, Module_ID)
        # 	if latest_record and index:
        # 		Input_Dir = "/".join(str(latest_record[0][index]).split('/')[:-1])
        # 	else:
        # 		Input_Dir = ""
        return Output_Dir, Input_Dir

    else:
        return Output_Dir, Input_Dir


def isActive(dbconnection):
    try:
        if dbconnection == "Offline":
            return False
        elif dbconnection.is_connected():
            return True
        else:
            return False
    except Exception as err:
        print("Unexpected form, {}".format(repr(err)))
        return False


##########################################################################
##  Functions for setting up XML and RD53 configuration
##########################################################################


def SetupXMLConfig(Input_Dir, Output_Dir):
    try:
        os.system("cp {0}/CMSIT.xml {1}/CMSIT.xml".format(Input_Dir, Output_Dir))
    except OSError:
        print("Can not copy the XML files to {0}".format(Output_Dir))
    try:
        os.system(
            "cp {0}/CMSIT.xml  {1}/test/CMSIT.xml".format(
                Output_Dir, os.environ.get("PH2ACF_BASE_DIR")
            )
        )
    except OSError:
        print(
            "Can not copy {0}/CMSIT.xml to {1}/test/CMSIT.xml".format(
                Output_Dir, os.environ.get("PH2ACF_BASE_DIR")
            )
        )


##########################################################################
##########################################################################


def SetupXMLConfigfromFile(InputFile, Output_Dir, firmware, RD53Dict):
    changeMade = False
    try:
        root, tree = LoadXML(InputFile)
        # firmware is a list of QtBeBoards (FC7s) indexed by optical group.
        # Stores each board name and IP Address. Used to fill IP address
        # For all nodes that match .//connection
        # for Node in root.findall(".//connection"):
        #     opticalGroupID = Node.getparent().get('Id')
        #     fwIP = firmware[opticalGroupID].getIPAddress()
        #     print(f'opticalGroupID: {opticalGroupID}\nIP: {fwIP}')
        #     if fwIP not in Node.attrib["uri"]:
        #         print("Modified Node")
        #         Node.set(
        #             "uri", "chtcp-2.0://localhost:10203?target={}:50001".format(fwIP)
        #         )
        #         changeMade = True

        ###----Can probably remove this block that's commented------######
        # counter = 0

        """
		for Node in root.findall(nodeName[defaultACFVersion]):
			ParentNode = Node.getparent() #.getparent()
			try:
				ChipKey = "{}_{}_{}".format(ParentNode.attrib["Name"],ParentNode.attrib["Id"],Node.attrib["Id"])
			except:
				ChipKey = "{}_{}".format(ParentNode.attrib["Id"],Node.attrib["Id"])
			if ChipKey in RD53Dict.keys():
				Node.set('configfile','CMSIT_RD53_{}.txt'.format(ChipKey))
				changeMade = True
			else:
				try:
					keyList = list(RD53Dict.keys())
					ChipKey = keyList[counter]
					Node.set('configfile','CMSIT_RD53_{}.txt'.format(ChipKey))
					changeMade = True
					logger.info("Chip with key {} not listed in rd53 list, Please check the XML configuration".format(ChipKey))
				except Exception as err:
					pass
			counter += 1"""
    ###--------------------------------------------------------#####
    except Exception as error:
        print("Failed to set up the XML file, {}".format(error))

    try:
        # print('lenth of XML dict is {0}'.format(len(updatedXMLValues)))
        if len(updatedXMLValues) > 0:
            changeMade = True
            print(updatedXMLValues)

            for Node in root.findall(".//Settings"):
                # print("Found Settings Node!")
                if "VCAL_HIGH" in Node.attrib:
                    RD53Node = Node.getparent()
                    HyBridNode = RD53Node.getparent()
                    ## Potential Change: please check if it is [HyBrid ID/RD53 ID] or [HyBrid ID/RD53 Lane]
                    chipKeyName = "{0}/{1}".format(
                        HyBridNode.attrib["Id"], RD53Node.attrib["Id"]
                    )
                    print("chipKeyName is {0}".format(chipKeyName))
                    if len(updatedXMLValues[chipKeyName]) > 0:
                        for key in updatedXMLValues[chipKeyName].keys():
                            Node.set(key, str(updatedXMLValues[chipKeyName][key]))
                            

    except Exception as error:
        print("Failed to set up the XML file, {}".format(error))

    try:
        logger.info(updatedGlobalValue)
        if len(updatedGlobalValue) > 0:
            changeMade = True
            # print(updatedGlobalValue[1])
            for Node in root.findall(".//Setting"):
                if len(updatedGlobalValue[1]) > 0:
                    if Node.attrib["name"] == "TargetThr":
                        Node.text = updatedGlobalValue[1]["TargetThr"]
                        print(
                            "TargetThr value has been set to {0}".format(
                                updatedGlobalValue[1]["TargetThr"]
                            )
                        )
    except Exception as error:
        print(
            "Failed to update the TargetThr value, {0}".format(
                updatedGlobalValue[1]["TargetThr"]
            )
        )

    try:
        if changeMade:
            ModifiedFile = InputFile + ".changed"
            tree.write(ModifiedFile)
            InputFile = ModifiedFile

    except Exception as error:
        print("Failed to set up the XML file, {}".format(error))

    try:
        os.system("cp {0} {1}/CMSIT.xml".format(InputFile, Output_Dir))
    except OSError:
        print("Can not copy the XML files {0} to {1}".format(InputFile, Output_Dir))
    try:
        os.system(
            "cp {0}/CMSIT.xml  {1}/test/CMSIT.xml".format(
                Output_Dir, os.environ.get("PH2ACF_BASE_DIR")
            )
        )
    except OSError:
        print(
            "Can not copy {0}/CMSIT.xml to {1}/test/CMSIT.xml".format(
                Output_Dir, os.environ.get("PH2ACF_BASE_DIR")
            )
        )


##########################################################################
##########################################################################


def SetupRD53Config(Input_Dir, Output_Dir, RD53Dict):
    for key in RD53Dict.keys():
        try:
            os.system(
                "cp {0}/CMSIT_RD53_{1}_OUT.txt {2}/CMSIT_RD53_{1}_IN.txt".format(
                    Input_Dir, key, Output_Dir
                )
            )
        except OSError:
            print(
                "Can not copy the RD53 configuration files to {0} for RD53 ID: {1}".format(
                    Output_Dir, key
                )
            )
        try:
            os.system(
                "cp {0}/CMSIT_RD53_{1}_IN.txt  {2}/test/CMSIT_RD53_{1}.txt".format(
                    Output_Dir, key, os.environ.get("PH2ACF_BASE_DIR")
                )
            )
        except OSError:
            print(
                "Can not copy {0}/CMSIT_RD53_{1}_IN.txt to {2}/test/CMSIT_RD53_{1}.txt".format(
                    Output_Dir, key, os.environ.get("PH2ACF_BASE_DIR")
                )
            )


##########################################################################
##########################################################################


def SetupRD53ConfigfromFile(InputFileDict, Output_Dir):
    for key in InputFileDict.keys():
        try:
            os.system(
                "cp {0} {1}/CMSIT_RD53_{2}_IN.txt".format(
                    InputFileDict[key], Output_Dir, key
                )
            )
        except OSError:
            print(
                "Can not copy the XML files {0} to {1}".format(
                    InputFileDict[key], Output_Dir
                )
            )
        try:
            os.system(
                "cp {0}/CMSIT_RD53_{1}_IN.txt  {2}/test/CMSIT_RD53_{1}.txt".format(
                    Output_Dir, key, os.environ.get("PH2ACF_BASE_DIR")
                )
            )
        except OSError:
            print(
                "Can not copy {0}/CMSIT_RD53_{1}_IN.txt to {2}/test/CMSIT_RD53.txt".format(
                    Output_Dir, key, os.environ.get("PH2ACF_BASE_DIR")
                )
            )


##########################################################################
##########################################################################
def UpdateXMLValue(pFilename, pAttribute, pValue):
    root, tree = LoadXML(pFilename)
    for Node in root.findall(".//Setting"):
        if Node.attrib["name"] == pAttribute:
            Node.text = pValue
            #The next line should be done using logger, not print.
            #print("{0} has been set to {1}.".format(pAttribute, pValue))
            tree.write(pFilename)


def CheckXMLValue(pFilename, pAttribute):
    root, tree = LoadXML(pFilename)
    for Node in root.findall(".//Setting"):
        if Node.attrib["name"] == pAttribute:
            #The next line should be done using logger, not print.
            print("{0} is set to {1}.".format(pAttribute, Node.text))


##########################################################################
##########################################################################


def GenerateXMLConfig(firmwareList, testName, outputDir, **arg):
    outputFile = outputDir + "/CMSIT_" + testName + ".xml"
    
    boardtype = "RD53A"
    RegisterSettingsList = RegisterSettings #TODO: Investigate whether this actually matters (ie deep vs shallow copy)
    revPolarity = False #Flag to determine whether or not to reverse the Aurora lane polarity
    
    # Get Hardware discription and a list of the modules
    HWDescription0 = HWDescription()
    for BeBoard in firmwareList:
        BeBoardModule0 = BeBoardModule()

        #Set up Optical Groups
        for og in BeBoard.getAllOpticalGroups().values():
            OpticalGroupModule0 = OGModule()
            OpticalGroupModule0.SetOpticalGrp(og.getOpticalGroupID(), og.getFMCID())
            
            # Set up each module within the optical group
            for module in og.getAllModules().values():
                HyBridModule0 = HyBridModule()
                HyBridModule0.SetHyBridModule(module.getFMCPort(), "1")
                HyBridModule0.SetHyBridName(module.getModuleName())
        
                moduleType = module.getModuleType()
                
                RxPolarities = None
                if "CROC" in moduleType:
                    RxPolarities = "0"
                if "CROC" in moduleType and "Quad" in moduleType:
                    RxPolarities = "1" 
                
                revPolarity = not ("CROC" in moduleType and "1x2" in moduleType)
                FESettings_Dict = FESettings_DictB if "CROC" in moduleType else FESettings_DictA
                globalSettings_Dict = globalSettings_DictB if "CROC" in moduleType else globalSettings_DictA
                HWSettings_Dict = HWSettings_DictB if "CROC" in moduleType else HWSettings_DictA
                FELaneConfig_Dict = FELaneConfig_DictB[module.getModuleType().split(" ")[0]] if "CROC" in moduleType else None
                boardtype = "RD53B"+module.getModuleVersion() if "CROC" in moduleType else "RD53A"
                
                # Sets up all the chips on the module and adds them to the hybrid module to then be stored in the class
                for chip in module.getChips().values():
                    print("chip {0} status is {1}".format(chip.getID(), chip.getStatus()))
                    FEChip = FE()
                    FEChip.SetFE(
                        chip.getID(),
                        "1" if chip.getStatus() else "0",
                        chip.getLane(),
                        RxPolarities,
                        "CMSIT_RD53_{0}_{1}_{2}.txt".format(
                            module.getModuleName(), module.getFMCPort(), chip.getID()
                        ),
                    )
                    
                    FEChip.ConfigureFE(FESettings_Dict[testName])
                    FEChip.ConfigureLaneConfig(FELaneConfig_Dict[testName][int(chip.getLane())])
                    FEChip.VDDAtrim = chip.getVDDA()
                    FEChip.VDDDtrim = chip.getVDDD()
                    HyBridModule0.AddFE(FEChip)
                HyBridModule0.ConfigureGlobal(globalSettings_Dict[testName])
                OpticalGroupModule0.AddHyBrid(HyBridModule0)
            
            BeBoardModule0.AddOGModule(OpticalGroupModule0)
        
        if revPolarity:
            RegisterSettingsList['user.ctrl_regs.gtx_rx_polarity.fmc_l12'] = 11
        
        BeBoardModule0.SetURI(BeBoard.getIPAddress())
        BeBoardModule0.SetBeBoard(BeBoard.getBoardID(), "RD53")
        
        BeBoardModule0.SetRegisterValue(RegisterSettingsList)
        HWDescription0.AddBeBoard(BeBoardModule0)
    
    HWDescription0.AddSettings(HWSettings_Dict[testName])  
    MonitoringModule0 = MonitoringModule(boardtype)
    if "RD53A" in boardtype:
        MonitoringModule0.SetMonitoringList(MonitoringListA)
    else:
        MonitoringModule0.SetMonitoringList(MonitoringListB)
    HWDescription0.AddMonitoring(MonitoringModule0)
    GenerateHWDescriptionXML(HWDescription0, outputFile, boardtype)

    return outputFile


##########################################################################
##  Functions for setting up XML and RD53 configuration (END)
##########################################################################

# FIXME: automate this in runTest
# def retrieve_result_plot(result_dir, result_file, plot_file, output_file):
# 	os.system("root -l -b -q 'extractPlots.cpp(\"{0}\", \"{1}\", \"{2}\")'".format(result_dir+result_file, plot_file, output_file))
# 	result_image = tk.Image('photo', file=output_file) #FIXME: update to using pillow
# 	return result_image

##########################################################################
##########################################################################


class LogParser:
    def __init__(self):
        self.error_message = ""
        self.results_location = ""

    def getGrade(self, file):
        pass


##########################################################################
##  Functions for ROOT TBrowser
##########################################################################


def GetTBrowser(DQMFile):
    process = Popen(
        "{0}/Gui/GUIUtils/runBrowser.sh {1} {2}".format(
            os.environ.get("GUI_dir"),
            os.environ.get("GUI_dir") + "/Gui/GUIUtils",
            str(DQMFile),
        ),
        shell=True,
        stdout=PIPE,
        stderr=PIPE,
    )


##########################################################################
##  Functions for ROOT TBrowser (END)
##########################################################################


def isCompositeTest(TestName):
    if TestName in CompositeTests.keys():
        return True
    else:
        return False


def isSingleTest(TestName):
    if TestName in Test_to_Ph2ACF_Map.keys():
        return True
    else:
        return False


def formatter(DirName, columns, **kwargs):
    dirName = DirName.split("/")[-1]
    ReturnList = []
    ReturnDict = {}
    Module_ID = None
    recheckFlag = False
    for column in columns:
        if column == "id":
            ReturnList.append("")
        if column == "part_id":
            if "part_id" in kwargs.keys():
                Module_ID = kwargs["part_id"]
                ReturnList.append(Module_ID)
                ReturnDict.update({"part_id": Module_ID})
            else:
                Module = dirName.split("_")[1]
                if "Module" in Module:
                    Module_ID = Module.lstrip("Module")
                    ReturnList.append(Module_ID)
                    ReturnDict.update({"part_id": Module_ID})
                else:
                    Module_ID = "-1"
                    ReturnList.append(Module_ID)
                    ReturnDict.update({"part_id": Module_ID})
        if column == "user":
            ReturnList.append("local")
            ReturnDict.update({"user": "local"})
        if column == "test_id":
            pass
        if column == "test_name":
            ReturnList.append(dirName.split("_")[-3])
            ReturnDict.update({"test_name": dirName.split("_")[-3]})
        if column == "test_grade":
            if Module_ID != None:
                gradeFileName = "{}/Grade_Module{}.txt".format(DirName, Module_ID)
                if os.path.isfile(gradeFileName):
                    gradeFile = open(gradeFileName, "r")
                    content = gradeFile.readlines()
                    Grade = float(content[-1].split(" ")[-1])
                else:
                    Grade = -1
            else:
                Grade = -1
                recheckFlag = True
            ReturnList.append(Grade)
            ReturnDict.update({"test_grade": Grade})
        if column == "date":
            if str(sys.version).split(" ")[0].startswith(("3.7", "3.8", "3.9")):
                TimeStamp = datetime.fromisoformat(dirName.split("_")[-2])
            elif str(sys.version).split(" ")[0].startswith(("3.6")):
                TimeStamp = datetime.strptime(
                    dirName.split("_")[-2].split(".")[0], "%Y-%m-%dT%H:%M:%S"
                )
            ReturnList.append(TimeStamp)
            ReturnDict.update({"date": TimeStamp})
        if column == "test_id":
            data_id = ""
            ReturnList.append(data_id)
            ReturnDict.update({"test_id": data_id})

        # if column == "description":
        # 	ReturnList.append("")
        # if column == "type":
        # 	ReturnList.append("")

    if recheckFlag:
        if "part_id" in columns:
            try:
                indexModule = columns.index("part_id")
                indexGrade = columns.index("test_grade")
                Module_ID = ReturnList[indexModule]
                gradeFileName = "{}/Grade_Module{}.txt".format(DirName, Module_ID)
                if os.path.isfile(gradeFileName):
                    gradeFile = open(gradeFileName, "r")
                    content = gradeFile.readlines()
                    Grade = float(content[-1].split(" ")[-1])
                    ReturnList[indexGrade] = Grade
                else:
                    ReturnList[indexGrade] = -1
            except Exception as err:
                print("recheck failed")
        else:
            pass

    ReturnList.append(DirName)
    # ReturnDict.update({"localFile":DirName})
    return ReturnList
    # return ReturnDict
