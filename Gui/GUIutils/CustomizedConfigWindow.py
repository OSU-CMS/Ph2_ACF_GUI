'''
  CreateWindow.py
  brief                 Run window
  author                Kai Wei
  version               0.1
  date                  10/12/20
  Support:              email to wei.856@osu.edu
'''

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

from Gui.GUIutils.settings import *
from Gui.GUIutils.ANSIColorText import *
from Gui.GUIutils.AbortWindow import *
from Gui.GUIutils.RunWindow import *
from Gui.GUIutils.ReviewModuleWindow import *
from Gui.GUIutils.ErrorWindow import *
from Gui.GUIutils.guiUtils import *

class CustomizedConfigWindow(tk.Toplevel):
	def __init__(self, origin, parent, dbconnection):
		tk.Toplevel.__init__(self, parent.root)
		# setup root window
		self.parent = parent
		self.origin = origin
		self.master = self.parent.root
		self.dbconnection = dbconnection
		self.title("Customizing Config")
		self.geometry("1000x500")
		self.columnconfigure(0, weight=1, minsize=400)
		self.rowconfigure(0, weight=1, minsize=20)

		config_frame = tk.Frame(self, width=800, height=400)
		config_frame.grid(row=0, column=0, sticky='nsew')
		config_frame.columnconfigure(0, weight=1, minsize=200)
		config_frame.columnconfigure(1, weight=1, minsize=200)
		config_frame.columnconfigure(2, weight=1, minsize=200)
		config_frame.columnconfigure(3, weight=1, minsize=200)
		config_frame.rowconfigure(0, weight=1, minsize=20)
		config_frame.rowconfigure(1, weight=1, minsize=20)
		config_frame.rowconfigure(2, weight=1, minsize=20)
		config_frame.rowconfigure(3, weight=1, minsize=20)
		config_frame.grid_propagate(False)

		test_label = tk.Label(master=config_frame, text=self.parent.current_test_name.get(), font=("Helvetica", 25, "bold"))
		test_label.pack(anchor='nw', side=tk.TOP)

		module_label = tk.Label(master=config_frame, text="Module ID: {0}".format(self.parent.current_module_id), font=("Helvetica", 25))
		module_label.pack(anchor='nw', side=tk.TOP)

		self.config_label = tk.Label(master = config_frame, text = "XML:", font=("Helvetica", 17))
		self.config_label.grid(row=1, column=0, columnspan=1, sticky='we')

		self.config_entry = tk.Entry(master = config_frame, text='')
		self.config_file = ConfigFiles.get(origin.calibration, "None")
		self.config_file = os.environ.get('GUI_dir')+self.config_file
		self.config_entry.insert(0,self.config_file)
		self.config_entry.grid(row=1, column=1, columnspan=2, sticky='we')

		self.rd53_label = tk.Label(master = config_frame, text = "RD53:", font=("Helvetica", 17))
		self.rd53_label.grid(row=2, column=0, columnspan=1, sticky='we')
		
		self.rd53_entry = tk.Entry(master = config_frame, text='')
		self.rd53_file = "/test/CMSIT_RD53.txt"
		self.rd53_file = os.environ.get('GUI_dir')+self.rd53_file
		self.rd53_entry.insert(0,self.rd53_file)
		self.rd53_entry.grid(row=2, column=1, columnspan=2, sticky='we')

		save_button = ttk.Button(
			master = config_frame,
			text = "Save",
			command = self.save
		)
		save_button.grid(row=3, column=3, columnspan=1, sticky='we')

	def save(self):
		self.origin.rd53_file = self.rd53_entry.get()
		self.origin.config_file = self.config_entry.get()
		self.destroy()
		