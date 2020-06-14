'''
  gui.py
  \brief                 Functions for interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import ROOT as r
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
		self.register_tag("30", "Black", "White")
		self.reset_to_default_attribs()

	def reset_to_default_attribs(self):
		self.tag = '30'
		self.bright = 'bright'
		self.foregroundcolor = 'Black'
		self.backgroundcolor = 'White'

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
					self.tag = '30' # black
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

def saveTest():
	if config.current_test_grade < 0:
		displayErrorWindow("Error: Finish test before saving")
		return 

	try:
		current_date = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
		testing_info = (config.new_test_moduleID, config.current_user, config.new_test_name.get(), current_date, config.new_test_grade)
		database.createTestEntry(testing_info)
	except:
		displayErrorWindow("Error: Could not save")
		return

##########################################################################
##########################################################################

def openRunWindow():

	def abortTest():
		abort_window = tk.Toplevel(config.root)
		abort_window.title("Abort test")
		abort_window.geometry("500x100")

		warning_label = tk.Label(master=abort_window, text="Are you sure you want to abort? (Cannot be undone)", font=("Helvetica", 20))
		warning_label.pack()

		def abortRun():
			run_window.destroy()
			abort_window.destroy()

		def goBack():
			abort_window.destroy()

		abort_button_a = ttk.Button(
			master = abort_window,
			text = "Abort",
			command = abortRun
		)
		abort_button_a.pack()

		cancel_button = ttk.Button(
			master = abort_window,
			text = "Cancel",
			command = goBack
		)
		cancel_button.pack()

	def runTest():

		def readerThread(q):
			try:
				with run_process.stdout as pipe:
					for line in iter(pipe.readline, b''):
						q.put(line)
				with run_process.stderr as pipe:
					for line in iter(pipe.readline, b''):
						q.put(line)
			finally:
				q.put(None)

		def quit():
			runProcess.kill()
			runWindow.destroy()

		def update(q):
			for line in iter_except(q.get_nowait, Empty):
				if line is None:
					return 
				else:
					console_text.write(line.decode('utf-8'))
					break
			config.root.after(40, update, q)

		run_process = Popen(['python', 'runTest_wrap.py'], stdout=PIPE, stderr=PIPE)
		q = Queue(maxsize=1024)
		t = Thread(target=readerThread, args=[q])
		t.daemon = True
		t.start()
		update(q)

		#FIXME: automate plot retrieval 
		# retrieveResultPlot('', 'Run000000_PixelAlive.root', 'Detector/Board_0/Module_0/Chip_0/D_B(0)_M(0)_PixelAlive_Chip(0)', 'testing1.png')

		re_img = Image.open("testing1.png")
		config.current_test_plot = ImageTk.PhotoImage(re_img)
		plot_label.grid_remove()
		plot_canvas.create_image(0, 0, image=config.current_test_plot, anchor="nw")

	# setup root window
	run_window = tk.Toplevel(config.root)
	run_window.title("Grading Run")
	run_window.geometry("1200x800")
	run_window.rowconfigure(0, weight=1, minsize=800)
	run_window.columnconfigure(0, weight=1, minsize=400)
	run_window.columnconfigure(1, weight=1, minsize=600)

	# setup left side
	left_width = 500
	left_frame = tk.Frame(run_window, width=left_width, height=800)
	left_frame.grid(row=0, column=0, sticky='nsew')
	left_frame.columnconfigure(0, weight=1, minsize=left_width)
	left_frame.rowconfigure(0, weight=1, minsize=200)
	left_frame.rowconfigure(1, weight=1, minsize=600)
	left_frame.grid_propagate(False)

	info_frame = tk.Frame(left_frame, width=left_width, height=200)
	info_frame.grid(row=0, column=0, sticky='nsew')
	
	console_frame = tk.Frame(left_frame, width=left_width, height=600, relief=tk.GROOVE, bd=2)
	console_frame.grid(row=1, column=0, sticky='nsew')

	test_label = tk.Label(master=info_frame, text=config.current_test_name.get(), font=("Helvetica", 25, "bold"))
	test_label.pack(anchor='nw', side=tk.TOP)

	module_label = tk.Label(master=info_frame, text="Module ID: {0}".format(config.current_module_id), font=("Helvetica", 25))
	module_label.pack(anchor='nw', side=tk.TOP)

	test_num_label = tk.Label(master=info_frame, text="Test #: {0}".format(config.current_test_num), font=("Helvetica", 15))
	test_num_label.pack(anchor='nw', side=tk.TOP)

	start_button = ttk.Button(
		master = info_frame,
		text = "Start",
		command = runTest
	)
	start_button.place(x=0, y=100)

	abort_button = ttk.Button(
		master = info_frame,
		text = "Abort",
		command = abortTest
	)
	abort_button.place(x=120, y=100)

	save_button = ttk.Button(
		master = info_frame,
		text = "Save",
		command = saveTest
	)
	save_button.place(x=240, y=100)

	scrollbar = tk.Scrollbar(console_frame)
	scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
	console_text = AnsiColorText(console_frame, yscrollcommand=scrollbar.set)
	console_text.config(yscrollcommand=scrollbar.set)
	scrollbar.config(command=console_text.yview)
	console_text.pack(expand=True, fill='both')

	# setup right side
	right_width = 700
	right_frame = tk.Frame(run_window, width=right_width, height=800)
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

	grade_grade = tk.Label(master=grade_frame, text="{0}%".format(config.current_test_grade), font=("Helvetica", 45, "bold"))
	grade_grade.pack(anchor='sw', side=tk.BOTTOM)

	grade_label = tk.Label(master=grade_frame, text="Grade", font=("Helvetica", 25))
	grade_label.pack(anchor='sw', side=tk.BOTTOM)

	plot_label = tk.Label(master=plot_frame, text='Plot will appear here', font=("Helvetica", 20))
	plot_label.grid(row=0, column=0)

	plot_canvas = tk.Canvas(plot_frame)
	plot_canvas.pack(expand=True, fill='both', anchor='ne', side=tk.TOP)
	
	def goToReviewModule():
		config.review_module_id = config.current_module_id
		openReviewModuleWindow()
		return 

	review_button = ttk.Button(
		master = footer_frame,
		text = "Review Module {0}".format(config.current_module_id),
		command = goToReviewModule
	)
	review_button.pack()

##########################################################################
##########################################################################

def openCreateWindow():
	#FIXME: check if this window is needed

    if config.current_user == '':
        displayErrorWindow("Error: Please login")
        return

    create_window = tk.Toplevel(config.root)
    create_window.title("Create Test")
    create_window.geometry("1000x500")

    # FIXME: temporary labels until create test capacity determined
    create_tmp_lbl = tk.Label(master = create_window, text = "Create test options will appear in this window", font=("Helvetica", 20))
    create_tmp_lbl.pack()

##########################################################################
##########################################################################

def openResultsWindow():
	#FIXME: do we want this window?
	
    if config.current_user == '':
        displayErrorWindow("Error: Please login")
        return

    results_window = tk.Toplevel(config.root)
    results_window.title("Review Results")
    results_window.geometry("500x500")

    results = database.retrieveAllTestTasks()

    nRows = len(results)
    nColumns = 5

    if nRows == 0:
        results_lbl = tk.Label(master = results_window, text = "No results to show", font=("Helvetica", 20))
        results_lbl.pack()
        return()
  
    scrollbar = tk.Scrollbar(results_window)
    scrollbar.pack(side=tk.RIGHT, fill=tk.BOTH)
    
    resultsBox = tk.Listbox(results_window)
    resultsBox.pack(fill=tk.BOTH, expand=True)

    # create result table
    for result in results:
        resultsBox.insert(tk.END, result)

    resultsBox.config(yscrollcommand=scrollbar.set)
    scrollbar.config(command=resultsBox.yview)

##########################################################################
##########################################################################

def displayErrorWindow(errorText):
    errorWindow = tk.Toplevel(config.root)
    errorWindow.title("ERROR")
    errorWindow.geometry("600x50")
    error_label = tk.Label(master = errorWindow, text = errorText, font=("Helvetica", 15))
    error_label.pack(expand=True, fill=tk.BOTH)

##########################################################################
##########################################################################

def openStartWindow():
	if config.current_user == '':
		displayErrorWindow("Error: Please login")
		return

	start_window = tk.Toplevel(config.root)
	start_window.title('New test')
	start_window.geometry("500x200")

	for row in range(1,5):
		start_window.rowconfigure(row, weight=1, minsize=40)
	start_window.rowconfigure(0, weight=1, minsize=20)
	start_window.rowconfigure(5, weight=1, minsize=20)
	start_window.columnconfigure(1, weight=1, minsize=200)
	start_window.columnconfigure(2, weight=1, minsize=200)
	start_window.columnconfigure(0, weight=1, minsize=50)
	start_window.columnconfigure(3, weight=1, minsize=50)

	start_label = tk.Label(master=start_window, text="Start a new test", font=("Helvetica", 20, 'bold'))
	start_label.grid(row=1, column=1, columnspan=2, sticky="wen")

	moduleID_label = tk.Label(master=start_window, text="Module ID", font=("Helvetica", 15))
	moduleID_label.grid(row=2, column=1, sticky='w')

	moduleID_entry = tk.Entry(master=start_window)
	moduleID_entry.grid(row=2, column=2, sticky='e')

	test_mode_label = tk.Label(master=start_window, text="Test mode", font=("Helvetica", 15))
	test_mode_label.grid(row=3, column=1, sticky='w')

	current_modes = []
	for mode in list(database.retrieveAllModes()):
		current_modes.append(mode[1])

	mode_menu = tk.OptionMenu(start_window, config.current_test_name, *current_modes)
	mode_menu.grid(row=3, column=2, sticky='e')

	def tryOpenRunWindow():
		try:
			config.current_module_id = int(moduleID_entry.get())
		except:
			displayErrorWindow("Error: Please enter valid module ID")
			return
		openRunWindow()
		start_window.destroy()

	start_button = ttk.Button(
		master = start_window,
		text = "Start test", 
		command = tryOpenRunWindow
	)
	start_button.grid(row=4, column=1, columnspan=2)

##########################################################################
##########################################################################

def openContinueWindow():
	if config.current_user == '':
		displayErrorWindow("Error: Please login")
		return

	start_window = tk.Toplevel(config.root)
	start_window.title('Continue testing')
	start_window.geometry("500x200")

	for row in range(1,5):
		start_window.rowconfigure(row, weight=1, minsize=40)
	start_window.rowconfigure(0, weight=1, minsize=20)
	start_window.rowconfigure(5, weight=1, minsize=20)
	start_window.columnconfigure(1, weight=1, minsize=200)
	start_window.columnconfigure(2, weight=1, minsize=200)
	start_window.columnconfigure(0, weight=1, minsize=50)
	start_window.columnconfigure(3, weight=1, minsize=50)

	start_label = tk.Label(master = start_window, text = "Start a new test", font=("Helvetica", 20, 'bold'))
	start_label.grid(row=1, column=1, columnspan=2, sticky="wen")

	moduleID_label = tk.Label(master = start_window, text = "Module ID", font=("Helvetica", 15))
	moduleID_label.grid(row=2, column=1, sticky='w')

	moduleID_entry = tk.Entry(master = start_window)
	moduleID_entry.grid(row=2, column=2, sticky='e')
	moduleID_entry.insert(0, str(config.current_module_id))

	test_mode_label = tk.Label(master = start_window, text = "Test mode", font=("Helvetica", 15))
	test_mode_label.grid(row=3, column=1, sticky='w')

	currentModes = []
	for mode in list(database.retrieveAllModes()):
		currentModes.append(mode[1])

	mode_menu = tk.OptionMenu(start_window, config.current_test_name, *currentModes)
	mode_menu.grid(row=3, column=2, sticky='e')

	def tryOpenRunWindow():
		try:
			config.current_module_id = int(moduleID_entry.get())
		except:
			displayErrorWindow("Error: Please enter valid module ID")
			return
		openRunWindow()
		start_window.destroy()

	start_button = ttk.Button(
		master = start_window,
		text = "Start test", 
		command = tryOpenRunWindow
	)
	start_button.grid(row=4, column=1, columnspan=2)

##########################################################################
##########################################################################

def openReviewModuleWindow():
	if config.current_user == '':
		displayErrorWindow("Error: Please login")
		return
	elif config.review_module_id == -1:
		displayErrorWindow("Error: Please enter module ID")
		return

	config.module_results = database.retrieveModuleTests(config.review_module_id)
	if len(config.module_results) == 0:
		displayErrorWindow("Error: No results found for this module, start a new test")
		config.current_module_id = config.review_module_id 
		openContinueWindow()
		return 

	best_result = config.module_results[0]
	latest_result = config.module_results[0]

	for result in config.module_results:
		if result[5] > best_result[5]: 
			best_result = result

		if selectEarly(latest_result[4], result[4]):
			latest_result = result
	
	window_width = 1000
	window_height = 850
	window_rows = [50, 500, 300]
	window_cols = [window_width]

	sub_rows = [50, 350, 100]
	sub_cols = [int(window_width/2)]

	table_rows = [75, 225]
	table_cols = [window_width-15, 15]

	tableb_rows = [29, 23, 23]
	tableb_cols = [125, 125, 125, 125, 125, 125, 125, 125]

	assert sum(window_rows) == window_height, 'Check height'
	assert sum(window_cols) == window_width, 'Check width'
	assert window_rows[1] == sum(sub_rows), 'Check bl subview'
	assert window_rows[2] == sum(table_rows), 'Check table subview'
	assert sum(tableb_rows) == table_rows[0], 'Check tableb rows'
	assert sum(tableb_cols) == window_width, 'Check tableb cols'

	module_review_window = tk.Toplevel(config.root, bg='white')
	module_review_window.title('Review Module ID: {0}'.format(config.review_module_id))
	module_review_window.geometry("{0}x{1}".format(window_width, window_height))
	module_review_window.rowconfigure(0, weight=1, minsize=window_rows[0])
	module_review_window.rowconfigure(1, weight=1, minsize=window_rows[1])
	module_review_window.rowconfigure(2, weight=1, minsize=window_rows[2])
	module_review_window.columnconfigure(0, weight=1, minsize=window_cols[0])

	# setup base frames
	header_frame = tk.Frame(module_review_window, width=window_cols[0], height=window_rows[0])
	header_frame.grid(row=0, column=0, sticky='nwes')
	header_frame.grid_propagate(False)

	bl_frame = tk.Frame(module_review_window, width=window_cols[0], height=window_rows[1], bg='white')
	bl_frame.grid(row=1, column=0, sticky='nwes')
	bl_frame.columnconfigure(0, weight=1, minsize=window_cols[0]/2)
	bl_frame.columnconfigure(1, weight=1, minsize=window_cols[0]/2)
	bl_frame.rowconfigure(0, weight=1, minsize=window_rows[1])
	bl_frame.grid_propagate(False)

	best_frame = tk.Frame(bl_frame, width=sum(sub_cols), height=sum(sub_rows), relief=tk.SUNKEN, bd=2, bg='white')
	best_frame.grid(row=0, column=0, sticky='nwes')
	best_frame.columnconfigure(0, weight=1, minsize=sub_cols[0])
	best_frame.rowconfigure(0, weight=1, minsize=sub_rows[0])
	best_frame.rowconfigure(1, weight=1, minsize=sub_rows[1])
	best_frame.rowconfigure(2, weight=1, minsize=sub_rows[2])
	best_tframe = tk.Frame(best_frame, width=sub_cols[0], height=sub_rows[1], bg='white')
	best_tframe.grid(row=1, column=0, sticky='nwes')
	best_bframe = tk.Frame(best_frame, width=sub_cols[0], height=sub_rows[2], bg='white')
	best_bframe.grid(row=2, column=0, sticky='nwes')
	
	best_frame.grid_propagate(False)
	best_tframe.grid_propagate(False)
	best_bframe.grid_propagate(False)

	latest_frame = tk.Frame(bl_frame, width=sum(sub_cols), height=sum(sub_rows), relief=tk.SUNKEN, bd=2, bg='white')
	latest_frame.grid(row=0, column=1, sticky='nwes')
	latest_frame.columnconfigure(0, weight=1, minsize=sub_cols[0])
	latest_frame.rowconfigure(0, weight=1, minsize=sub_rows[0])
	latest_frame.rowconfigure(1, weight=1, minsize=sub_rows[1])
	latest_frame.rowconfigure(2, weight=1, minsize=sub_rows[2])
	latest_tframe = tk.Frame(latest_frame, width=sub_cols[0], height=sub_rows[1], bg='white')
	latest_tframe.grid(row=1, column=0, sticky='nwes')
	latest_bframe = tk.Frame(latest_frame, width=sub_cols[0], height=sub_rows[2], bg='white')
	latest_bframe.grid(row=2, column=0, sticky='nwes')
	
	latest_frame.grid_propagate(False)
	latest_tframe.grid_propagate(False)
	latest_bframe.grid_propagate(False)

	table_frame = tk.Frame(module_review_window, width=window_width, height=window_rows[2], relief=tk.SUNKEN, bd=2)
	table_frame.grid(row=2, column=0, sticky='nwes')
	table_frame.grid_propagate(False)

	table_frame.columnconfigure(0, weight=1, minsize=window_width)
	table_frame.rowconfigure(0, weight=1, minsize=table_rows[0])
	table_frame.rowconfigure(1, weight=1, minsize=table_rows[1])
	table_tframe = tk.Frame(table_frame, width=window_width, height=table_rows[0])
	table_tframe.grid(row=0, column=0, sticky='nwes')
	for row in range(3):
		table_tframe.grid_rowconfigure(row, weight=1, minsize=tableb_rows[row])
	for col in range(len(tableb_cols)):
		table_tframe.grid_columnconfigure(row, weight=1, minsize=tableb_cols[col])


	table_bframe = tk.Frame(table_frame, width=window_width, height=table_rows[1])
	table_bframe.grid(row=1, column=0, sticky='nwes')
	table_bframe.rowconfigure(0, weight=1, minsize=table_rows[1])
	table_bframe.columnconfigure(0, weight=1, minsize=table_cols[0])
	table_bframe.columnconfigure(1, weight=1, minsize=table_cols[1])

	table_frame.grid_propagate(False)
	table_tframe.grid_propagate(False)
	table_bframe.grid_propagate(False)

	def tryChangeModule():
		try:
			config.review_module_id = int(change_entry.get())
		except:
			displayErrorWindow("Error: Please enter valid module ID")
			return

		try:
			module_review_window.destroy()
			openReviewModuleWindow()
		except:
			displayErrorWindow("Error: Could not change modules")

	# setup header
	module_label = tk.Label(master = header_frame, text = "Module ID: {0}".format(config.review_module_id), font=("Helvetica", 25, 'bold'))
	module_label.pack(anchor='ne', side=tk.LEFT)

	change_entry = tk.Entry(master=header_frame)
	change_entry.pack(anchor='ne', side=tk.RIGHT)

	change_button = ttk.Button(
		master = header_frame,
		text = "Change module:",
		command = tryChangeModule
	)
	change_button.pack(side=tk.RIGHT, anchor='ne')

	def tryOpenContinue():
		try:
			config.current_module_id = config.review_module_id
			openContinueWindow()
			return
		except:
			displayErrorWindow("Error: Could not open continue window")

	continue_button = ttk.Button(
		master = header_frame,
		text = "Continue Testing", 
		command = tryOpenContinue,
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
	best_img = Image.open("test_best1.png")
	best_img = best_img.resize((sub_cols[0], sub_rows[1]), Image.ANTIALIAS)
	config.review_best_plot = ImageTk.PhotoImage(best_img)
	best_canvas.create_image(0, 0, image=config.review_best_plot, anchor="nw")

	# setup latest result
	latest_title = tk.Label(master=latest_frame, text="Latest result", font=("Helvetica", 25), bg='white')
	latest_title.grid(row=0, column=0, sticky='n')
	
	# FIXME: automate plot retrieval
	latest_canvas = tk.Canvas(latest_tframe, bg='white')
	latest_canvas.pack(expand=True, fill='both', anchor='nw', side=tk.TOP)
	latest_img = Image.open("test_latest1.png")
	latest_img = latest_img.resize((sub_cols[0], sub_rows[1]), Image.ANTIALIAS)
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

	table_num = tk.Label(master=table_tframe, text = "(Showing {0} results)".format(len(config.module_results)), font=("Helvetica", 12))
	table_num.grid(row=0, column=3, sticky='sw')

	def sortResults():
		sort_option = config.review_sort_attr.get()
		options_dict = {"user": 2, "testname": 3, "grade": 5}

		if config.module_results[0][0] == "":
			config.module_results.pop(0)

		if sort_option != "date":
			table_canvas.delete('all')
			config.module_results.sort(key=lambda x: x[options_dict[sort_option]], reverse=config.review_sort_dir.get()=="increasing")
			makeTable()

		#FIXME: implement date sort

	def filterResults():
		filter_option = config.review_filter_attr.get()
		filter_eq = config.review_filter_eq.get()
		filter_val = filter_entry.get()

		config.module_results = database.retrieveModuleTests(config.review_module_id)
		filtered_results = []

		if config.module_results[0][0] == "":
			config.module_results.pop(0)

		options_dict = {"user": 2, "testname": 3, "date": 4, "grade": 5}
		if filter_eq == 'contains':
			if filter_option == 'grade':
				displayErrorWindow('Error: Option not supported for chosen variable')
				return 

			filtered_results = [r for r in config.module_results if filter_val in r[options_dict[filter_option]]]
		else:
			op_dict = {
					'=': operator.eq, '>=': operator.ge, '>': operator.gt,
					'<=': operator.le, '<': operator.lt
					}	

			if filter_option in ["user", "testname", "date"]:
				if filter_eq != '=':
					displayErrorWindow('Error: Option not supported for chosen variable')
					return
				else: 
					filtered_results = [r for r in config.module_results if op_dict[filter_eq](r[options_dict[filter_option]], filter_val)]
			else:
				try:
					filter_val = int(filter_val)
				except:
					displayErrorWindow('Error: Cannot filter a string with an numeric operation')
					return

				filtered_results = [r for r in config.module_results if op_dict[filter_eq](int(r[options_dict[filter_option]]), filter_val)]
		
		table_canvas.delete('all')

		if len(filtered_results) == 0:
			error_label = tk.Label(master=table_canvas, text="No results found", font=("Helvetica", 15))
			table_canvas.create_window(10, 10, window=error_label, anchor="nw")
			table_num['text'] = "(Showing 0 results)"
			return
		
		config.module_results = filtered_results
		
		makeTable()


	def resetResults():
		config.module_results = database.retrieveModuleTests(config.review_module_id)
		makeTable()

	sort_button = ttk.Button(
		master = table_tframe,
		text = "Sort by", 
		command = sortResults,
	)
	sort_button.grid(row=1, column=0, sticky='nsew')

	sort_menu = tk.OptionMenu(table_tframe, config.review_sort_attr, *['date', 'user', 'grade', 'testname'])
	sort_menu.grid(row=1, column=1, sticky='nsew')

	dir_menu = tk.OptionMenu(table_tframe, config.review_sort_dir, *["increasing", "decreasing"])
	dir_menu.grid(row=1, column=2, sticky='nsw')

	filter_button = ttk.Button(
		master = table_tframe,
		text = "Filter", 
		command = filterResults,
	)
	filter_button.grid(row=1, column=3, sticky='nsew', padx=(0, 0))

	filter_menu = tk.OptionMenu(table_tframe, config.review_filter_attr, *['date', 'user', 'grade', 'testname'])
	filter_menu.grid(row=1, column=4, sticky='nsw')

	eq_menu = tk.OptionMenu(table_tframe, config.review_filter_eq, *["=", ">=", "<=", ">", "<", "contains"])
	eq_menu.grid(row=1, column=5, sticky='nsw')

	filter_entry = tk.Entry(master=table_tframe)
	filter_entry.insert(0, '{0}'.format(config.current_user))
	filter_entry.grid(row=1, column=6, sticky='nsw')

	reset_button = ttk.Button(
		master = table_tframe,
		text = "Reset", 
		command = resetResults,
	)
	reset_button.grid(row=1, column=7, sticky='nse')

	yscrollbar = tk.Scrollbar(table_bframe)
	yscrollbar.grid(row=0, column=1, sticky='ns')

	table_canvas = tk.Canvas(table_bframe, yscrollcommand=yscrollbar.set)
	table_canvas.grid(row=0, column=0, sticky='nsew')
	yscrollbar.config(command=table_canvas.yview)

	def openResult(result_rowid):
		table_frame.grid_remove()
		bl_frame.grid_remove()
		header_frame.grid_remove()

		def backToModule():
			rheader_frame.grid_remove()
			rplot_frame.grid_remove()
			header_frame.grid()
			table_frame.grid()
			bl_frame.grid()


		rheader_frame = tk.Frame(module_review_window, width=window_cols[0], height=window_rows[0])
		rheader_frame.grid(row=0, column=0, sticky='nwes')
		rheader_frame.grid_propagate(False)

		rplot_frame = tk.Frame(module_review_window, width=window_cols[0], height=window_height-window_rows[0])
		rplot_frame.grid(row=1, column=0, rowspan=2, sticky='nwes')
		rplot_frame.grid_propagate(False)

		rplot_frame.grid_rowconfigure(0, weight=1, minsize=window_height-window_rows[0])
		rplot_frame.grid_columnconfigure(0, weight=1, minsize=window_cols[0]-15)
		rplot_frame.grid_columnconfigure(1, weight=1, minsize=15)

		# setup header
		back_image = Image.open('back-arrow.png')
		back_image = back_image.resize((27, 20), Image.ANTIALIAS)
		back_photo = ImageTk.PhotoImage(back_image)

		back_button = ttk.Button(
			master = rheader_frame,
			text = 'Back',
			image = back_photo,	
			command = backToModule
		)
		back_button.image = back_photo
		back_button.pack(side=tk.RIGHT, anchor='ne')

		result_tuple = database.retrieveModuleTest(result_rowid)[0]
		
		name_label = tk.Label(master=rheader_frame, text=result_tuple[3], font=("Helvetica", 25))
		name_label.pack(side=tk.LEFT, anchor='sw')

		grade_label = tk.Label(master=rheader_frame, text='{}%'.format(result_tuple[5]), font=("Helvetica", 30, 'bold'), padx=5)
		grade_label.pack(side=tk.LEFT, anchor='sw')

		date_label = tk.Label(master=rheader_frame, text=result_tuple[4], font=("Helvetica", 18), padx=4)
		date_label.pack(side=tk.LEFT, anchor='sw')

		user_label = tk.Label(master=rheader_frame, text=result_tuple[2], font=("Helvetica", 18))
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

		plot_yscrollbar = tk.Scrollbar(rplot_frame)
		plot_yscrollbar.grid(row=0, column=1, sticky='nsew')

		plot_canvas = tk.Canvas(rplot_frame, scrollregion=(0,0,1000,sub_rows[1]*(nrows+3)), yscrollcommand=plot_yscrollbar.set, bg='white')
		plot_canvas.grid(row=0, column=0, sticky='nsew')
		plot_yscrollbar.config(command=plot_canvas.yview)

		config.plot_images = []

		for row in range(len(test_plots)):
			config.plot_images.append([])

			for col in range(len(test_plots[row])):
				plot_img = Image.open(test_plots[row][col])
				plot_img = plot_img.resize((sub_cols[0], sub_rows[1]), Image.ANTIALIAS)
				config.plot_images[row].append(ImageTk.PhotoImage(plot_img))

		for row in range(len(test_plots)):
			for col in range(len(test_plots[row])):
				plot_canvas.create_image(col*(window_width/ncols), row*((window_height-window_rows[0])/nrows),
						image=config.plot_images[row][col], anchor="nw")
		

	def makeTable():
		table_canvas.delete('all')

		table_num['text'] = "(Showing {0} results)".format(len(config.module_results))

		if config.module_results[0][0] != "":
			config.module_results = [["", "", "User", "Test", "Date", "Grade"]] + config.module_results

		n_cols = 4
		row_ids = [r[0] for r in config.module_results]

		for j, result in enumerate(config.module_results):

			if j != 0:
				result_button = tk.Button(
					master = table_canvas,
					text = "{0}".format(j),
					command = partial(openResult, row_ids[j])
				)
				
				table_canvas.create_window(0*table_cols[0]/n_cols, j*25, window=result_button, anchor='nw')


			for i, item in enumerate(result):
				if i in [0,1]: continue

				bold = ''
				if j == 0: bold = 'bold'
				row_label = tk.Label(master=table_canvas, text=str(item), font=("Helvetica", 15, bold))

				anchor = 'nw'
				if i == 5: anchor = 'ne'
				table_canvas.create_window(((i-2)*table_cols[0]/n_cols)+65, j*25, window=row_label, anchor=anchor)

		table_canvas.config(scrollregion=(0,0,1000, (len(config.module_results)+2)*25))


	makeTable()

##########################################################################
##########################################################################

#FIXME: automate this in runTest
def retrieveResultPlot(result_dir, result_file, plot_file, output_file):
	os.system("root -l -b -q 'extractPlots.cpp(\"{0}\", \"{1}\", \"{2}\")'".format(result_dir+result_file, plot_file, output_file))
	result_image = tk.Image('photo', file=output_file) #FIXME: update to using pillow
	return result_image

##########################################################################
##########################################################################

def selectEarly(date1, date2):
	date1_date = date1.split()[0]
	date2_date = date2.split()[0]

	date1_time = date1.split()[1]
	date2_time = date2.split()[1]

	date1_date_nums = re.findall('\d+', date1_date)
	date2_date_nums = re.findall('\d+', date2_date)

	date1_time_nums = re.findall('\d+', date1_time)
	date2_time_nums = re.findall('\d+', date2_time)

	date1_time_nums = [int(d) for d in date1_time_nums]
	date2_time_nums = [int(d) for d in date2_time_nums]

	for i in range(2,-1,-1):
		if date1_date_nums[i] > date2_date_nums[i]:
			return True
		elif date1_date_nums[i] < date2_date_nums[i]:
			return False
		elif date1_date_nums[i] == date2_date_nums[i]:
			continue

	for i in range(3):
		if date1_time_nums[i] > date2_time_nums[i]:
			return True
		elif date1_time_nums[i] < date2_time_nums[i]:
			return False
		elif date1_time_nums[i] == date2_time_nums[i]:
			continue

	return False