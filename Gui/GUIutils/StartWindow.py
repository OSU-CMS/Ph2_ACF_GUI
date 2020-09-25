'''
  StartWindow.py
  brief                 Create test options
  author                Kai Wei
  version               0.1
  date                  09/24/20
  Support:              email to wei.856@osu.edu
'''

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

from Gui.GUIutils.RunWindow import *
from Gui.GUIutils.ErrorWindow import *

class StartWindow(tk.Toplevel):
	def __init__(self):
		if config.current_user == '':
			ErrorWindow("Error: Please login")
			return
		tk.Toplevel.__init__(self)

		self.master = config.root
		self.title('New test')
		self.geometry("500x200")

		for row in range(1,5):
			self.rowconfigure(row, weight=1, minsize=40)

		self.rowconfigure(0, weight=1, minsize=20)
		self.rowconfigure(5, weight=1, minsize=20)
		self.columnconfigure(1, weight=1, minsize=200)
		self.columnconfigure(2, weight=1, minsize=200)
		self.columnconfigure(0, weight=1, minsize=50)
		self.columnconfigure(3, weight=1, minsize=50)

		self.start_label = tk.Label(master=self, text="Start a new test", font=("Helvetica", 20, 'bold'))
		self.start_label.grid(row=1, column=1, columnspan=2, sticky="wen")

		self.moduleID_label = tk.Label(master=self, text="Module ID", font=("Helvetica", 15))
		self.moduleID_label.grid(row=2, column=1, sticky='w')

		self.moduleID_entry = tk.Entry(master=self)
		self.moduleID_entry.grid(row=2, column=2, sticky='e')

		self.test_mode_label = tk.Label(master=self, text="Test mode", font=("Helvetica", 15))
		self.test_mode_label.grid(row=3, column=1, sticky='w')

		current_modes = []
		for mode in list(database.retrieveAllModes()):
			current_modes.append(mode[1])

		self.mode_menu = tk.OptionMenu(self, config.current_test_name, *current_modes)
		self.mode_menu.grid(row=3, column=2, sticky='e')

		self.start_button = ttk.Button(
			master = self,
			text = "Start test", 
			command = self.open_self
		)
		self.start_button.grid(row=4, column=1, columnspan=2)

	def open_self(self):
		try:
			config.current_module_id = int(self.moduleID_entry.get())
		except:
			ErrorWindow("Error: Please enter valid module ID")
			return
		RunWindow()
		self.destroy()