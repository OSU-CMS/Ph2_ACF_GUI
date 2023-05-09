'''
  gui.py
  brief                 Interface classes for pixel grading gui
  author                Kai Wei
  version               1.0
  date                  04/27/21
  Support:              email to wei.856@osu.edu
'''

# import ROOT as r
#import config
#import database
import sys
#import tkinter as tk
import os
import re
import operator
import math
import hashlib
from queue import Queue, Empty
from threading import Thread
#from tkinter import ttk
from datetime import datetime, timedelta
from subprocess import Popen, PIPE
from itertools import islice
from textwrap import dedent
#from PIL import ImageTk, Image
from functools import partial
#from tkinter import scrolledtext 

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
		os.system("cp {0}/CMSIT.xml  {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("PH2ACF_BASE_DIR")))
	except OSError:
		print("Can not copy {0}/CMSIT.xml to {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("PH2ACF_BASE_DIR")))

##########################################################################
##########################################################################

def SetupXMLConfigfromFile(InputFile, Output_Dir, firmwareName, RD53Dict):
	changeMade = False
	try:
		root,tree = LoadXML(InputFile)
		# FirmwareList just stores the IP address of the FC7 board being used
		fwIP = FirmwareList[firmwareName]
		# For all nodes that match .//connection 
		for Node in root.findall(".//connection"):
			if fwIP not in Node.attrib['uri']:
				Node.set('uri','chtcp-2.0://localhost:10203?target={}:50001'.format(fwIP))
				changeMade = True

###----Can probably remove this block that's commented------######
		#counter = 0
		
		'''
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
			counter += 1'''
###--------------------------------------------------------#####			
	except Exception as error:
		print("Failed to set up the XML file, {}".format(error))

	try:
		#print('lenth of XML dict is {0}'.format(len(updatedXMLValues)))
		if len(updatedXMLValues) > 0:
			changeMade = True
			print(updatedXMLValues)

			for Node in root.findall(".//Settings"):
				#print("Found Settings Node!")
				if 'VCAL_HIGH' in Node.attrib:
					RD53Node = Node.getparent()
					HyBridNode = RD53Node.getparent()
					## Potential Change: please check if it is [HyBrid ID/RD53 ID] or [HyBrid ID/RD53 Lane]
					chipKeyName ="{0}/{1}".format(HyBridNode.attrib["Id"],RD53Node.attrib["Id"])
					print('chipKeyName is {0}'.format(chipKeyName))
					if len(updatedXMLValues[chipKeyName]) > 0:
						for key in updatedXMLValues[chipKeyName].keys():
							Node.set(key,str(updatedXMLValues[chipKeyName][key]))
							print('Node {0} has been set to {1}'.format(key,updatedXMLValues[chipKeyName][key]))
				
	except Exception as error:
		print("Failed to set up the XML file, {}".format(error))


	try:
		logger.info(updatedGlobalValue)
		if len(updatedGlobalValue) > 0:
			changeMade = True
			#print(updatedGlobalValue[1])
			for Node in root.findall('.//Setting'):
				if len(updatedGlobalValue[1])>0:
					if Node.attrib['name']=='TargetThr':
						Node.text = updatedGlobalValue[1]['TargetThr']
						print('TargetThr value has been set to {0}'.format(updatedGlobalValue[1]['TargetThr']))
	except Exception as error:
		print('Failed to update the TargetThr value, {0}'.format(updatedGlobalValue[1]['TargetThr']))

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
		os.system("cp {0}/CMSIT.xml  {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("PH2ACF_BASE_DIR")))
	except OSError:
		print("Can not copy {0}/CMSIT.xml to {1}/test/CMSIT.xml".format(Output_Dir,os.environ.get("PH2ACF_BASE_DIR")))

##########################################################################
##########################################################################

def SetupRD53Config(Input_Dir, Output_Dir, RD53Dict):
	for key in RD53Dict.keys():
		try:
			os.system("cp {0}/CMSIT_RD53_{1}_OUT.txt {2}/CMSIT_RD53_{1}_IN.txt".format(Input_Dir,key,Output_Dir))
		except OSError:
			print("Can not copy the RD53 configuration files to {0} for RD53 ID: {1}".format(Output_Dir, key))
		try:
			os.system("cp {0}/CMSIT_RD53_{1}_IN.txt  {2}/test/CMSIT_RD53_{1}.txt".format(Output_Dir,key,os.environ.get("PH2ACF_BASE_DIR")))
		except OSError:
			print("Can not copy {0}/CMSIT_RD53_{1}_IN.txt to {2}/test/CMSIT_RD53_{1}.txt".format(Output_Dir,key,os.environ.get("PH2ACF_BASE_DIR")))

##########################################################################
##########################################################################

def SetupRD53ConfigfromFile(InputFileDict, Output_Dir):
	for key in InputFileDict.keys():
		try:
			os.system("cp {0} {1}/CMSIT_RD53_{2}_IN.txt".format(InputFileDict[key],Output_Dir,key))
		except OSError:
			print("Can not copy the XML files {0} to {1}".format(InputFileDict[key],Output_Dir))
		try:
			os.system("cp {0}/CMSIT_RD53_{1}_IN.txt  {2}/test/CMSIT_RD53_{1}.txt".format(Output_Dir,key,os.environ.get("PH2ACF_BASE_DIR")))
		except OSError:
			print("Can not copy {0}/CMSIT_RD53_{1}_IN.txt to {2}/test/CMSIT_RD53.txt".format(Output_Dir,key,os.environ.get("PH2ACF_BASE_DIR")))
##########################################################################
##########################################################################
def UpdateXMLValue(pFilename, pAttribute, pValue):
	root,tree = LoadXML(pFilename)
	for Node in root.findall('.//Setting'):
		if Node.attrib['name']==pAttribute:
			Node.text = pValue
			print('{0} has been set to {1}.'.format(pAttribute,pValue))
			tree.write(pFilename)

def CheckXMLValue(pFilename, pAttribute):
	root,tree = LoadXML(pFilename)
	for Node in root.findall('.//Setting'):
		if Node.attrib['name']==pAttribute:
			print('{0} is set to {1}.'.format(pAttribute,Node.text))

##########################################################################
##########################################################################

def GenerateXMLConfig(firmwareList, testName, outputDir, **arg):
	#try:
		outputFile = outputDir + "/CMSIT_" + testName +".xml" 
		# Get Hardware discription and a list of the modules
		HWDescription0 = HWDescription()
		BeBoardModule0 = BeBoardModule()
		AllModules = firmwareList.getAllModules().values()
		boardtype = 'RD53A'
		# Setup all optical groups for the modules
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
			
			if 'CROC' in module.getModuleType(): 
				FESettings_Dict = FESettings_DictB
				globalSettings_Dict = globalSettings_DictB
				HWSettings_Dict = HWSettings_DictB
				boardtype = 'RD53B'
			else: 
				FESettings_Dict = FESettings_DictA
				globalSettings_Dict = globalSettings_DictA
				HWSettings_Dict = HWSettings_DictA
				boardtype = 'RD53A'
			

			# Sets up all the chips on the module and adds them to the hybrid module to then be stored in the class
			for chip in module.getChips().values():
				print('chip {0} status is {1}'.format(chip.getID(),chip.getStatus()))
				if not chip.getStatus():
					continue
				FEChip = FE()
				FEChip.SetFE(chip.getID(),chip.getLane(),"CMSIT_RD53_{0}_{1}_{2}.txt".format(module.getModuleName(),module.getModuleID(),chip.getID()))
				FEChip.ConfigureFE(FESettings_Dict[testName])
				FEChip.VDDAtrim = chip.getVDDA()
				FEChip.VDDDtrim = chip.getVDDD()
				HyBridModule0.AddFE(FEChip)
			HyBridModule0.ConfigureGlobal(globalSettings_Dict[testName])
			OpticalGroupModule0.AddHyBrid(HyBridModule0)

		for OpticalGroupModule in AllOG.values():
			BeBoardModule0.AddOGModule(OpticalGroupModule)

		BeBoardModule0.SetRegisterValue(RegisterSettings)
		HWDescription0.AddBeBoard(BeBoardModule0)
		HWDescription0.AddSettings(HWSettings_Dict[testName])
		MonitoringModule0 = MonitoringModule(boardtype)
		if 'RD53A' in boardtype:
			MonitoringModule0.SetMonitoringList(MonitoringListA)
		else:
			MonitoringModule0.SetMonitoringList(MonitoringListB)
		HWDescription0.AddMonitoring(MonitoringModule0)
		GenerateHWDescriptionXML(HWDescription0,outputFile,boardtype)
	#except:
	#	logger.warning("Unexpcted issue generating {}. Please check the file".format(outputFile))
	#	outputFile = None

		return outputFile




##########################################################################
##  Functions for setting up XML and RD53 configuration (END)
##########################################################################

#FIXME: automate this in runTest
#def retrieve_result_plot(result_dir, result_file, plot_file, output_file):
#	os.system("root -l -b -q 'extractPlots.cpp(\"{0}\", \"{1}\", \"{2}\")'".format(result_dir+result_file, plot_file, output_file))
#	result_image = tk.Image('photo', file=output_file) #FIXME: update to using pillow
#	return result_image

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
		if column == "test_id":
			pass
		if column == "test_name":
			ReturnList.append(dirName.split('_')[-3])
			ReturnDict.update({"test_name":dirName.split('_')[-3]})
		if column == "test_grade":
			if Module_ID != None:
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
			try:
				indexModule = columns.index("part_id")
				indexGrade = columns.index("test_grade")
				Module_ID = ReturnList[indexModule]
				gradeFileName =  "{}/Grade_Module{}.txt".format(DirName,Module_ID)
				if os.path.isfile(gradeFileName):
					gradeFile = open(gradeFileName,"r")
					content = gradeFile.readlines()
					Grade = float(content[-1].split(' ')[-1])
					ReturnList[indexGrade] = Grade
				else:
					ReturnList[indexGrade] = -1
			except Exception as err:
				print("recheck failed")
		else:
			pass

	ReturnList.append(DirName)
	#ReturnDict.update({"localFile":DirName})
	return ReturnList
	#return ReturnDict
