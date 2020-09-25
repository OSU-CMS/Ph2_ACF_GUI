'''
  ANSIColorText.py
  brief                 Option frame for the main page of GUI
  author                Kai Wei
  version               0.1
  date                  09/24/20
  Support:              email to wei.856@osu.edu
'''

import config
import tkinter as tk

from tkinter import ttk

from Gui.GUIutils.CreateWindow import *
from Gui.GUIutils.StartWindow  import *
from Gui.GUIutils.ResultsWindow import *
from Gui.GUIutils.ReviewModuleWindow import *
from Gui.GUIutils.ErrorWindow import *

##########################################################################
##########################################################################

class OptionsFrame(tk.Frame):
	def __init__(self):
		tk.Frame.__init__(self)
		self.master = config.root
		self.config(width=600, height=400)

		self.rowconfigure(0, weight=1, minsize=40)
		self.rowconfigure(1, weight=1, minsize=40)
		self.rowconfigure(2, weight=1, minsize=40)
		self.rowconfigure(3, weight=1, minsize=40)
		self.rowconfigure(4, weight=1, minsize=40)
		self.rowconfigure(5, weight=1, minsize=40)
		self.rowconfigure(6, weight=1, minsize=40)
		self.rowconfigure(7, weight=1, minsize=40)
		self.columnconfigure(0, weight=1, minsize=100)
		self.columnconfigure(1, weight=1, minsize=100)
		self.columnconfigure(2, weight=1, minsize=100)
		self.columnconfigure(3, weight=1, minsize=100)
		self.grid_propagate(False)

		self.start_button = ttk.Button(
			master = self,
			text = "Start new test", 
			command = self.showStartWindow
		)
		self.start_button.grid(row=1, column=2, columnspan=2, sticky='we')

		self.create_button = ttk.Button(
			master = self,
			text = "Create new test", 
			command = self.showCreateWindow
		)
		self.create_button.grid(row=2, column=2, columnspan=2, sticky='ew')

		self.results_button = ttk.Button(
			master = self,
			text = "Review results", 
			command = self.showResultsWindow
		)
		self.results_button.grid(row=3, column=2, columnspan=2, sticky='ew')

		self.review_module_label = tk.Label(master = self, text = "Review a specific module", font=("Helvetica", 17))
		self.review_module_label.grid(row=4, column=1, columnspan=2, sticky='we')

		self.module_num_label = tk.Label(master = self, text = "Module ID", font=("Helvetica", 15))
		self.module_num_label.grid(row=5, column=1, sticky='e')

		self.module_num_entry = tk.Entry(master = self, text='')
		self.module_num_entry.grid(row=5, column=2)

		self.review_button = ttk.Button(
			master = self,
			text = "Review", 
			command = self.check_review_moduleID
		)
		self.review_button.grid(row=5, column=3)

	def showStartWindow(self):
		StartWindow()

	def showCreateWindow(self):
		CreateWindow()

	def showResultsWindow(self):
		ResultsWindow()

		
	def check_review_moduleID(self):
		try:
			config.review_module_id = int(self.module_num_entry.get())
		except:
			ErrorWindow("Error: Please enter valid module ID")
			return
		ReviewModuleWindow()
