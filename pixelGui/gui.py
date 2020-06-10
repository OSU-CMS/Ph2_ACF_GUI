'''
  gui.py
  \brief                 Functions for interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import config
import database
import sys
import tkinter as tk
import os
from queue import Queue, Empty
from threading import Thread
from tkinter import ttk
from datetime import datetime
from subprocess import Popen, PIPE
from itertools import islice
from textwrap import dedent

def iter_except(function, exception):
	try:
		while True:
			yield function()
	except exception:
		return

##########################################################################
##########################################################################

def saveTest():
	#FIXME: check test is finished running
	currentDate = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
	testingInfo = (config.new_test_moduleID, config.current_user, config.new_test_name.get(), currentDate, config.new_test_grade)

	try:
		database.createTestEntry(testingInfo)
	except:
		displayErrorWindow("Error: could not save")
		return

##########################################################################
##########################################################################

def openRunWindow():
	runWindow = tk.Toplevel(config.root)
	runWindow.title("Grading Run")
	runWindow.geometry("1000x500")

	runWindow.rowconfigure(0, weight=1, minsize=100)
	runWindow.rowconfigure(1, weight=1, minsize=200)
	runWindow.rowconfigure(2, weight=1, minsize=200)
	runWindow.columnconfigure(0, weight=1, minsize=500)

	descFrame = tk.Frame(runWindow, width = 1000, height = 100)
	resultFrame = tk.Frame(runWindow, width = 1000, height = 200, relief=tk.SUNKEN, bd=2)
	consoleFrame = tk.Frame(runWindow, width= 1000, height = 200, relief=tk.SUNKEN, bd=2)

	descFrame.grid(row=0, column=0, sticky="nw")
	resultFrame.grid(row=1, column=0, sticky="we")
	consoleFrame.grid(row=2, column=0, sticky="we")

	descFrame.rowconfigure(0, weight=1, minsize=100)
	descFrame.columnconfigure(0, weight=1, minsize=1000)

	resultFrame.rowconfigure(0, weight=1, minsize=200)
	resultFrame.columnconfigure(0, weight=1, minsize=1000)
	consoleFrame.rowconfigure(0, weight=1, minsize=200)
	consoleFrame.columnconfigure(0, weight=1, minsize=1000)

	descFrame.grid_propagate(False) 
	resultFrame.grid_propagate(False) 
	consoleFrame.grid_propagate(False)

	run_lbl = tk.Label(master = descFrame, text = "Test: {0}".format(config.new_test_name.get()), font=("Cambria", 25))
	run_lbl.place(x=0, y=0)

	user_lbl = tk.Label(master = descFrame, text = "User: {0}".format(config.current_user), font=("Cambria", 15))
	user_lbl.place(x=0, y=33)

	module_label = tk.Label(master = descFrame, text = "Module ID: {0}".format(config.new_test_moduleID), font=("Cambria", 25))
	module_label.place(x=400, y=0)

	def abortTest():
		abortWindow = tk.Toplevel(config.root)
		abortWindow.title("Abort test")
		abortWindow.geometry("500x100")

		warning_label = tk.Label(master = abortWindow, text = "Are you sure you want to abort? (Cannot be undone)", font=("Cambria", 20))
		warning_label.pack()

		def abortRun():
			runWindow.destroy()
			abortWindow.destroy()

		def goBack():
			abortWindow.destroy()

		abort_btn = ttk.Button(
			master = abortWindow,
			text = "Abort Test",
			command = abortRun
		)
		abort_btn.pack()

		cancel_btn = ttk.Button(
			master = abortWindow,
			text = "Cancel",
			command = goBack
		)
		cancel_btn.pack()

	def runTest():

		def reader_thread(q):
			try:
				with runProcess.stdout as pipe:
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
					# quit()
					return 
				else:
					console_text.insert(1.0, line)
					break
			config.root.after(40, update, q)

		runProcess = Popen(['python3', 'runTest_wrap.py'], stdout=PIPE)
		q = Queue(maxsize=1024)
		t = Thread(target=reader_thread, args=[q])
		t.daemon = True
		t.start()
		update(q)

	start_btn = ttk.Button(
		master = descFrame,
		text = "Start Test",
		command = runTest
	)
	start_btn.place(x=0, y=58)

	abort_btn = ttk.Button(
		master = descFrame,
		text = "Abort Test",
		command = abortTest
	)
	abort_btn.place(x=120, y=58)

	save_btn = ttk.Button(
		master = descFrame,
		text = "Save Test",
		command = saveTest
	)
	save_btn.place(x=240, y=58)

	console_text = tk.Text(master=consoleFrame, wrap='word', undo=False)
	console_text.pack(expand=True, fill='both')

	results_tmp_lbl = tk.Label(master = resultFrame, text = "Results will appear here", font=("Cambria", 20))
	results_tmp_lbl.place(x=0, y=60)

##########################################################################
##########################################################################

def openCreateWindow():
    if config.current_user == '':
        displayErrorWindow("Error: Please login")
        return

    createWindow = tk.Toplevel(config.root)
    createWindow.title("Create Test")
    createWindow.geometry("1000x500")

    # FIXME: temporary labels until create test capacity determined
    create_tmp_lbl = tk.Label(master = createWindow, text = "Create test options will appear in this window", font=("Cambria", 20))
    create_tmp_lbl.pack()

##########################################################################
##########################################################################

def openResultsWindow():
    if config.current_user == '':
        displayErrorWindow("Error: Please login")
        return
    resultsWindow = tk.Toplevel(config.root)
    resultsWindow.title("Review Results")
    resultsWindow.geometry("500x500")

    results = database.retrieveAllTestTasks()

    nRows = len(results)
    nColumns = 5

    if nRows == 0:
        results_lbl = tk.Label(master = resultsWindow, text = "No results to show", font=("Cambria", 20))
        results_lbl.pack()
        return()
  
    # FIXME: add buttons for editing data, make table more pretty

    scrollbar = tk.Scrollbar(resultsWindow)
    scrollbar.pack(side=tk.RIGHT, fill=tk.BOTH)
    
    resultsBox = tk.Listbox(resultsWindow)
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
    errorWindow.geometry("400x50")

    error_label = tk.Label(master = errorWindow, text = errorText, font=("Cambria", 20))
    error_label.pack()

##########################################################################
##########################################################################

def openStartWindow():
	if config.current_user == '':
		displayErrorWindow("Error: Please login")
		return

	startWindow = tk.Toplevel(config.root)
	startWindow.title('New test')
	startWindow.geometry("500x200")

	for row in range(1,5):
		startWindow.rowconfigure(row, weight=1, minsize=40)
	startWindow.rowconfigure(0, weight=1, minsize=20)
	startWindow.rowconfigure(5, weight=1, minsize=20)
	startWindow.columnconfigure(1, weight=1, minsize=200)
	startWindow.columnconfigure(2, weight=1, minsize=200)
	startWindow.columnconfigure(0, weight=1, minsize=50)
	startWindow.columnconfigure(3, weight=1, minsize=50)

	start_label = tk.Label(master = startWindow, text = "Start a new test", font=("Cambria", 20, 'bold'))
	start_label.grid(row=1, column=1, columnspan=2, sticky="wen")

	moduleID_label = tk.Label(master = startWindow, text = "Module ID", font=("Cambria", 15))
	moduleID_label.grid(row=2, column=1, sticky='w')

	moduleID_entry = tk.Entry(master = startWindow, text='')
	moduleID_entry.grid(row=2, column=2, sticky='e')

	test_mode_label = tk.Label(master = startWindow, text = "Test mode", font=("Cambria", 15))
	test_mode_label.grid(row=3, column=1, sticky='w')

	currentModes = []
	for mode in list(database.retrieveAllModes()):
		currentModes.append(mode[1])
	mode_menu = tk.OptionMenu(startWindow, config.new_test_name, *currentModes)
	mode_menu.grid(row=3, column=2, sticky='e')

	def tryOpenRunWindow():
		try:
			config.new_test_moduleID = int(moduleID_entry.get())
		except:
			displayErrorWindow("Error: Please enter valid module ID")
			return
		openRunWindow()
		startWindow.destroy()

	start_button = ttk.Button(
		master = startWindow,
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

	startWindow = tk.Toplevel(config.root)
	startWindow.title('Continue testing')
	startWindow.geometry("500x200")

	for row in range(1,5):
		startWindow.rowconfigure(row, weight=1, minsize=40)
	startWindow.rowconfigure(0, weight=1, minsize=20)
	startWindow.rowconfigure(5, weight=1, minsize=20)
	startWindow.columnconfigure(1, weight=1, minsize=200)
	startWindow.columnconfigure(2, weight=1, minsize=200)
	startWindow.columnconfigure(0, weight=1, minsize=50)
	startWindow.columnconfigure(3, weight=1, minsize=50)

	start_label = tk.Label(master = startWindow, text = "Start a new test", font=("Cambria", 20, 'bold'))
	start_label.grid(row=1, column=1, columnspan=2, sticky="wen")

	moduleID_label = tk.Label(master = startWindow, text = "Module ID", font=("Cambria", 15))
	moduleID_label.grid(row=2, column=1, sticky='w')

	moduleID_entry = tk.Entry(master = startWindow)
	moduleID_entry.grid(row=2, column=2, sticky='e')
	moduleID_entry.insert(0, str(config.review_module_id))

	test_mode_label = tk.Label(master = startWindow, text = "Test mode", font=("Cambria", 15))
	test_mode_label.grid(row=3, column=1, sticky='w')

	currentModes = []
	for mode in list(database.retrieveAllModes()):
		currentModes.append(mode[1])
	mode_menu = tk.OptionMenu(startWindow, config.new_test_name, *currentModes)
	mode_menu.grid(row=3, column=2, sticky='e')

	def tryOpenRunWindow():
		try:
			config.new_test_moduleID = int(moduleID_entry.get())
		except:
			displayErrorWindow("Error: Please enter valid module ID")
			return
		openRunWindow()
		startWindow.destroy()

	start_button = ttk.Button(
		master = startWindow,
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

	moduleResults = database.retrieveModuleTests(config.review_module_id)
	if len(moduleResults) == 0:
		pass 
		# FIXME: add dialog box stating no results and option to start new test

	moduleReviewWindow = tk.Toplevel(config.root)
	moduleReviewWindow.title('Review Module ID: {0}'.format(config.review_module_id))
	moduleReviewWindow.geometry("900x700")
	moduleReviewWindow.rowconfigure(0, weight=1, minsize=100)
	moduleReviewWindow.rowconfigure(1, weight=1, minsize=400)
	moduleReviewWindow.rowconfigure(2, weight=1, minsize=200)
	moduleReviewWindow.columnconfigure(0, weight=1, minsize=900)

	headerFrame = tk.Frame(moduleReviewWindow, width=900, height=100)
	headerFrame.grid(row=0, column=0, sticky='nwe')
	headerFrame.rowconfigure(0, weight=1, minsize=100)
	headerFrame.columnconfigure(0, weight=1, minsize=450)
	headerFrame.columnconfigure(1, weight=1, minsize=450)

	module_label = tk.Label(master = headerFrame, text = "Module ID: {0}".format(config.review_module_id), font=("Cambria", 25, 'bold'))
	module_label.grid(row=0,column=0, sticky='wn')

	continue_button = ttk.Button(
		master = headerFrame, 
		text = "Continue Testing", 
		command = openContinueWindow,
		style='Important.TButton'
	)
	continue_button.grid(row=0, column=1, sticky='n')

##########################################################################
##########################################################################

def openConnectionWindow():
    #FIXME: create window
    print("OPENING connection")