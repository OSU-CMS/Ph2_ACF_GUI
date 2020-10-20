'''
  CreateWindow.py
  brief                 Run window
  author                Kai Wei
  version               0.1
  date                  09/24/20
  Support:              email to wei.856@osu.edu
'''

import sys
import tkinter as tk
import os
import re
import operator
import math
import hashlib
import subprocess
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

from Gui.GUIutils.settings import *
from Gui.GUIutils.ANSIColorText import *
from Gui.GUIutils.AbortWindow import *
from Gui.GUIutils.ReviewModuleWindow import *
from Gui.GUIutils.ErrorWindow import *
from Gui.GUIutils.guiUtils import *
from Gui.GUIutils.CustomizedConfigWindow import *


class RunWindow(tk.Toplevel):
	def __init__(self, parent, dbconnection):
		tk.Toplevel.__init__(self, parent.root)

		# setup root window
		self.parent = parent
		self.master = self.parent.root
		self.dbconnection = dbconnection
		self.title("Grading Run")
		self.geometry("1200x800")
		self.rowconfigure(0, weight=1, minsize=scaleInvHeight(self.master, 0.7))
		self.columnconfigure(0, weight=1, minsize=scaleInvHeight(self.master, 0.3))
		self.columnconfigure(1, weight=1, minsize=scaleInvHeight(self.master, 0.3))
		self.calibration = self.parent.current_test_name.get()
		self.input_dir = ''
		self.output_dir = ''
		self.config_file = '' #os.environ.get('GUI_dir')+ConfigFiles.get(self.calibration, "None")
		self.rd53_file  = ''

		# setup left side
		left_width = scaleInvWidth(self.master, 0.3)
		left_frame = tk.Frame(self, width=left_width, height=scaleInvHeight(self.master, 0.8))
		left_frame.grid(row=0, column=0, sticky='nsew')
		left_frame.columnconfigure(0, weight=1, minsize=left_width)
		left_frame.rowconfigure(0, weight=1, minsize=scaleInvHeight(self.master, 0.1))
		left_frame.rowconfigure(1, weight=1, minsize=scaleInvHeight(self.master, 0.3))
		left_frame.grid_propagate(False)

		info_frame_height = scaleInvHeight(self.master, 0.2)
		info_frame = tk.Frame(left_frame, width=left_width, height=info_frame_height)
		info_frame.grid(row=0, column=0, sticky='nsew')
		info_frame.columnconfigure(0, weight=1, minsize=int(left_width/5))
		info_frame.columnconfigure(1, weight=1, minsize=int(left_width/5))
		info_frame.columnconfigure(2, weight=1, minsize=int(left_width/5))
		info_frame.rowconfigure(0, weight=1, minsize=int(info_frame_height/8))
		info_frame.rowconfigure(1, weight=1, minsize=int(info_frame_height/8))
		info_frame.rowconfigure(2, weight=1, minsize=int(info_frame_height/8))
		info_frame.rowconfigure(3, weight=1, minsize=int(info_frame_height/8))
		info_frame.rowconfigure(4, weight=1, minsize=int(info_frame_height/8))
		info_frame.grid_propagate(False)
		
		console_frame = tk.Frame(left_frame, width=left_width, height=scaleInvHeight(self.master, 0.6), relief=tk.GROOVE, bd=2, bg='black')
		console_frame.grid(row=1, column=0, sticky='nsew')

		test_label = tk.Label(master=info_frame, text=self.parent.current_test_name.get(), font=("Helvetica", 25, "bold"))
		test_label.pack(anchor='nw', side=tk.TOP)

		module_label = tk.Label(master=info_frame, text="Module ID: {0}".format(self.parent.current_module_id), font=("Helvetica", 25))
		module_label.pack(anchor='nw', side=tk.TOP)

		config_button = ttk.Button(
			master = info_frame,
			text = "Configure",
			command = self.config_test
		)
		config_button.grid(row=2, column=0, columnspan=1, sticky='we')

		customizedconfig_button = ttk.Button(
			master = info_frame,
			text  = "Customize...",
			command = self.customizedconfig_test
		)
		customizedconfig_button.grid(row=2, column=1, columnspan=2, sticky='we')

		start_button = ttk.Button(
			master = info_frame,
			text = "Start",
			command = self.run_test
		)
		start_button.grid(row=3, column=0, columnspan=1, sticky='we')

		abort_button = ttk.Button(
			master = info_frame,
			text = "Abort",
			command = self.abort_test
		)
		abort_button.grid(row=3, column=1, columnspan=1, sticky='we')

		save_button = ttk.Button(
			master = info_frame,
			text = "Save",
			command = self.save_test
		)
		save_button.grid(row=3, column=2, columnspan=1, sticky='we')



		module_label = tk.Label(master=info_frame, text="Console output", font=("Helvetica", 20))
		module_label.pack(anchor='sw', side=tk.BOTTOM)

		scrollbar = tk.Scrollbar(console_frame)
		scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

		xscrollbar = tk.Scrollbar(console_frame, orient=tk.HORIZONTAL)
		xscrollbar.pack(side=tk.BOTTOM, fill=tk.X)

		self.console_text = AnsiColorText(console_frame, yscrollcommand=scrollbar.set, xscrollcommand=xscrollbar.set, bg='black', wrap='word')
		self.console_text.config(yscrollcommand=scrollbar.set)
		self.console_text.config(xscrollcommand=xscrollbar.set)
		scrollbar.config(command=self.console_text.yview)
		xscrollbar.config(command=self.console_text.xview)

		self.console_text.pack(expand=True, fill='both')

		# setup right side
		right_width = 700
		right_frame = tk.Frame(self, width=right_width, height=800)
		right_frame.grid(row=0, column=1, sticky='nsew')
		right_frame.columnconfigure(0, weight=1, minsize=right_width)
		right_frame.rowconfigure(0, weight=1, minsize=100)
		right_frame.rowconfigure(1, weight=1, minsize=550)
		right_frame.rowconfigure(2, weight=1, minsize=150)
		right_frame.grid_propagate(False)

		grade_frame = tk.Frame(right_frame, width=right_width, height=150)
		grade_frame.grid(row=0, column=0, sticky='new')
		grade_frame.grid_propagate(False)

		plot_frame = tk.Frame(right_frame, width=right_width, height=550, relief=tk.SUNKEN, bd=1)
		plot_frame.grid(row=1, column=0, sticky='news')
		plot_frame.grid_propagate(False)

		footer_frame = tk.Frame(right_frame, width=right_width, height=150)
		footer_frame.grid(row=2, column=0, sticky='nsew')
		footer_frame.grid_propagate(False)

		user_label = tk.Label(master=grade_frame, text="{0}".format(self.parent.current_user), font=("Helvetica", 20))
		user_label.pack(side=tk.RIGHT)

		self.emu_label = tk.Label(master=grade_frame, text="", font=("Helvetica", 20), fg='red')
		self.emu_label.pack(side=tk.RIGHT)

		grade_grade = tk.Label(master=grade_frame, text="{0}%".format(self.parent.current_test_grade), font=("Helvetica", 45, "bold"))
		grade_grade.pack(anchor='sw', side=tk.BOTTOM)

		if self.parent.current_test_grade < 0: grade_grade['text'] = '-%'

		grade_label = tk.Label(master=grade_frame, text="Grade", font=("Helvetica", 25))
		grade_label.pack(anchor='sw', side=tk.BOTTOM)

		self.plot_canvas = tk.Canvas(plot_frame)
		self.plot_canvas.pack(expand=True, fill='both', anchor='ne', side=tk.TOP)

		self.plot_label = tk.Label(master=self.plot_canvas, text='Plot will appear here', font=("Helvetica", 20))
		self.plot_label.pack(side=tk.TOP, anchor='nw')
		
		review_button = ttk.Button(
			master = footer_frame,
			text = "Review Module {0}".format(self.parent.current_module_id),
			command = self.go_to_review_module
		)
		review_button.pack()


	def go_to_review_module(self):
		self.parent.review_module_id = self.parent.current_module_id
		ReviewModuleWindow(self.parent, self.dbconnection)
		return


	def abort_test(self):
		AbortWindow(self,self.parent)
		return
			
	def config_test(self):
		#TCPConfigure(self.calibration, self.config_entry.get())
		self.input_dir = ""
		self.output_dir, self.input_dir = ConfigureTest(self.calibration, self.parent.current_module_id, self.output_dir, self.dbconnection)
		if self.input_dir == "":
			if self.config_file == "":
				config_file = os.environ.get('GUI_dir')+ConfigFiles.get(self.calibration, "None")
				SetupXMLConfigfromFile(config_file,self.output_dir)
				ErrorWindow(self.parent,"Noitce: Using default XML configuration")
			else:
				SetupXMLConfigfromFile(self.config_file,self.output_dir)
		else:
			if self.config_file != "":
				SetupXMLConfigfromFile(self.config_file,self.output_dir)
			else:
				SetupXMLConfig(self.input_dir,self.output_dir)

		if self.input_dir == "":
			if self.rd53_file == "":
				rd53_file = os.environ.get('Ph2_ACF_AREA')+"/settings/RD53Files/CMSIT_RD53.txt"
				SetupRD53ConfigfromFile(rd53_file,self.output_dir)
			else:
				SetupRD53ConfigfromFile(self.rd53_file,self.output_dir)
		else:
			if self.rd53_file == "":
				SetupRD53Config(self.input_dir,self.output_dir)
			else:
				SetupRD53ConfigfromFile(self.rd53_file,self.output_dir)
		return

	def customizedconfig_test(self):
		CustomizedConfigWindow(self,self.parent,self.dbconnection)
		return

	def run_test(self):
		self.run_process = Popen('python CMSITminiDAQ.py -f {0} -c {1}'.format("CMSIT.xml",self.calibration), shell=True, stdout=PIPE, stderr=PIPE)
		q = Queue(maxsize=1024)
		t = Thread(target=self.reader_thread, args=[q])
		t.daemon = True
		t.start()
		self.update(q)

		
		#FIXME: automate plot retrieval 
		# retrieveResultPlot('', 'Run000000_PixelAlive.root', 'Detector/Board_0/Module_0/Chip_0/D_B(0)_M(0)_PixelAlive_Chip(0)', 'testing1.png')

		re_img = Image.open("test_plots/pixelalive_ex.png")
		self.parent.current_test_plot = ImageTk.PhotoImage(re_img)
		self.plot_label.pack_forget()
		self.plot_canvas.create_image(0, 0, image=self.parent.current_test_plot, anchor="nw")


	def reader_thread(self, q):
		try:
			with self.run_process.stdout as pipe:
				for line in iter(pipe.readline, b''):
					q.put(line)
			with self.run_process.stderr as pipe:
				for line in iter(pipe.readline, b''):
					q.put(line)
		finally:
			q.put(None)


	def update(self, q):
		for line in iter_except(q.get_nowait, Empty):
			if line is None:
				return 
			else:
				line_text = line.decode('utf-8')
				if "under Emulation Mode" in line_text: self.emu_label['text'] = "Emulation mode"
				self.console_text.write(line.decode('utf-8'))

		self.parent.root.after(40, self.update, q)

	def save_test(self):
		#if self.parent.current_test_grade < 0:
		if self.run_process.poll() is None:
			ErrorWindow(self.parent,"Error: Test not finished or broken")
			return 
		try:
			os.system("cp {0}/test/Results/Run*.root {1}/".format(os.environ.get("Ph2_ACF_AREA"),self.output_dir))
			getfile = subprocess.run('ls -alt {0}/test/Results/Run*.root'.format(self.output_dir), shell=True, stdout=subprocess.PIPE)
			filelist = getfile.stdout.decode('utf-8')
			outputfile = filelist.rstrip('\n').split('\n')[-1].split(' ')[-1]
			DQMFile = self.output_dir + "/" + outputfile
			time_stamp = datetime.fromisoformat(self.output_dir.split('_')[-2])

			testing_info = [self.parent.current_module_id, self.parent.current_user, self.calibration, time_stamp.strftime('%Y-%m-%d %H:%M:%S.%f'), self.parent.new_test_grade, DQMFile]
			insertTestResult(self.dbconnection, testing_info)
			# fixme 
			#database.createTestEntry(testing_info)
		except:
			ErrorWindow(self.parent, "Error: Could not save")
			return
	
	def quit(self):
		self.run_process.kill()
		self.destroy()