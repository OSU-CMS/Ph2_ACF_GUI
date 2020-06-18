'''
  gui.py
  \brief                 Interface classes for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
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

class LoginFrame(tk.Frame):
	def __init__(self):
		tk.Frame.__init__(self)

		self.cms_img = Image.open('icons/cmsicon.png')
		self.cms_img = self.cms_img.resize((70, 70), Image.ANTIALIAS)
		self.cms_photo = ImageTk.PhotoImage(self.cms_img)
		self.osu_img = Image.open('icons/osuicon.png')
		self.osu_img = self.osu_img.resize((160, 70), Image.ANTIALIAS)
		self.osu_photo = ImageTk.PhotoImage(self.osu_img)

		self.master = config.root
		self.config(width=600, height=250)

		self.grid(row=1, column=0, columnspan=2, sticky="nsew")
		self.rowconfigure(0, weight=1, minsize=35)
		self.rowconfigure(1, weight=1, minsize=25)
		self.rowconfigure(2, weight=1, minsize=40)
		self.rowconfigure(3, weight=1, minsize=30)
		self.rowconfigure(4, weight=1, minsize=65)
		self.rowconfigure(5, weight=1, minsize=55)
		self.columnconfigure(0, weight=1, minsize=300)
		self.columnconfigure(1, weight=1, minsize=300)
		self.grid_propagate(False)

		self.login_label = tk.Label(master = self, text = "Please login", font=("Helvetica", 20, 'bold'))
		self.login_label.grid(row=1, column=0, columnspan=2, sticky="n")

		self.username_label = tk.Label(master = self, text = "Username", font=("Helvetica", 15))
		self.username_label.grid(row=2, column=0, sticky='e')

		self.username_entry = tk.Entry(master = self, text='')
		self.username_entry.grid(row=2, column=1, sticky='w')

		self.osu_label = tk.Label(master=self, image=self.osu_photo)
		self.osu_label.grid(row=4, column=0, sticky='ewns')

		self.cms_label = tk.Label(master=self, image=self.cms_photo)
		self.cms_label.grid(row=4, column=1, sticky='ewns')

		self.login_button = ttk.Button(
			master = self, 
			text = "Log in", 
			command = self.login_user
		)
		self.login_button.grid(row=3, column=0, columnspan=2)

		self.logout_button = ttk.Button(
			master = self, 
			text = "Log out", 
			command = self.logout_user
		)

		self.options_frame = OptionsFrame()


	def login_user(self):
		if self.username_entry.get() == '':
			ErrorWindow("Error: Please enter a valid username")
			return
		config.current_user = self.username_entry.get()

		self.login_label['text'] = 'Welcome {0}'.format(config.current_user)
		self.login_label['font'] = ('Helvetica', 20, 'bold')

		self.username_label.grid_remove()
		self.username_entry.grid_remove()
		self.login_button.grid_remove()
		self.logout_button.grid(row=2, column=0, columnspan=2)
		self.options_frame.grid(row=1, column=1, sticky="es") #FIXME
		self.grid(row=1, column=0,  sticky="ws")
		self['width'] = 300
		self.columnconfigure(0, weight=1, minsize=150)
		self.columnconfigure(1, weight=1, minsize=150)


	def logout_user(self):
		config.current_user = ''

		self.login_label['text'] = 'Please login'
		self.login_label['font'] = ('Helvetica', 20, 'bold')
		self.username_label.grid()
		self.username_entry.grid()
		self.login_button.grid()
		
		self.logout_button.grid_remove()
		self.options_frame.grid_remove()
		self.grid(row=1, column=0,  sticky="news")
		self['width'] = 600
		self.columnconfigure(0, weight=1, minsize=300)
		self.columnconfigure(1, weight=1, minsize=300)

##########################################################################
##########################################################################

class OptionsFrame(tk.Frame):
	def __init__(self):
		tk.Frame.__init__(self)
		self.master = config.root
		self.config(width=300, height=250)

		self.rowconfigure(0, weight=1, minsize=5)
		self.rowconfigure(1, weight=1, minsize=40)
		self.rowconfigure(2, weight=1, minsize=40)
		self.rowconfigure(3, weight=1, minsize=40)
		self.rowconfigure(4, weight=1, minsize=40)
		self.rowconfigure(5, weight=1, minsize=40)
		self.rowconfigure(6, weight=1, minsize=40)
		self.rowconfigure(7, weight=1, minsize=5)
		self.columnconfigure(0, weight=1, minsize=100)
		self.columnconfigure(1, weight=1, minsize=100)
		self.columnconfigure(2, weight=1, minsize=100)
		self.grid_propagate(False)

		self.start_button = ttk.Button(
			master = self,
			text = "Start new test", 
			command = StartWindow
		)
		self.start_button.grid(row=1, column=0, columnspan=3, sticky='we')

		self.create_button = ttk.Button(
			master = self,
			text = "Create new test", 
			command = CreateWindow
		)
		self.create_button.grid(row=2, column=0, columnspan=3, sticky='ew')

		self.results_button = ttk.Button(
			master = self,
			text = "Review results", 
			command = ResultsWindow
		)
		self.results_button.grid(row=3, column=0, columnspan=3, sticky='ew')

		self.review_module_label = tk.Label(master = self, text = "Review a specific module", font=("Helvetica", 17))
		self.review_module_label.grid(row=4, column=0, columnspan=3, sticky='we')

		self.module_num_label = tk.Label(master = self, text = "Module ID", font=("Helvetica", 15))
		self.module_num_label.grid(row=5, column=0, sticky='e')

		self.module_num_entry = tk.Entry(master = self, text='')
		self.module_num_entry.grid(row=5, column=1)

		self.review_button = ttk.Button(
			master = self,
			text = "Review", 
			command = self.check_review_moduleID
		)
		self.review_button.grid(row=5, column=2)


	def check_review_moduleID(self):
		try:
			config.review_module_id = int(self.module_num_entry.get())
		except:
			ErrorWindow("Error: Please enter valid module ID")
			return
		ReviewModuleWindow()

##########################################################################
##########################################################################

class AnsiColorText(tk.Text):
	foreground_colors = {
			'bright' : {
							'30' : 'Black',
							'31' : 'Red',
							'32' : 'Green',
							'33' : 'Brown',
							'34' : 'Blue',
							'35' : 'Purple',
							'36' : 'Cyan',
							'37' : 'White'
							},
			'dim'    :  {
							'30' : 'DarkGray',
							'31' : 'LightRed',
							'32' : 'LightGreen',
							'33' : 'Yellow',
							'34' : 'LightBlue',
							'35' : 'Magenta',
							'36' : 'Pink',
							'37' : 'White'
							}
		}

	background_colors= {
			'bright' : {
							'40' : 'Black',
							'41' : 'Red',
							'42' : 'Green',
							'43' : 'Brown',
							'44' : 'Blue',
							'45' : 'Purple',
							'46' : 'Cyan',
							'47' : 'White'
							},
			'dim'    :  {
							'40' : 'DarkGray',
							'41' : 'LightRed',
							'42' : 'LightGreen',
							'43' : 'Yellow',
							'44' : 'LightBlue',
							'45' : 'Magenta',
							'46' : 'Pink',
							'47' : 'White'
							}
		}

	color_pat = re.compile('\x01?\x1b\[([\d+;]*?)m\x02?')
	inner_color_pat = re.compile("^(\d+;?)+$")

	def __init__(self, *args, **kwargs):
		tk.Text.__init__(self, *args, **kwargs)
		self.known_tags = set([])
		self.register_tag("37", "White", "Black")
		self.reset_to_default_attribs()

	def reset_to_default_attribs(self):
		self.tag = '37'
		self.bright = 'bright'
		self.foregroundcolor = 'White'
		self.backgroundcolor = 'Black'

	def register_tag(self, txt, foreground, background):
		self.tag_config(txt, foreground=foreground, background=background)
		self.known_tags.add(txt)

	def write(self, text, is_editable=False):
		segments = AnsiColorText.color_pat.split(text)
		if segments:
			for text in segments:
				if AnsiColorText.inner_color_pat.match(text):
					if text not in self.known_tags:
						parts = text.split(";")
						for part in parts:
							if part in AnsiColorText.foreground_colors[self.bright]:
								self.foregroundcolor = AnsiColorText.foreground_colors[self.bright][part]
							elif part in AnsiColorText.background_colors[self.bright]:
								self.backgroundcolor = AnsiColorText.background_colors[self.bright][part]
							else:
								for ch in part:
									if ch == '0' :
										self.reset_to_default_attribs()
									if ch == '1' :
										self.bright = 'bright'
									if ch == '2' :
										self.bright = 'dim'

					self.register_tag(text,
										foreground=self.foregroundcolor,
										background=self.backgroundcolor)
					self.tag = text
				elif text == '':
					self.tag = '37' # black
				else:
					self.insert(tk.END,text,self.tag)

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

class AbortWindow(tk.Toplevel):
	def __init__(self, window):
		tk.Toplevel.__init__(self)

		self.master = config.root
		self.title("Abort test")
		self.geometry("500x100")

		warning_label = tk.Label(master=self, text="Are you sure you want to abort? (Cannot be undone)", font=("Helvetica", 20))
		warning_label.pack()

		abort_button_a = ttk.Button(
			master = self,
			text = "Abort",
			command = partial(self.abort_action, window)
		)
		abort_button_a.pack()

		cancel_button = ttk.Button(
			master = self,
			text = "Cancel",
			command = self.go_back
		)
		cancel_button.pack()
	

	def abort_action(self, window):
		window.destroy()
		self.destroy()


	def go_back(self):
		self.destroy()

##########################################################################
##########################################################################

class RunWindow(tk.Toplevel):
	def __init__(self):
		tk.Toplevel.__init__(self)

		# setup root window
		self.master = config.root
		self.title("Grading Run")
		self.geometry("1200x800")
		self.rowconfigure(0, weight=1, minsize=800)
		self.columnconfigure(0, weight=1, minsize=400)
		self.columnconfigure(1, weight=1, minsize=600)

		# setup left side
		left_width = 500
		left_frame = tk.Frame(self, width=left_width, height=800)
		left_frame.grid(row=0, column=0, sticky='nsew')
		left_frame.columnconfigure(0, weight=1, minsize=left_width)
		left_frame.rowconfigure(0, weight=1, minsize=200)
		left_frame.rowconfigure(1, weight=1, minsize=600)
		left_frame.grid_propagate(False)

		info_frame = tk.Frame(left_frame, width=left_width, height=200)
		info_frame.grid(row=0, column=0, sticky='nsew')
		
		console_frame = tk.Frame(left_frame, width=left_width, height=600, relief=tk.GROOVE, bd=2, bg='black')
		console_frame.grid(row=1, column=0, sticky='nsew')

		test_label = tk.Label(master=info_frame, text=config.current_test_name.get(), font=("Helvetica", 25, "bold"))
		test_label.pack(anchor='nw', side=tk.TOP)

		module_label = tk.Label(master=info_frame, text="Module ID: {0}".format(config.current_module_id), font=("Helvetica", 25))
		module_label.pack(anchor='nw', side=tk.TOP)

		start_button = ttk.Button(
			master = info_frame,
			text = "Start",
			command = self.run_test
		)
		start_button.place(x=0, y=100)

		abort_button = ttk.Button(
			master = info_frame,
			text = "Abort",
			command = self.abort_test
		)
		abort_button.place(x=120, y=100)

		save_button = ttk.Button(
			master = info_frame,
			text = "Save",
			command = self.save_test
		)
		save_button.place(x=240, y=100)

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

		user_label = tk.Label(master=grade_frame, text="{0}".format(config.current_user), font=("Helvetica", 20))
		user_label.pack(side=tk.RIGHT)

		self.emu_label = tk.Label(master=grade_frame, text="", font=("Helvetica", 20), fg='red')
		self.emu_label.pack(side=tk.RIGHT)

		grade_grade = tk.Label(master=grade_frame, text="{0}%".format(config.current_test_grade), font=("Helvetica", 45, "bold"))
		grade_grade.pack(anchor='sw', side=tk.BOTTOM)

		if config.current_test_grade < 0: grade_grade['text'] = '-%'

		grade_label = tk.Label(master=grade_frame, text="Grade", font=("Helvetica", 25))
		grade_label.pack(anchor='sw', side=tk.BOTTOM)

		self.plot_canvas = tk.Canvas(plot_frame)
		self.plot_canvas.pack(expand=True, fill='both', anchor='ne', side=tk.TOP)

		self.plot_label = tk.Label(master=self.plot_canvas, text='Plot will appear here', font=("Helvetica", 20))
		self.plot_label.pack(side=tk.TOP, anchor='nw')
		
		review_button = ttk.Button(
			master = footer_frame,
			text = "Review Module {0}".format(config.current_module_id),
			command = self.go_to_review_module
		)
		review_button.pack()


	def go_to_review_module(self):
		config.review_module_id = config.current_module_id
		ReviewModuleWindow()
		return


	def abort_test(self):
		AbortWindow(self)
		return
			

	def run_test(self):
		self.run_process = Popen(['python', 'CMSminiDAQ_wrap.py'], stdout=PIPE, stderr=PIPE)
		q = Queue(maxsize=1024)
		t = Thread(target=self.reader_thread, args=[q])
		t.daemon = True
		t.start()
		self.update(q)

		#FIXME: automate plot retrieval 
		# retrieveResultPlot('', 'Run000000_PixelAlive.root', 'Detector/Board_0/Module_0/Chip_0/D_B(0)_M(0)_PixelAlive_Chip(0)', 'testing1.png')

		re_img = Image.open("test_plots/pixelalive_ex.png")
		config.current_test_plot = ImageTk.PhotoImage(re_img)
		self.plot_label.pack_forget()
		self.plot_canvas.create_image(0, 0, image=config.current_test_plot, anchor="nw")


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

		config.root.after(40, self.update, q)

	def save_test(self):
		if config.current_test_grade < 0:
			ErrorWindow("Error: Finish test before saving")
			return 

		try:
			current_date = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
			testing_info = (config.new_test_moduleID, config.current_user, config.new_test_name.get(), current_date, config.new_test_grade)
			database.createTestEntry(testing_info)
		except:
			ErrorWindow("Error: Could not save")
			return
	
	def quit(self):
		self.run_process.kill()
		self.destroy()

##########################################################################
##########################################################################

#FIXME: check if this window is needed
class CreateWindow(tk.Toplevel):
	def __init__(self):
		tk.Toplevel.__init__(self)

		if config.current_user == '':
			ErrorWindow("Error: Please login")
			return

		self.master = config.root
		self['bg'] = 'white'
		self.title("Create Test")
		self.geometry("1000x500")

		create_tmp_lbl = tk.Label(master = self, text = "Create test options will appear in this window", font=("Helvetica", 20))
		create_tmp_lbl.pack()

##########################################################################
##########################################################################

class ResultsWindow(tk.Toplevel):
	def __init__(self):
		tk.Toplevel.__init__(self)

		if config.current_user == '':
			ErrorWindow("Error: Please login")
			return

		self.re_width = 900
		re_height = 500
	
		self.master = config.root
		self.title("Review Results")
		self.geometry("{}x{}".format(self.re_width, re_height))
		self.grid_columnconfigure(0, weight=1, minsize=self.re_width)
		self.grid_rowconfigure(0, weight=1, minsize=100)
		self.grid_rowconfigure(1, weight=1, minsize=re_height-100)

		config.all_results = database.retrieveAllTestTasks()
		config.all_results.sort(key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"), reverse=True)

		nRows = len(config.all_results)
		nColumns = 5

		if nRows == 0:
			self.results_lbl = tk.Label(self, text = "No results to show", font=("Helvetica", 20))
			self.results_lbl.pack()

		self.top_frame = tk.Frame(self, width=self.re_width, height=100)
		self.top_frame.grid(row=0, column=0, sticky='nsew')

		self.bot_frame = tk.Frame(self, width=self.re_width, height=re_height-100)
		self.bot_frame.grid(row=1, column=0, sticky='nsew')

		top_cols = [40, 40, 40, 40, 40, 40, 40, 40]
		for i, col in enumerate(top_cols):
			self.top_frame.grid_columnconfigure(i, weight=1, minsize=col)

		self.top_frame.grid_rowconfigure(0, weight=1, minsize=50)
		self.top_frame.grid_rowconfigure(1, weight=1, minsize=50)

		self.bot_frame.grid_rowconfigure(0, weight=1, minsize=re_height-100)
		self.bot_frame.grid_columnconfigure(0, weight=1, minsize=self.re_width-15)
		self.bot_frame.grid_columnconfigure(1, weight=1, minsize=15)

		# setup header
		self.rtable_label = tk.Label(master=self.top_frame, text = "All results", font=("Helvetica", 25, 'bold'))
		self.rtable_label.grid(row=0, column=0, columnspan=2, sticky='nsew')

		self.rtable_num = tk.Label(master=self.top_frame, text = "(Showing {0} results)".format(len(config.all_results)), font=("Helvetica", 20))
		self.rtable_num.grid(row=0, column=2, columnspan=2, sticky='nsew')

		self.rsort_button = ttk.Button(
			master = self.top_frame,
			text = "Sort by", 
			command = self.sort_results,
		)
		self.rsort_button.grid(row=1, column=0, sticky='nsew')

		self.rsort_menu = tk.OptionMenu(self.top_frame, config.result_sort_attr, *['module ID', 'date', 'user', 'grade', 'testname'])
		self.rsort_menu.grid(row=1, column=1, sticky='nsew')

		self.rdir_menu = tk.OptionMenu(self.top_frame, config.result_sort_dir, *["increasing", "decreasing"])
		self.rdir_menu.grid(row=1, column=2, sticky='nsw')

		self.rfilter_button = ttk.Button(
			master = self.top_frame,
			text = "Filter", 
			command = self.filter_results,
		)
		self.rfilter_button.grid(row=1, column=3, sticky='nsew', padx=(20, 0))

		self.rfilter_menu = tk.OptionMenu(self.top_frame, config.result_filter_attr, *['module ID', 'date', 'user', 'grade', 'testname'])
		self.rfilter_menu.grid(row=1, column=4, sticky='nsw')

		self.req_menu = tk.OptionMenu(self.top_frame, config.result_filter_eq, *["=", ">=", "<=", ">", "<", "contains"])
		self.req_menu.grid(row=1, column=5, sticky='nsw')

		self.rfilter_entry = tk.Entry(master=self.top_frame)
		self.rfilter_entry.insert(0, '{0}'.format(config.current_user))
		self.rfilter_entry.grid(row=1, column=6, sticky='w')

		self.rreset_button = ttk.Button(
			master = self.top_frame,
			text = "Reset", 
			command = self.reset_results,
		)
		self.rreset_button.grid(row=1, column=7, sticky='nse')

		self.ryscrollbar = tk.Scrollbar(self.bot_frame)
		self.ryscrollbar.grid(row=0, column=1, sticky='ns')

		self.rtable_canvas = tk.Canvas(self.bot_frame, yscrollcommand=self.ryscrollbar.set)
		self.rtable_canvas.grid(row=0, column=0, sticky='nsew')
		self.ryscrollbar.config(command=self.rtable_canvas.yview)

		self.rmake_table()


	def sort_results(self):
		sort_option = config.result_sort_attr.get()
		options_dict = {"module ID": 1, "user": 2, "testname": 3, "grade": 5}

		if config.all_results[0][0] == "":
			config.all_results.pop(0)

		if sort_option != "date":
			self.rtable_canvas.delete('all')
			config.all_results.sort(key=lambda x: x[options_dict[sort_option]], reverse=config.result_sort_dir.get()=="increasing")
			self.rmake_table()
		else:
			config.all_results.sort(key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"), reverse=config.result_sort_dir.get()=="increasing")
			self.rmake_table()


	def filter_results(self):
		filter_option = config.result_filter_attr.get()
		filter_eq = config.result_filter_eq.get()
		filter_val = self.rfilter_entry.get()

		config.all_results = database.retrieveAllTestTasks()
		filtered_results = []

		if config.all_results[0][0] == "":
			config.all_results.pop(0)

		options_dict = {"module ID": 1, "user": 2, "testname": 3, "date": 4, "grade": 5}
		if filter_eq == 'contains':
			if filter_option == 'grade' or filter_option == 'module ID':
				ErrorWindow('Error: Option not supported for chosen variable')
				return 

			filtered_results = [r for r in config.all_results if filter_val in r[options_dict[filter_option]]]
		else:
			op_dict = {
					'=': operator.eq, '>=': operator.ge, '>': operator.gt,
					'<=': operator.le, '<': operator.lt
					}	

			if filter_option in ["user", "testname", "date"]:
				if filter_eq != '=':
					ErrorWindow('Error: Option not supported for chosen variable')
					return
				else: 
					filtered_results = [r for r in config.all_results if op_dict[filter_eq](r[options_dict[filter_option]], filter_val)]
			else:
				try:
					filter_val = int(filter_val)
				except:
					ErrorWindow('Error: Cannot filter a string with an numeric operation')
					return

				filtered_results = [r for r in config.all_results if op_dict[filter_eq](int(r[options_dict[filter_option]]), filter_val)]
		
		self.rtable_canvas.delete('all')

		if len(filtered_results) == 0:
			self.error_label = tk.Label(master=rtable_canvas, text="No results found", font=("Helvetica", 15))
			self.rtable_canvas.create_window(10, 10, window=error_label, anchor="nw")
			self.rtable_num['text'] = "(Showing 0 results)"
			return
		
		config.all_results = filtered_results
		self.rmake_table()


	def reset_results(self):
		config.all_results = database.retrieveAllTestTasks()
		self.rmake_table()


	def rmake_table(self):
		self.rtable_canvas.delete('all')
		self.rtable_num['text'] = "(Showing {0} results)".format(len(config.all_results))

		if config.all_results[0][0] != "":
			config.all_results = [["", "Module ID", "User", "Test", "Date", "Grade"]] + config.all_results

		n_cols = 5
		row_ids = [r[0] for r in config.all_results]

		for j, result in enumerate(config.all_results):

			if j != 0:
				result_button = tk.Button(
					master = self.rtable_canvas,
					text = "{0}".format(j)
					# command = partial(openResult, row_ids[j])
				)
				
				self.rtable_canvas.create_window(0*(self.re_width-15)/n_cols, j*25, window=result_button, anchor='nw')


			for i, item in enumerate(result):
				if i in [0]: continue

				bold = ''
				if j == 0: bold = 'bold'
				row_label = tk.Label(master=self.rtable_canvas, text=str(item), font=("Helvetica", 15, bold))

				anchor = 'nw'
				if i == 5: anchor = 'ne'
				self.rtable_canvas.create_window(((i-1)*(self.re_width-15)/n_cols)+65, j*25, window=row_label, anchor=anchor)

		self.rtable_canvas.config(scrollregion=(0,0,1000, (len(config.all_results)+2)*25))
	
##########################################################################
##########################################################################

class ErrorWindow(tk.Toplevel):
	def __init__(self, text):
		tk.Toplevel.__init__(self)
		self.master = config.root
		self.title("ERROR")
		self.geometry("600x50")
		error_label = tk.Label(master=self, text=text, font=("Helvetica", 15, "bold"))
		error_label.pack(expand=True, fill=tk.BOTH)

##########################################################################
##########################################################################

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

##########################################################################
##########################################################################

class ContinueWindow(tk.Toplevel):
	def __init__(self):
		tk.Toplevel.__init__(self)

		if config.current_user == '':
			ErrorWindow("Error: Please login")
			return

		self.master = config.root
		self.title('Continue testing')
		self.geometry("500x200")

		for row in range(1,5):
			self.rowconfigure(row, weight=1, minsize=40)

		self.rowconfigure(0, weight=1, minsize=20)
		self.rowconfigure(5, weight=1, minsize=20)
		self.columnconfigure(1, weight=1, minsize=200)
		self.columnconfigure(2, weight=1, minsize=200)
		self.columnconfigure(0, weight=1, minsize=50)
		self.columnconfigure(3, weight=1, minsize=50)

		self.start_label = tk.Label(master = self, text = "Start a new test", font=("Helvetica", 20, 'bold'))
		self.start_label.grid(row=1, column=1, columnspan=2, sticky="wen")

		self.moduleID_label = tk.Label(master = self, text = "Module ID", font=("Helvetica", 15))
		self.moduleID_label.grid(row=2, column=1, sticky='w')

		self.moduleID_entry = tk.Entry(master = self)
		self.moduleID_entry.grid(row=2, column=2, sticky='e')
		self.moduleID_entry.insert(0, str(config.current_module_id))

		self.test_mode_label = tk.Label(master = self, text = "Test mode", font=("Helvetica", 15))
		self.test_mode_label.grid(row=3, column=1, sticky='w')

		currentModes = []
		for mode in list(database.retrieveAllModes()):
			currentModes.append(mode[1])

		self.mode_menu = tk.OptionMenu(self, config.current_test_name, *currentModes)
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

##########################################################################
##########################################################################

class ReviewModuleWindow(tk.Toplevel):
	def __init__(self):
		tk.Toplevel.__init__(self)

		if config.current_user == '':
			ErrorWindow("Error: Please login")
			return
		elif config.review_module_id == -1:
			ErrorWindow("Error: Please enter module ID")
			return

		config.module_results = database.retrieveModuleTests(config.review_module_id)
		if len(config.module_results) == 0:
			ErrorWindow("Error: No results found for this module, start a new test")
			config.current_module_id = config.review_module_id 
			ContinueWindow()
			self.destroy()
			return

		best_result = config.module_results[0]
		latest_result = config.module_results[0]

		best_result = sorted(config.module_results, key=lambda r: r[5])[-1]
		latest_result = sorted(config.module_results, key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"))[-1]

		config.module_results.sort(key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"), reverse=True)
		
		self.window_width = 1000
		self.window_height = 850
		self.window_rows = [50, 500, 300]
		self.window_cols = [self.window_width]

		self.sub_rows = [50, 350, 100]
		self.sub_cols = [int(self.window_width/2)]

		self.table_rows = [75, 225]
		self.table_cols = [self.window_width-15, 15]

		self.tableb_rows = [29, 23, 23]
		self.tableb_cols = [125, 125, 125, 125, 125, 125, 125, 125]

		# dev
		assert sum(self.window_rows) == self.window_height, 'Check height'
		assert sum(self.window_cols) == self.window_width, 'Check width'
		assert self.window_rows[1] == sum(self.sub_rows), 'Check bl subview'
		assert self.window_rows[2] == sum(self.table_rows), 'Check table subview'
		assert sum(self.tableb_rows) == self.table_rows[0], 'Check tableb rows'
		assert sum(self.tableb_cols) == self.window_width, 'Check tableb cols'

		self.master = config.root
		self.config(bg='white')
		self.title('Review Module ID: {0}'.format(config.review_module_id))
		self.geometry("{0}x{1}".format(self.window_width, self.window_height))
		self.rowconfigure(0, weight=1, minsize=self.window_rows[0])
		self.rowconfigure(1, weight=1, minsize=self.window_rows[1])
		self.rowconfigure(2, weight=1, minsize=self.window_rows[2])
		self.columnconfigure(0, weight=1, minsize=self.window_cols[0])

		# setup base frames
		self.header_frame = tk.Frame(self, width=self.window_cols[0], height=self.window_rows[0])
		self.header_frame.grid(row=0, column=0, sticky='nwes')
		self.header_frame.grid_propagate(False)

		self.bl_frame = tk.Frame(self, width=self.window_cols[0], height=self.window_rows[1], bg='white')
		self.bl_frame.grid(row=1, column=0, sticky='nwes')
		self.bl_frame.columnconfigure(0, weight=1, minsize=self.window_cols[0]/2)
		self.bl_frame.columnconfigure(1, weight=1, minsize=self.window_cols[0]/2)
		self.bl_frame.rowconfigure(0, weight=1, minsize=self.window_rows[1])
		self.bl_frame.grid_propagate(False)

		best_frame = tk.Frame(self.bl_frame, width=sum(self.sub_cols), height=sum(self.sub_rows), relief=tk.SUNKEN, bd=2, bg='white')
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

		latest_frame = tk.Frame(self.bl_frame, width=sum(self.sub_cols), height=sum(self.sub_rows), relief=tk.SUNKEN, bd=2, bg='white')
		latest_frame.grid(row=0, column=1, sticky='nwes')
		latest_frame.columnconfigure(0, weight=1, minsize=self.sub_cols[0])
		latest_frame.rowconfigure(0, weight=1, minsize=self.sub_rows[0])
		latest_frame.rowconfigure(1, weight=1, minsize=self.sub_rows[1])
		latest_frame.rowconfigure(2, weight=1, minsize=self.sub_rows[2])
		latest_tframe = tk.Frame(latest_frame, width=self.sub_cols[0], height=self.sub_rows[1], bg='white')
		latest_tframe.grid(row=1, column=0, sticky='nwes')
		latest_bframe = tk.Frame(latest_frame, width=self.sub_cols[0], height=self.sub_rows[2], bg='white')
		latest_bframe.grid(row=2, column=0, sticky='nwes')
		
		latest_frame.grid_propagate(False)
		latest_tframe.grid_propagate(False)
		latest_bframe.grid_propagate(False)

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

		# setup header
		module_label = tk.Label(master = self.header_frame, text = "Module ID: {0}".format(config.review_module_id), font=("Helvetica", 25, 'bold'))
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

		# setup best result
		best_title = tk.Label(master=best_frame, text="Best result", font=("Helvetica", 25),bg='white')
		best_title.grid(row=0, column=0, sticky='n')
		
		best_glabel = tk.Label(master=best_bframe, text="{0}%".format(best_result[5]), font=("Helvetica", 45, 'bold'), bg='white')
		best_glabel.pack(anchor='ne', side=tk.RIGHT, expand=False)

		best_tlabel = tk.Label(master=best_bframe, text="{0}".format(best_result[3]), font=("Helvetica", 20), bg='white')
		best_tlabel.pack(anchor='nw', side=tk.TOP)

		best_dlabel = tk.Label(master=best_bframe, text="{0}".format(best_result[4]), font=("Helvetica", 20), bg='white')
		best_dlabel.pack(anchor='nw', side=tk.TOP)

		best_ulabel = tk.Label(master=best_bframe, text="{0}".format(best_result[2]), font=("Helvetica", 15), bg='white')
		best_ulabel.pack(anchor='nw', side=tk.TOP)

		#FIXME: automate plot retrieval
		best_canvas = tk.Canvas(best_tframe, bg='white')
		best_canvas.pack(expand=True, fill='both', anchor='nw', side=tk.TOP)
		best_img = Image.open("test_plots/test_best1.png")
		best_img = best_img.resize((self.sub_cols[0], self.sub_rows[1]), Image.ANTIALIAS)
		config.review_best_plot = ImageTk.PhotoImage(best_img)
		best_canvas.create_image(0, 0, image=config.review_best_plot, anchor="nw")

		# setup latest result
		latest_title = tk.Label(master=latest_frame, text="Latest result", font=("Helvetica", 25), bg='white')
		latest_title.grid(row=0, column=0, sticky='n')
		
		# FIXME: automate plot retrieval
		latest_canvas = tk.Canvas(latest_tframe, bg='white')
		latest_canvas.pack(expand=True, fill='both', anchor='nw', side=tk.TOP)
		latest_img = Image.open("test_plots/test_latest1.png")
		latest_img = latest_img.resize((self.sub_cols[0], self.sub_rows[1]), Image.ANTIALIAS)
		config.review_latest_plot = ImageTk.PhotoImage(latest_img)
		latest_canvas.create_image(0, 0, image=config.review_latest_plot, anchor="nw")

		latest_glabel = tk.Label(master=latest_bframe, text="{0}%".format(latest_result[5]), font=("Helvetica", 40, 'bold'), bg='white')
		latest_glabel.pack(anchor='ne', side=tk.RIGHT, expand=False)

		latest_tlabel = tk.Label(master=latest_bframe, text="{0}".format(latest_result[3]), font=("Helvetica", 20), bg='white')
		latest_tlabel.pack(anchor='nw', side=tk.TOP)

		latest_dlabel = tk.Label(master=latest_bframe, text="{0}".format(latest_result[4]), font=("Helvetica", 20), bg='white')
		latest_dlabel.pack(anchor='nw', side=tk.TOP)

		latest_ulabel = tk.Label(master=latest_bframe, text="{0}".format(latest_result[2]), font=("Helvetica", 15), bg='white')
		latest_ulabel.pack(anchor='nw', side=tk.TOP)

		# setup table 
		config.module_results = database.retrieveModuleTests(config.review_module_id)
		table_label = tk.Label(master=table_tframe, text = "All results for module {0}".format(config.review_module_id), font=("Helvetica", 20))
		table_label.pack(side=tk.TOP, anchor='nw')

		self.table_num = tk.Label(master=table_tframe, text = "(Showing {0} results)".format(len(config.module_results)), font=("Helvetica", 12))
		self.table_num.grid(row=0, column=3, sticky='sw')

		sort_button = ttk.Button(
			master = table_tframe,
			text = "Sort by", 
			command = self.sort_results,
		)
		sort_button.grid(row=1, column=0, sticky='nsew')

		sort_menu = tk.OptionMenu(table_tframe, config.review_sort_attr, *['date', 'user', 'grade', 'testname'])
		sort_menu.grid(row=1, column=1, sticky='nsew')

		dir_menu = tk.OptionMenu(table_tframe, config.review_sort_dir, *["increasing", "decreasing"])
		dir_menu.grid(row=1, column=2, sticky='nsw')

		filter_button = ttk.Button(
			master = table_tframe,
			text = "Filter", 
			command = self.filter_results,
		)
		filter_button.grid(row=1, column=3, sticky='nsew', padx=(0, 0))

		filter_menu = tk.OptionMenu(table_tframe, config.review_filter_attr, *['date', 'user', 'grade', 'testname'])
		filter_menu.grid(row=1, column=4, sticky='nsw')

		eq_menu = tk.OptionMenu(table_tframe, config.review_filter_eq, *["=", ">=", "<=", ">", "<", "contains"])
		eq_menu.grid(row=1, column=5, sticky='nsw')

		self.filter_entry = tk.Entry(master=table_tframe)
		self.filter_entry.insert(0, '{0}'.format(config.current_user))
		self.filter_entry.grid(row=1, column=6, sticky='nsw')

		reset_button = ttk.Button(
			master = table_tframe,
			text = "Reset", 
			command = self.reset_results,
		)
		reset_button.grid(row=1, column=7, sticky='nse')

		yscrollbar = tk.Scrollbar(table_bframe)
		yscrollbar.grid(row=0, column=1, sticky='ns')

		self.table_canvas = tk.Canvas(table_bframe, yscrollcommand=yscrollbar.set)
		self.table_canvas.grid(row=0, column=0, sticky='nsew')
		yscrollbar.config(command=self.table_canvas.yview)

		self.make_table()


	def filter_results(self):
		filter_option = config.review_filter_attr.get()
		filter_eq = config.review_filter_eq.get()
		filter_val = self.filter_entry.get()

		config.module_results = database.retrieveModuleTests(config.review_module_id)
		filtered_results = []

		if config.module_results[0][0] == "":
			config.module_results.pop(0)

		options_dict = {"user": 2, "testname": 3, "date": 4, "grade": 5}
		if filter_eq == 'contains':
			if filter_option == 'grade':
				ErrorWindow('Error: Option not supported for chosen variable')
				return 

			filtered_results = [r for r in config.module_results if filter_val in r[options_dict[filter_option]]]
		else:
			op_dict = {
					'=': operator.eq, '>=': operator.ge, '>': operator.gt,
					'<=': operator.le, '<': operator.lt
					}	

			if filter_option in ["user", "testname", "date"]:
				if filter_eq != '=':
					ErrorWindow('Error: Option not supported for chosen variable')
					return
				else: 
					filtered_results = [r for r in config.module_results if op_dict[filter_eq](r[options_dict[filter_option]], filter_val)]
			else:
				try:
					filter_val = int(filter_val)
				except:
					ErrorWindow('Error: Cannot filter a string with an numeric operation')
					return

				filtered_results = [r for r in config.module_results if op_dict[filter_eq](int(r[options_dict[filter_option]]), filter_val)]
		
		self.table_canvas.delete('all')

		if len(filtered_results) == 0:
			error_label = tk.Label(master=table_canvas, text="No results found", font=("Helvetica", 15))
			self.table_canvas.create_window(10, 10, window=error_label, anchor="nw")
			self.table_num['text'] = "(Showing 0 results)"
			return
		
		config.module_results = filtered_results
		self.make_table()


	def reset_results(self):
		config.module_results = database.retrieveModuleTests(config.review_module_id)
		self.make_table()


	def back_to_module(self):
		self.rheader_frame.grid_remove()
		self.rplot_frame.grid_remove()
		self.header_frame.grid()
		self.table_frame.grid()
		self.bl_frame.grid()


	def open_result(self, result_rowid):
		self.table_frame.grid_remove()
		self.bl_frame.grid_remove()
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

		result_tuple = database.retrieveModuleTest(result_rowid)[0]
		
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

		config.plot_images = []

		for row in range(len(test_plots)):
			config.plot_images.append([])

			for col in range(len(test_plots[row])):
				plot_img = Image.open(test_plots[row][col])
				plot_img = plot_img.resize((self.sub_cols[0], self.sub_rows[1]), Image.ANTIALIAS)
				config.plot_images[row].append(ImageTk.PhotoImage(plot_img))

		for row in range(len(test_plots)):
			for col in range(len(test_plots[row])):
				plot_canvas.create_image(col*(self.window_width/ncols), row*((self.window_height-self.window_rows[0])/nrows),
						image=config.plot_images[row][col], anchor="nw")
		

	def make_table(self):
		self.table_canvas.delete('all')
		self.table_num['text'] = "(Showing {0} results)".format(len(config.module_results))

		if config.module_results[0][0] != "":
			config.module_results = [["", "", "User", "Test", "Date", "Grade"]] + config.module_results

		n_cols = 4
		row_ids = [r[0] for r in config.module_results]

		for j, result in enumerate(config.module_results):

			if j != 0:
				result_button = tk.Button(
					master = self.table_canvas,
					text = "{0}".format(j),
					command = partial(self.open_result, row_ids[j])
				)
				
				self.table_canvas.create_window(0*self.table_cols[0]/n_cols, j*25, window=result_button, anchor='nw')


			for i, item in enumerate(result):
				if i in [0,1]: continue

				bold = ''
				if j == 0: bold = 'bold'
				row_label = tk.Label(master=self.table_canvas, text=str(item), font=("Helvetica", 15, bold))

				anchor = 'nw'
				if i == 5: anchor = 'ne'
				self.table_canvas.create_window(((i-2)*self.table_cols[0]/n_cols)+65, j*25, window=row_label, anchor=anchor)

		self.table_canvas.config(scrollregion=(0,0,1000, (len(config.module_results)+2)*25))


	def sort_results(self):
		sort_option = config.review_sort_attr.get()
		options_dict = {"user": 2, "testname": 3, "grade": 5}

		if config.module_results[0][0] == "":
			config.module_results.pop(0)

		if sort_option != "date":
			self.table_canvas.delete('all')
			config.module_results.sort(key=lambda x: x[options_dict[sort_option]], reverse=config.review_sort_dir.get()=="increasing")
			self.make_table()

		else:
			config.module_results.sort(key=lambda r: datetime.strptime(r[4], "%d/%m/%Y %H:%M:%S"), reverse=config.review_sort_dir.get()=="increasing")
			self.make_table()


	def try_change_module(self):
		try:
			config.review_module_id = int(self.change_entry.get())
		except:
			ErrorWindow("Error: Please enter valid module ID")
			return

		try:
			self.destroy()
			ReviewModuleWindow()
		except:
			ErrorWindow("Error: Could not change modules")
	

	def try_open_continue(self):
		try:
			config.current_module_id = config.review_module_id
			ContinueWindow()
			return
		except:
			ErrorWindow("Error: Could not open continue window")

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




