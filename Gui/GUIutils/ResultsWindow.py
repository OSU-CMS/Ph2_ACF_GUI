'''
  ResultsWindow.py
  brief                 Window for showing the results
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

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
from Gui.GUIutils.ErrorWindow import *

class ResultsWindow(tk.Toplevel):
	def __init__(self, parent, dbconnection):
		tk.Toplevel.__init__(self,parent.root)
		self.parent = parent
		self.master = self.parent.root
		self.dbconnection = dbconnection

		if self.parent.current_user == '':
			ErrorWindow(self.parent, "Error: Please login")
			return

		self.re_width = scaleInvWidth(self.master, 0.6)
		self.re_height = scaleInvHeight(self.master, 0.6)
	

		self.title("Review Results")
		self.geometry("{}x{}".format(self.re_width, self.re_height))
		self.grid_columnconfigure(0, weight=1, minsize=self.re_width)
		self.grid_rowconfigure(0, weight=1, minsize=100)
		self.grid_rowconfigure(1, weight=1, minsize=self.re_height-100)

		#FIXME
		#self.parent.all_results = database.retrieveAllTestTasks()
		self.parent.all_results = retrieveAllTestResults(self.dbconnection)

		#self.parent.all_results = retrieveAllTestResults(self.dbconnection)
		self.parent.all_results.sort(key=lambda r: r[3], reverse=True)
		self.setupFrame()

	def setupFrame(self):
		nRows = len(self.parent.all_results)
		nColumns = 5

		if nRows == 0:
			self.results_lbl = tk.Label(self, text = "No results to show", font=("Helvetica", 20))
			self.results_lbl.pack()

		self.top_frame = tk.Frame(self, width=self.re_width, height=15)
		self.top_frame.grid(row=0, column=0, sticky='nsew')

		self.bot_frame = tk.Frame(self, width=self.re_width, height=self.re_height-15)
		self.bot_frame.grid(row=1, column=0, sticky='nsew')

		top_cols = [40, 40, 40, 40, 40, 40, 40, 40]
		for i, col in enumerate(top_cols):
			self.top_frame.grid_columnconfigure(i, weight=1, minsize=col)

		self.top_frame.grid_rowconfigure(0, weight=1, minsize=50)
		self.top_frame.grid_rowconfigure(1, weight=1, minsize=10)

		self.bot_frame.grid_rowconfigure(0, weight=1, minsize=self.re_height-100)
		self.bot_frame.grid_columnconfigure(0, weight=1, minsize=self.re_width-15)
		self.bot_frame.grid_columnconfigure(1, weight=1, minsize=15)

		# setup header
		self.rtable_label = tk.Label(master=self.top_frame, text = "All results", font=("Helvetica", 25, 'bold'))
		self.rtable_label.grid(row=0, column=0, columnspan=2, sticky='nsew')

		self.rtable_num = tk.Label(master=self.top_frame, text = "(Showing {0} results)".format(len(self.parent.all_results)), font=("Helvetica", 20))
		self.rtable_num.grid(row=0, column=2, columnspan=2, sticky='nsew')

		self.rsort_button = ttk.Button(
			master = self.top_frame,
			text = "Sort by", 
			command = self.sort_results,
		)
		self.rsort_button.grid(row=1, column=0, sticky='nsew')

		self.rsort_menu = tk.OptionMenu(self.top_frame, self.parent.result_sort_attr, *['module ID', 'date', 'user', 'grade', 'testname'])
		self.rsort_menu.grid(row=1, column=1, sticky='nsew')

		self.rdir_menu = tk.OptionMenu(self.top_frame, self.parent.result_sort_dir, *["increasing", "decreasing"])
		self.rdir_menu.grid(row=1, column=2, sticky='nsw')

		self.rfilter_button = ttk.Button(
			master = self.top_frame,
			text = "Filter", 
			command = self.filter_results,
		)
		self.rfilter_button.grid(row=1, column=3, sticky='nsew', padx=(20, 0))

		self.rfilter_menu = tk.OptionMenu(self.top_frame, self.parent.result_filter_attr, *['module ID', 'date', 'user', 'grade', 'testname'])
		self.rfilter_menu.grid(row=1, column=4, sticky='nsew')

		self.req_menu = tk.OptionMenu(self.top_frame, self.parent.result_filter_eq, *["=", ">=", "<=", ">", "<", "contains"])
		self.req_menu.grid(row=1, column=5, sticky='nsew')

		self.rfilter_entry = tk.Entry(master=self.top_frame)
		self.rfilter_entry.insert(0, '{0}'.format(self.parent.current_user))
		self.rfilter_entry.grid(row=1, column=6, sticky='nsew')

		self.rreset_button = ttk.Button(
			master = self.top_frame,
			text = "Reset", 
			command = self.reset_results,
		)
		self.rreset_button.grid(row=1, column=7, sticky='nsew')

		self.ryscrollbar = tk.Scrollbar(self.bot_frame)
		self.ryscrollbar.grid(row=0, column=1, sticky='nsew')

		self.rtable_canvas = tk.Canvas(self.bot_frame, yscrollcommand=self.ryscrollbar.set)
		self.rtable_canvas.grid(row=0, column=0, sticky='nsew')
		self.ryscrollbar.config(command=self.rtable_canvas.yview)

		self.rmake_table()


	def sort_results(self):
		sort_option = self.parent.result_sort_attr.get()
		options_dict = {"module ID": 0, "user": 1, "testname": 2, "grade": 4}

		if self.parent.all_results[0][0] == "":
			self.parent.all_results.pop(0)

		if sort_option != "date":
			self.rtable_canvas.delete('all')
			self.parent.all_results.sort(key=lambda x: x[options_dict[sort_option]], reverse=self.parent.result_sort_dir.get()=="increasing")
			self.rmake_table()
		else:
			#self.parent.all_results.sort(key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"), reverse=self.parent.result_sort_dir.get()=="increasing")
			self.parent.all_results.sort(key=lambda r: r[3], reverse=self.parent.result_sort_dir.get()=="increasing")
			self.rmake_table()


	def filter_results(self):
		filter_option = self.parent.result_filter_attr.get()
		filter_eq = self.parent.result_filter_eq.get()
		filter_val = self.rfilter_entry.get()

		# FIXME
		#self.parent.all_results = database.retrieveAllTestTasks()
		self.parent.all_results = retrieveAllTestResults(self.dbconnection)

		filtered_results = []

		if self.parent.all_results[0][0] == "":
			self.parent.all_results.pop(0)

		options_dict = {"module ID": 0, "user": 1, "testname": 2, "date": 3, "grade": 4}
		if filter_eq == 'contains':
			if filter_option == 'grade' or filter_option == 'module ID':
				ErrorWindow(self.parent, 'Error: Option not supported for chosen variable')
				return 

			filtered_results = [r for r in self.parent.all_results if filter_val in r[options_dict[filter_option]]]
		else:
			op_dict = {
					'=': operator.eq, '>=': operator.ge, '>': operator.gt,
					'<=': operator.le, '<': operator.lt
					}	

			if filter_option in ["user", "testname", "date"]:
				if filter_eq != '=':
					ErrorWindow(self.parent, 'Error: Option not supported for chosen variable')
					return
				else: 
					filtered_results = [r for r in self.parent.all_results if op_dict[filter_eq](r[options_dict[filter_option]], filter_val)]
			else:
				try:
					filter_val = int(filter_val)
				except:
					ErrorWindow(self.parent, 'Error: Cannot filter a string with an numeric operation')
					return

				filtered_results = [r for r in self.parent.all_results if op_dict[filter_eq](int(r[options_dict[filter_option]]), filter_val)]
		
		self.rtable_canvas.delete('all')

		if len(filtered_results) == 0:
			self.error_label = tk.Label(master=self.rtable_canvas, text="No results found", font=("Helvetica", 15))
			self.rtable_canvas.create_window(10, 10, window=self.error_label, anchor="nw")
			self.rtable_num['text'] = "(Showing 0 results)"
			return
		
		self.parent.all_results = filtered_results
		self.rmake_table()


	def reset_results(self):
		#self.parent.all_results = database.retrieveAllTestTasks()
		self.parent.all_results = retrieveAllTestResults(self.dbconnection)
		self.rmake_table()


	def rmake_table(self):
		self.rtable_canvas.delete('all')
		self.rtable_num['text'] = "(Showing {0} results)".format(len(self.parent.all_results))

		if not self.parent.all_results:
			ErrorWindow(self.parent, "No test results available")
			return
		if self.parent.all_results[0][0] != "Module ID":
			self.parent.all_results = [[ "Module ID", "User", "Test", "Date", "Grade"]] + self.parent.all_results

		n_cols = 5
		row_ids = [r[0] for r in self.parent.all_results]

		for j, result in enumerate(self.parent.all_results):

			if j != 0:
				result_button = tk.Button(
					master = self.rtable_canvas,
					text = "{0}".format(j)
					#command = partial(openResult, row_ids[j])
				)
				
				self.rtable_canvas.create_window(0*(self.re_width-15)/n_cols, j*25, window=result_button, anchor='nw')


			for i, item in enumerate(result):
				if i in [5]: continue

				bold = ''
				if j == 0: bold = 'bold'
				row_label = tk.Label(master=self.rtable_canvas, text=str(item), font=("Helvetica", 15, bold))

				anchor = 'nw'
				if i == 5: anchor = 'ne'
				self.rtable_canvas.create_window((i*(self.re_width-15)/n_cols)+65, j*25, window=row_label, anchor=anchor)

		self.rtable_canvas.config(scrollregion=(0,0,1000, (len(self.parent.all_results)+2)*25))