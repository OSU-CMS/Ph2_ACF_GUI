'''
  gui.py
  brief                 Interface classes for pixel grading gui
  author                Brandon Manley
  version               0.1
  date                  06/08/20
  Support:              email to manley.329@osu.edu
'''

# import ROOT as r
import config
import database
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

def ConfigureTest(Calibration, Module_ID, Output_Dir):
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
	#Register the module to DB
	#Get the appropiate XML config file and RD53 file from either DB or local, copy to output file
	#Store the XML config and RD53 config to created folder and DB
	return Output_Dir

##########################################################################
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






