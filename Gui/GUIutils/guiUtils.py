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
from datetime import datetime
from subprocess import Popen, PIPE
from itertools import islice
from textwrap import dedent
from PIL import ImageTk, Image
from functools import partial
from tkinter import scrolledtext 

from  Gui.GUIutils.DBConnection import *

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

def ConfigureTest(Calibration, Module_ID, Output_Dir, DBConnection):
	if not Output_Dir:
		test_dir = os.environ.get('DATA_dir') + "/Test_" +str(Calibration)
		if not os.path.isdir(test_dir):
			try:
				os.makedirs(test_dir)
			except OSError:
				print("Can not create directory: {0}".format(test_dir))
		time_stamp = datetime.utcnow().isoformat() + "_UTC"
		Output_Dir = test_dir + "/Test_Module" + str(Module_ID) + "_" + str(Calibration) + "_" + str(time_stamp)
		try:
			os.makedirs(Output_Dir)
		except OSError:
			return "OutputDir not created"
	
	#FIXME:
	#Get the appropiate XML config file and RD53 file from either DB or local, copy to output file
	#Store the XML config and RD53 config to created folder and DB
	header = retriveTestTableHeader(DBConnection)
	col_names = list(map(lambda x: header[x][0], range(0,len(header))))
	index = col_names.index('DQMFile')
	latest_record  = retrieveModuleLastTest(DBConnection, Module_ID)
	if latest_record and index:
		Input_Dir = "/".join(str(latest_record[0][index]).split('/')[0:-1])
	else:
		Input_Dir = ""
	return Output_Dir, Input_Dir

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

def SetupXMLConfigfromFile(InputFile, Output_Dir):
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

def SetupRD53Config(Input_Dir, Output_Dir):
	try:
		os.system("cp {0}/CMSIT_RD53_OUT.txt {1}/CMSIT_RD53_IN.txt".format(Input_Dir,Output_Dir))
	except OSError:
		print("Can not copy the RD53 configuration files to {0}".format(Output_Dir))
	try:
		os.system("cp {0}/CMSIT_RD53_IN.txt  {1}/test/CMSIT_RD53.txt".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))
	except OSError:
		print("Can not copy {0}/CMSIT_RD53_IN.txt to {1}/test/CMSIT_RD53.txt".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))

##########################################################################
##########################################################################

def SetupRD53ConfigfromFile(InputFile, Output_Dir):
	try:
		os.system("cp {0} {1}/CMSIT_RD53_IN.txt".format(InputFile,Output_Dir))
	except OSError:
		print("Can not copy the XML files {0} to {1}".format(InputFile,Output_Dir))
	try:
		os.system("cp {0}/CMSIT_RD53_IN.txt  {1}/test/CMSIT_RD53.txt".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))
	except OSError:
		print("Can not copy {0}/CMSIT_RD53_IN.txt to {1}/test/CMSIT_RD53.txt".format(Output_Dir,os.environ.get("Ph2_ACF_AREA")))

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



