'''
  gui.py
  brief                 Interface classes for pixel grading gui
  author                Brandon Manley
  version               0.1
  date                  06/08/20
  Support:              email to manley.329@osu.edu
'''

# import ROOT as r
#import config
#import database
import sys
import tkinter as tk
import os
import re
import operator
import math
import hashlib
from queue import Queue, Empty
from threading import Thread
from tkinter import ttk
from datetime import datetime, timedelta
from subprocess import Popen, PIPE
from itertools import islice
from textwrap import dedent
from PIL import ImageTk, Image
from functools import partial
from tkinter import scrolledtext 

from  Gui.GUIutils.settings import *
from  Gui.GUIutils.DBConnection import *
from  Configuration.XMLUtil import *

import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

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

def ConfigureTest(Test, Module_ID, Output_Dir, Input_Dir, DBConnection):
	if not Output_Dir:
		test_dir = os.environ.get('DATA_dir') + "/Test_" +str(Test)
		if not os.path.isdir(test_dir):
			try:
				os.makedirs(test_dir)
			except OSError:
				print("Can not create directory: {0}".format(test_dir))
		time = datetime.utcnow()
		timeRound = time - timedelta(microseconds=time.microsecond)
		time_stamp = timeRound.isoformat() + "_UTC"

		Output_Dir = test_dir + "/Test_Module" + str(Module_ID) + "_" + str(Test) + "_" + str(time_stamp)
		try:
			os.makedirs(Output_Dir)
		except OSError:
			print("OutputDir not created")
			return "",""
	
	#FIXME:
	#Get the appropiate XML config file and RD53 file from either DB or local, copy to output file
	#Store the XML config and RD53 config to created folder and DB
	#if isActive(DBConnection):
	#	header = retriveTestTableHeader(DBConnection)
	#	col_names = list(map(lambda x: header[x][0], range(0,len(header))))
	#	index = col_names.index('DQMFile')
	#	latest_record  = retrieveModuleLastTest(DBConnection, Module_ID)
	#	if latest_record and index:
	#		Input_Dir = "/".join(str(latest_record[0][index]).split('/')[:-1])
	#	else:
	#		Input_Dir = ""
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
		os.system("cp {0}/CMSIT.xml {1}/CMSIT.xml".format(Input_Dir,Output_Dir))
	except OSError:
		print("Can not copy the XML files to {0}".format(Output_Dir))
	try:
		os.system("cp {0}/CMSIT.xml  {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))
	except OSError:
		print("Can not copy {0}/CMSIT.xml to {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))

##########################################################################
##########################################################################

def SetupXMLConfigfromFile(InputFile, Output_Dir, firmwareName, RD53Dict):
	changeMade = False
	try:
		root,tree = LoadXML(InputFile)
		fwIP = FirmwareList[firmwareName]
		for Node in root.findall(".//connection"):
			if fwIP not in Node.attrib['uri']:
				Node.set('uri','chtcp-2.0://localhost:10203?target={}:50001'.format(fwIP))
				changeMade = True


		counter = 0
		for Node in root.findall(".//RD53"):
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
			counter += 1
	except Exception as error:
		print("Failed to set up the XML file, {}".format(error))

	try:
		print('lenth of XML dict is {0}'.format(len(updatedXMLValues)))
		if len(updatedXMLValues) > 0:
			changeMade = True
			print(updatedXMLValues)

			for Node in root.findall(".//Settings"):
				print("Found Settings Node!")
				if 'Vthreshold_LIN' in Node.attrib:
					RD53Node = Node.getparent()
					HyBridNode = RD53Node.getparent()
					chipKeyName ="{0}/{1}".format(HyBridNode.attrib["Id"],RD53Node.attrib["Lane"])
					print('chipKeyName is {0}'.format(chipKeyName))
					if len(updatedXMLValues[chipKeyName]) > 0:
						for key in updatedXMLValues[chipKeyName].keys():
							Node.set(key,str(updatedXMLValues[chipKeyName][key]))
							print('Node {0} has been set to {1}'.format(key,updatedXMLValues[chipKeyName][key]))
				
	except Exception as error:
		print("Failed to set up the XML file, {}".format(error))


	try:
		if changeMade:
			ModifiedFile = InputFile+".changed"
			tree.write(ModifiedFile)
			InputFile = ModifiedFile

	except Exception as error:
		print("Failed to set up the XML file, {}".format(error))

	try:
		os.system("cp {0} {1}/CMSIT.xml".format(InputFile,Output_Dir))
	except OSError:
		print("Can not copy the XML files {0} to {1}".format(InputFile,Output_Dir))
	try:
		os.system("cp {0}/CMSIT.xml  {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))
	except OSError:
		print("Can not copy {0}/CMSIT.xml to {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))

##########################################################################
##########################################################################

def SetupRD53Config(Input_Dir, Output_Dir, RD53Dict):
	for key in RD53Dict.keys():
		try:
			os.system("cp {0}/CMSIT_RD53_{1}_OUT.txt {2}/CMSIT_RD53_{1}_IN.txt".format(Input_Dir,key,Output_Dir))
		except OSError:
			print("Can not copy the RD53 configuration files to {0} for RD53 ID: {1}".format(Output_Dir, key))
		try:
			os.system("cp {0}/CMSIT_RD53_{1}_IN.txt  {2}/test/CMSIT_RD53_{1}.txt".format(Output_Dir,key,os.environ.get("Ph2_ACF_AREA")))
		except OSError:
			print("Can not copy {0}/CMSIT_RD53_{1}_IN.txt to {2}/test/CMSIT_RD53_{1}.txt".format(Output_Dir,key,os.environ.get("Ph2_ACF_AREA")))

##########################################################################
##########################################################################

def SetupRD53ConfigfromFile(InputFileDict, Output_Dir):
	for key in InputFileDict.keys():
		try:
			os.system("cp {0} {1}/CMSIT_RD53_{2}_IN.txt".format(InputFileDict[key],Output_Dir,key))
		except OSError:
			print("Can not copy the XML files {0} to {1}".format(InputFileDict[key],Output_Dir))
		try:
			os.system("cp {0}/CMSIT_RD53_{1}_IN.txt  {2}/test/CMSIT_RD53_{1}.txt".format(Output_Dir,key,os.environ.get("Ph2_ACF_AREA")))
		except OSError:
			print("Can not copy {0}/CMSIT_RD53_{1}_IN.txt to {2}/test/CMSIT_RD53.txt".format(Output_Dir,key,os.environ.get("Ph2_ACF_AREA")))


def GenerateXMLConfig(firmwareList, testName, outputDir, **arg):
	#try:
		outputFile = outputDir + "/CMSIT_" + testName +".xml" 

		HWDescription0 = HWDescription()
		BeBoardModule0 = BeBoardModule()
		AllModules = firmwareList.getAllModules().values()
		for module in AllModules:
			AllOG = {}
			OpticalGroupModule = OGModule()
			OpticalGroupModule.SetOpticalGrp(module.getOpticalGroupID(),module.getFMCID())
			AllOG[module.getOpticalGroupID()] = OpticalGroupModule

		for module in firmwareList.getAllModules().values():
			OpticalGroupModule0 = AllOG[module.getOpticalGroupID()]
			HyBridModule0 = HyBridModule()
			HyBridModule0.SetHyBridModule(module.getModuleID(),"1")
			HyBridModule0.SetHyBridName(module.getModuleName())
			for chip in module.getChips().values():
				FEChip = FE()
				FEChip.SetFE(chip.getID(),chip.getLane(),"CMSIT_RD53_{0}_{1}_{2}.txt".format(module.getModuleName(),module.getModuleID(),chip.getLane()))
				FEChip.ConfigureFE(FESettings_Dict[testName])
				HyBridModule0.AddFE(FEChip)
			HyBridModule0.ConfigureGlobal(globalSettings_Dict[testName])
			OpticalGroupModule0.AddHyBrid(HyBridModule0)

		for OpticalGroupModule in AllOG.values():
			BeBoardModule0.AddOGModule(OpticalGroupModule)

		BeBoardModule0.SetRegisterValue(RegisterSettings)
		HWDescription0.AddBeBoard(BeBoardModule0)
		HWDescription0.AddSettings(HWSettings_Dict[testName])
		MonitoringModule0 = MonitoringModule()
		MonitoringModule0.SetMonitoringList(MonitoringList)
		HWDescription0.AddMonitoring(MonitoringModule0)
		GenerateHWDescriptionXML(HWDescription0,outputFile)
	#except:
	#	logger.warning("Unexpcted issue generating {}. Please check the file".format(outputFile))
	#	outputFile = None

		return outputFile

##########################################################################
##  Functions for setting up XML and RD53 configuration (END)
##########################################################################

#FIXME: automate this in runTest
def retrieve_result_plot(result_dir, result_file, plot_file, output_file):
	os.system("root -l -b -q 'extractPlots.cpp(\"{0}\", \"{1}\", \"{2}\")'".format(result_dir+result_file, plot_file, output_file))
	result_image = tk.Image('photo', file=output_file) #FIXME: update to using pillow
	return result_image

##########################################################################
##########################################################################

class LogParser():
	def __init__(self):
		self.error_message = ''
		self.results_location = ''

	def getGrade(self, file):
		pass


##########################################################################
##  Functions for ROOT TBrowser 
##########################################################################

def GetTBrowser(DQMFile):
	process = Popen("{0}/Gui/GUIUtils/runBrowser.sh {1} {2}".format(os.environ.get("GUI_dir"),os.environ.get("GUI_dir")+ "/Gui/GUIUtils" , str(DQMFile)), shell=True,stdout=PIPE, stderr=PIPE)



##########################################################################
##  Functions for ROOT TBrowser (END)
##########################################################################


def isCompositeTest(TestName):
	if TestName in CompositeTest:
		return True
	else:
		return False

def isSingleTest(TestName):
	if TestName in SingleTest:
		return True
	else:
		return False

def formatter(DirName, columns, **kwargs):
	dirName = DirName.split('/')[-1]
	ReturnList  = []
	ReturnDict = {}
	Module_ID = None
	recheckFlag = False
	for column in columns:
		if column  == "id":
			ReturnList.append("")
		if column  == "part_id":
			if 'part_id' in kwargs.keys():
				Module_ID = kwargs['part_id']
				ReturnList.append(Module_ID)
				ReturnDict.update({"part_id": Module_ID})	
			else:
				Module = dirName.split('_')[1]
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
			ReturnDict.update({"user":"local"})
		if column == "test_name":
			ReturnList.append(dirName.split('_')[-3])
			ReturnDict.update({"test_name":dirName.split('_')[-3]})
		if column == "test_grade":
			if Module_ID:
				gradeFileName =  "{}/Grade_Module{}.txt".format(DirName,Module_ID)
				if os.path.isfile(gradeFileName):
					gradeFile = open(gradeFileName,"r")
					content = gradeFile.readlines()
					Grade = float(content[-1].split(' ')[-1])
				else:
					Grade  = -1
			else:
				Grade = -1
				recheckFlag = True
			ReturnList.append(Grade)
			ReturnDict.update({"test_grade":Grade})
		if column == "date":
			if str(sys.version).split(" ")[0].startswith(("3.7","3.8","3.9")):
				TimeStamp = datetime.fromisoformat(dirName.split('_')[-2])
			elif str(sys.version).split(" ")[0].startswith(("3.6")):
				TimeStamp = datetime.strptime(dirName.split('_')[-2].split('.')[0], '%Y-%m-%dT%H:%M:%S')
			ReturnList.append(TimeStamp)
			ReturnDict.update({"date":TimeStamp})
		if column == "test_id":
			data_id = ""
			ReturnList.append(data_id)
			ReturnDict.update({"test_id":data_id})

		#if column == "description":
		#	ReturnList.append("")
		#if column == "type":
		#	ReturnList.append("")

	if recheckFlag:
		if "part_id" in columns:
			indexModule = columns.index("part_id")
			indexGrade = column.index("test_grade")
			Module_ID = ReturnList[indexModule]
			gradeFileName =  "{}/Grade_Module{}.txt".format(DirName,Module_ID)
			if os.path.isfile(gradeFileName):
				gradeFile = open(gradeFileName,"r")
				content = gradeFile.readlines()
				Grade = float(content[-1].split(' ')[-1])
				ReturnList[indexGrade] = Grade
			else:
				ReturnList[indexGrade] = -1
		else:
			pass

	ReturnList.append(DirName)
	#ReturnDict.update({"localFile":DirName})
	return ReturnList
	#return ReturnDict