'''
  ANSIColorText.py
  brief                 Window for module test result review
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

from Gui.GUIutils.guiUtils import *
from Gui.GUIutils.ContinueWindow import *
from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.ErrorWindow import *


##########################################################################
##########################################################################

class ReviewModuleWindow(tk.Toplevel):
	def __init__(self, parent, dbconnection):
		tk.Toplevel.__init__(self, parent.root)

		self.parent = parent
		self.dbconnection = dbconnection

		if self.parent.current_user == '':
			ErrorWindow(self.parent, "Error: Please login")
			return
		elif self.parent.review_module_id == -1:
			ErrorWindow(self.parent, "Error: Please enter module ID")
			return

		self.parent.module_results = retrieveModuleTests(self.dbconnection, self.parent.review_module_id)
		if len(self.parent.module_results) == 0:
			ErrorWindow(self.parent, "Error: No results found for this module, start a new test")
			self.parent.current_module_id = self.parent.review_module_id 
			ContinueWindow(self.parent, self.dbconnection)
			self.destroy()
			return

		header = retriveTestTableHeader(self.dbconnection)
		col_names = list(map(lambda x: header[x][0], range(0,len(header))))
		#grade_index = col_names.index('Grading')
		time_index = col_names.index('ExecutionTime')

		#best_result = self.parent.module_results[0]
		#latest_result = self.parent.module_results[0]
		#best_result = sorted(self.parent.module_results, key=lambda r: r[grade_index])[-1]
		#latest_result = sorted(self.parent.module_results, key=lambda r: r[time_index])[-1]

		self.parent.module_results.sort(key=lambda r: r[time_index], reverse=True)
		
		self.window_width = 1200
		self.window_height = 850
		self.window_rows = [50, 500, 300]
		self.window_cols = [self.window_width]

		self.sub_rows = [50, 350, 100]
		self.sub_cols = [int(self.window_width/2)]

		self.table_rows = [75, 225]
		self.table_cols = [self.window_width-15, 15]

		self.tableb_rows = [29, 23, 23]
		self.tableb_cols = [150, 150, 150, 150, 150, 150, 150, 150]

		# dev
		assert sum(self.window_rows) == self.window_height, 'Check height'
		assert sum(self.window_cols) == self.window_width, 'Check width'
		assert self.window_rows[1] == sum(self.sub_rows), 'Check bl subview'
		assert self.window_rows[2] == sum(self.table_rows), 'Check table subview'
		assert sum(self.tableb_rows) == self.table_rows[0], 'Check tableb rows'
		assert sum(self.tableb_cols) == self.window_width, 'Check tableb cols'

		self.master = self.parent.root
		self.config(bg='white')
		self.title('Review Module ID: {0}'.format(self.parent.review_module_id))
		self.geometry("{0}x{1}".format(self.window_width, self.window_height))
		self.rowconfigure(0, weight=1, minsize=self.window_rows[0])
		self.rowconfigure(1, weight=1, minsize=self.window_rows[1])
		self.rowconfigure(2, weight=1, minsize=self.window_rows[2])
		self.columnconfigure(0, weight=1, minsize=self.window_cols[0])

		# setup header frames
		self.header_frame = tk.Frame(self, width=self.window_cols[0], height=self.window_rows[0])
		self.header_frame.grid(row=0, column=0, sticky='nwes')
		self.header_frame.grid_propagate(False)

		# setup header
		module_label = tk.Label(master = self.header_frame, text = "Module ID: {0}".format(self.parent.review_module_id), font=("Helvetica", 25, 'bold'))
		module_label.pack(anchor='ne', side=tk.LEFT)

		self.change_entry = tk.Entry(master=self.header_frame)
		self.change_entry.pack(anchor='ne', side=tk.RIGHT)

		change_button = ttk.Button(
			master = self.header_frame,
			text = "Change module:",
			command = self.try_change_module
		)
		change_button.pack(side=tk.RIGHT, anchor='ne')

		continue_button = ttk.Button(
			master = self.header_frame,
			text = "Continue Testing", 
			command = self.try_open_continue,
		)
		continue_button.pack(side=tk.RIGHT, anchor='n')

		# Histogram Frame for test history
		self.hist_frame = tk.Frame(self, width=self.window_cols[0], height=self.window_rows[1], bg='white')
		self.hist_frame.grid(row=1, column=0, sticky='nwes')
		self.hist_frame.columnconfigure(0, weight=1, minsize=self.window_cols[0])
		self.hist_frame.rowconfigure(0, weight=1, minsize=self.window_rows[1])
		self.hist_frame.grid_propagate(False)

		best_frame = tk.Frame(self.hist_frame, width=sum(self.sub_cols), height=sum(self.sub_rows), relief=tk.SUNKEN, bd=2, bg='white')
		best_frame.grid(row=0, column=0, sticky='nwes')
		best_frame.columnconfigure(0, weight=1, minsize=self.sub_cols[0])
		best_frame.rowconfigure(0, weight=1, minsize=self.sub_rows[0])
		best_frame.rowconfigure(1, weight=1, minsize=self.sub_rows[1])
		best_frame.rowconfigure(2, weight=1, minsize=self.sub_rows[2])
		best_tframe = tk.Frame(best_frame, width=self.sub_cols[0], height=self.sub_rows[1], bg='white')
		best_tframe.grid(row=1, column=0, sticky='nwes')
		best_bframe = tk.Frame(best_frame, width=self.sub_cols[0], height=self.sub_rows[2], bg='white')
		best_bframe.grid(row=2, column=0, sticky='nwes')
		
		best_frame.grid_propagate(False)
		best_tframe.grid_propagate(False)
		best_bframe.grid_propagate(False)

		# setup Histogram
		#best_title = tk.Label(master=best_frame, text="Best result", font=("Helvetica", 25),bg='white')
		#best_title.grid(row=0, column=0, sticky='n')
		#
		#best_glabel = tk.Label(master=best_bframe, text="{0}%".format(best_result[5]), font=("Helvetica", 45, 'bold'), bg='white')
		#best_glabel.pack(anchor='ne', side=tk.RIGHT, expand=False)

		#best_tlabel = tk.Label(master=best_bframe, text="{0}".format(best_result[3]), font=("Helvetica", 20), bg='white')
		#best_tlabel.pack(anchor='nw', side=tk.TOP)

		#best_dlabel = tk.Label(master=best_bframe, text="{0}".format(best_result[4]), font=("Helvetica", 20), bg='white')
		#best_dlabel.pack(anchor='nw', side=tk.TOP)

		#best_ulabel = tk.Label(master=best_bframe, text="{0}".format(best_result[2]), font=("Helvetica", 15), bg='white')
		#best_ulabel.pack(anchor='nw', side=tk.TOP)

		#FIXME: automate plot retrieval
		best_canvas = tk.Canvas(best_tframe, bg='white')
		best_canvas.pack(expand=True, fill='both', anchor='nw', side=tk.TOP)
		best_img = Image.open("test_plots/test_best1.png")
		best_img = best_img.resize((self.sub_cols[0], self.sub_rows[1]), Image.ANTIALIAS)
		self.parent.review_best_plot = ImageTk.PhotoImage(best_img)
		best_canvas.create_image(0, 0, image=self.parent.review_best_plot, anchor="nw")

		# Table Frame for test history
		self.table_frame = tk.Frame(self, width=self.window_width, height=self.window_rows[2], relief=tk.SUNKEN, bd=2)
		self.table_frame.grid(row=2, column=0, sticky='nwes')
		self.table_frame.grid_propagate(False)

		self.table_frame.columnconfigure(0, weight=1, minsize=self.window_width)
		self.table_frame.rowconfigure(0, weight=1, minsize=self.table_rows[0])
		self.table_frame.rowconfigure(1, weight=1, minsize=self.table_rows[1])
		table_tframe = tk.Frame(self.table_frame, width=self.window_width, height=self.table_rows[0])
		table_tframe.grid(row=0, column=0, sticky='nwes')
		for row in range(3):
			table_tframe.grid_rowconfigure(row, weight=1, minsize=self.tableb_rows[row])
		for col in range(len(self.tableb_cols)):
			table_tframe.grid_columnconfigure(row, weight=1, minsize=self.tableb_cols[col])

		table_bframe = tk.Frame(self.table_frame, width=self.window_width, height=self.table_rows[1])
		table_bframe.grid(row=1, column=0, sticky='nwes')
		table_bframe.rowconfigure(0, weight=1, minsize=self.table_rows[1])
		table_bframe.columnconfigure(0, weight=1, minsize=self.table_cols[0])
		table_bframe.columnconfigure(1, weight=1, minsize=self.table_cols[1])

		self.table_frame.grid_propagate(False)
		table_tframe.grid_propagate(False)
		table_bframe.grid_propagate(False)

		# setup table 
		self.parent.module_results = retrieveModuleTests(self.dbconnection, self.parent.review_module_id)
		table_label = tk.Label(master=table_tframe, text = "All results for module {0}".format(self.parent.review_module_id), font=("Helvetica", 20))
		table_label.pack(side=tk.TOP, anchor='nw')

		self.table_num = tk.Label(master=table_tframe, text = "(Showing {0} results)".format(len(self.parent.module_results)), font=("Helvetica", 12))
		self.table_num.grid(row=0, column=3, sticky='sw')

		sort_button = ttk.Button(
			master = table_tframe,
			text = "Sort by", 
			command = self.sort_results,
		)
		sort_button.grid(row=1, column=0, sticky='nsew')

		sort_menu = tk.OptionMenu(table_tframe, self.parent.review_sort_attr, *['date', 'user', 'grade', 'testname'])
		sort_menu.grid(row=1, column=1, sticky='nsew')

		dir_menu = tk.OptionMenu(table_tframe, self.parent.review_sort_dir, *["increasing", "decreasing"])
		dir_menu.grid(row=1, column=2, sticky='nsw')

		filter_button = ttk.Button(
			master = table_tframe,
			text = "Filter", 
			command = self.filter_results,
		)
		filter_button.grid(row=1, column=3, sticky='nsew', padx=(0, 0))

		filter_menu = tk.OptionMenu(table_tframe, self.parent.review_filter_attr, *['date', 'user', 'grade', 'testname'])
		filter_menu.grid(row=1, column=4, sticky='nsw')

		eq_menu = tk.OptionMenu(table_tframe, self.parent.review_filter_eq, *["=", ">=", "<=", ">", "<", "contains"])
		eq_menu.grid(row=1, column=5, sticky='nsw')

		self.filter_entry = tk.Entry(master=table_tframe)
		self.filter_entry.insert(0, '{0}'.format(self.parent.current_user))
		self.filter_entry.grid(row=1, column=6, sticky='nsw')

		reset_button = ttk.Button(
			master = table_tframe,
			text = "Reset", 
			command = self.reset_results,
		)
		reset_button.grid(row=1, column=7, sticky='nse')

		yscrollbar = tk.Scrollbar(table_bframe)
		yscrollbar.grid(row=0, column=1, sticky='nes')

		self.table_canvas = tk.Canvas(table_bframe, yscrollcommand=yscrollbar.set)
		self.table_canvas.grid(row=0, column=0, sticky='nsew')
		yscrollbar.config(command=self.table_canvas.yview)

		self.make_table()


	def filter_results(self):
		filter_option = self.parent.review_filter_attr.get()
		filter_eq = self.parent.review_filter_eq.get()
		filter_val = self.filter_entry.get()

		#self.parent.module_results = database.retrieveModuleTests(self.parent.review_module_id)
		self.parent.module_results = retrieveModuleTests(self.dbconnection, self.parent.review_module_id)
		filtered_results = []

		if self.parent.module_results[0][0] == "":
			self.parent.module_results.pop(0)

		options_dict = {"user": 2, "testname": 3, "date": 4, "grade": 5}
		if filter_eq == 'contains':
			if filter_option == 'grade':
				ErrorWindow(self.parent, 'Error: Option not supported for chosen variable')
				return 

			filtered_results = [r for r in self.parent.module_results if filter_val in r[options_dict[filter_option]]]
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
					filtered_results = [r for r in self.parent.module_results if op_dict[filter_eq](r[options_dict[filter_option]], filter_val)]
			else:
				try:
					filter_val = int(filter_val)
				except:
					ErrorWindow(self.parent, 'Error: Cannot filter a string with an numeric operation')
					return

				filtered_results = [r for r in self.parent.module_results if op_dict[filter_eq](int(r[options_dict[filter_option]]), filter_val)]
		
		self.table_canvas.delete('all')

		if len(filtered_results) == 0:
			error_label = tk.Label(master=self.table_canvas, text="No results found", font=("Helvetica", 15))
			self.table_canvas.create_window(10, 10, window=error_label, anchor="nw")
			self.table_num['text'] = "(Showing 0 results)"
			return
		
		self.parent.module_results = filtered_results
		self.make_table()


	def reset_results(self):
		#self.parent.module_results = database.retrieveModuleTests(self.parent.review_module_id)
		self.parent.module_results = retrieveModuleTests(self.dbconnection, self.parent.review_module_id)
		self.make_table()


	def back_to_module(self):
		self.rheader_frame.grid_remove()
		self.rplot_frame.grid_remove()
		self.header_frame.grid()
		self.table_frame.grid()
		self.hist_frame.grid()


	def open_result(self, result_rowid):
		self.table_frame.grid_remove()
		self.hist_frame.grid_remove()
		self.header_frame.grid_remove()

		self.rheader_frame = tk.Frame(self, width=self.window_cols[0], height=self.window_rows[0])
		self.rheader_frame.grid(row=0, column=0, sticky='nwes')
		self.rheader_frame.grid_propagate(False)

		self.rplot_frame = tk.Frame(self, width=self.window_cols[0], height=self.window_height-self.window_rows[0])
		self.rplot_frame.grid(row=1, column=0, rowspan=2, sticky='nwes')
		self.rplot_frame.grid_propagate(False)

		self.rplot_frame.grid_rowconfigure(0, weight=1, minsize=self.window_height-self.window_rows[0])
		self.rplot_frame.grid_columnconfigure(0, weight=1, minsize=self.window_cols[0]-15)
		self.rplot_frame.grid_columnconfigure(1, weight=1, minsize=15)

		# setup header
		back_image = Image.open('icons/back-arrow.png')
		back_image = back_image.resize((27, 20), Image.ANTIALIAS)
		back_photo = ImageTk.PhotoImage(back_image)

		back_button = ttk.Button(
			master = self.rheader_frame,
			text = 'Back',
			image = back_photo,	
			command = self.back_to_module
		)
		back_button.image = back_photo
		back_button.pack(side=tk.RIGHT, anchor='ne')

		#result_tuple = database.retrieveModuleTest(result_rowid)[0]
		result_tuple = retrieveModuleTest(self.dbconnection, result_rowid)[0]
		
		name_label = tk.Label(master=self.rheader_frame, text=result_tuple[3], font=("Helvetica", 25))
		name_label.pack(side=tk.LEFT, anchor='sw')

		grade_label = tk.Label(master=self.rheader_frame, text='{}%'.format(result_tuple[5]), font=("Helvetica", 30, 'bold'), padx=5)
		grade_label.pack(side=tk.LEFT, anchor='sw')

		date_label = tk.Label(master=self.rheader_frame, text=result_tuple[4], font=("Helvetica", 18), padx=4)
		date_label.pack(side=tk.LEFT, anchor='sw')

		user_label = tk.Label(master=self.rheader_frame, text=result_tuple[2], font=("Helvetica", 18))
		user_label.pack(side=tk.LEFT, anchor='sw')

		# setup plots
		#FIXME: automate plot retrieval from database
		#FIXME: organize test_plots
		test_plots = [
			['test_plots/test1.png', 'test_plots/test2.png'],
			['test_plots/test3.png', 'test_plots/test4.png'],
			['test_plots/test5.png', 'test_plots/test6.png'],
			['test_plots/test7.png']
		]

		ncols = 2
		nrows = math.ceil(len(test_plots)/2)

		plot_yscrollbar = tk.Scrollbar(self.rplot_frame)
		plot_yscrollbar.grid(row=0, column=1, sticky='nsew')

		plot_canvas = tk.Canvas(self.rplot_frame, scrollregion=(0,0,1000,self.sub_rows[1]*(nrows+3)), yscrollcommand=plot_yscrollbar.set, bg='white')
		plot_canvas.grid(row=0, column=0, sticky='nsew')
		plot_yscrollbar.config(command=plot_canvas.yview)

		self.parent.plot_images = []

		for row in range(len(test_plots)):
			self.parent.plot_images.append([])

			for col in range(len(test_plots[row])):
				plot_img = Image.open(test_plots[row][col])
				plot_img = plot_img.resize((self.sub_cols[0], self.sub_rows[1]), Image.ANTIALIAS)
				self.parent.plot_images[row].append(ImageTk.PhotoImage(plot_img))

		for row in range(len(test_plots)):
			for col in range(len(test_plots[row])):
				plot_canvas.create_image(col*(self.window_width/ncols), row*((self.window_height-self.window_rows[0])/nrows),
						image=self.parent.plot_images[row][col], anchor="nw")
		
	def show_result(self, DQMFile):
		GetTBrowser(DQMFile)

	def make_table(self):
		self.table_canvas.delete('all')
		self.table_num['text'] = "(Showing {0} results)".format(len(self.parent.module_results))

		if not self.parent.module_results:
			ErrorWindow(self.parent, "No test results available")
			return
		if self.parent.module_results[0][0] != "Module ID":
			self.parent.module_results = [[ "Module ID", "User", "Test", "Date", "Grade", "DQMFile"]] + self.parent.module_results

		n_cols = 6
		row_ids = [r[0] for r in self.parent.module_results]

		for j, result in enumerate(self.parent.module_results):

			if j != 0:
				result_button = tk.Button(
					master = self.table_canvas,
					text = "{0}".format(j)
					#command = partial(self.open_result, row_ids[j])
				)
				
				self.table_canvas.create_window(0*self.table_cols[0]/n_cols, j*25, window=result_button, anchor='nw')


			for i, item in enumerate(result):
				#if i in [5]: continue

				bold = ''
				if j == 0: bold = 'bold'
				row_label = tk.Label(master=self.table_canvas, text=str(item), font=("Helvetica", 15, bold))

				anchor = 'nw'
				if i == 5: anchor = 'ne'

				if i in [0,1,2,3,4] or j == 0:
					self.table_canvas.create_window((i*self.table_cols[0]/n_cols)+65, j*25, window=row_label, anchor=anchor)

				if i == 5 and j != 0: 
					detail_button = tk.Button(
						master = self.table_canvas,
						text = "Show Details...",
						command = self.show_result(item)
					)
					self.table_canvas.create_window((i*self.table_cols[0]/n_cols)+65, j*25, window=detail_button, anchor=anchor)


		self.table_canvas.config(scrollregion=(0,0,1000, (len(self.parent.module_results)+2)*25))



	def sort_results(self):
		sort_option = self.parent.review_sort_attr.get()
		options_dict = {"user": 2, "testname": 3, "grade": 5}

		if self.parent.module_results[0][0] == "":
			self.parent.module_results.pop(0)

		if sort_option != "date":
			self.table_canvas.delete('all')
			self.parent.module_results.sort(key=lambda x: x[options_dict[sort_option]], reverse=self.parent.review_sort_dir.get()=="increasing")
			self.make_table()

		else:
			self.parent.module_results.sort(key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"), reverse=self.parent.review_sort_dir.get()=="increasing")
			self.make_table()


	def try_change_module(self):
		try:
			self.parent.review_module_id = int(self.change_entry.get())
		except:
			ErrorWindow("Error: Please enter valid module ID")
			return

		try:
			self.destroy()
			ReviewModuleWindow(self.parent)
		except:
			ErrorWindow(self.parent, "Error: Could not change modules")
	

	def try_open_continue(self):
		try:
			self.parent.current_module_id = self.parent.review_module_id
			ContinueWindow(self.parent)
			return
		except:
			ErrorWindow(self.parent, "Error: Could not open continue window")
