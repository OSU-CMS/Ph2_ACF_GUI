'''
  acfGui.py
  \brief                 Interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/05/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk 
from tkinter import ttk
import config

def abortTest():
    #FIXME: add dialog box confirming abort
    #FIXME: indicate in database that test aborted??
    pass

def openRunWindow():

    runWindow = tk.Toplevel(config.root)
    runWindow.title("Grading Run")
    runWindow.geometry("1000x500")

    if config.testname.get() == "":
        err_lbl = tk.Label(master = runWindow, text = "ERROR: Please enter a valid test name", font=("Cambria", 15))
        err_lbl.pack()
        return
    if config.username_en.get() == "":
        err_lbl = tk.Label(master = runWindow, text = "ERROR: Please enter a valid user name", font=("Cambria", 15))
        err_lbl.pack()
        return

    #FIXME: make database request
    # want to make sure is verified user?? 

    runWindow.rowconfigure(0, weight=1, minsize=100)
    runWindow.rowconfigure(1, weight=1, minsize=200)
    runWindow.rowconfigure(2, weight=1, minsize=200)
    runWindow.columnconfigure(0, weight=1, minsize=500)

    descFrame = tk.Frame(runWindow, width = 500, height = 100)
    resultFrame = tk.Frame(runWindow, width = 495, height = 300, relief=tk.SUNKEN, bd=2)
    consoleFrame = tk.Frame(runWindow, width=495, height=200, relief=tk.SUNKEN, bd=2)

    descFrame.grid(row=0, column=0, sticky="nw")
    resultFrame.grid(row=1, column=0, sticky="we")
    consoleFrame.grid(row=2, column=0, sticky="we")

    descFrame.rowconfigure(0, weight=1, minsize=50)
    descFrame.columnconfigure(0, weight=1, minsize=495)

    resultFrame.rowconfigure(0, weight=1, minsize=100)
    consoleFrame.rowconfigure(0, weight=1, minsize=100)
   
    descFrame.grid_propagate(False) 
    resultFrame.grid_propagate(False) 
    consoleFrame.grid_propagate(False)

    run_lbl = tk.Label(master = descFrame, text = "Test: {0}".format(config.testname.get()), font=("Cambria", 25))
    run_lbl.place(x=0, y=0)

    user_lbl = tk.Label(master = descFrame, text = "User: {0}".format(config.username_en.get()), font=("Cambria", 15))
    user_lbl.place(x=0, y=33)

    abort_btn = ttk.Button(
        master = descFrame,
        text = "Abort Test",
        command = abortTest
    )
    abort_btn.place(x=0, y=58)

    #FIXME: RUN TEST

    # FIXME: temporary labels until backend communication configured
    console_tmp_lbl = tk.Label(master = consoleFrame, text = "Console output will appear here", font=("Cambria", 20))
    console_tmp_lbl.place(x=0, y=60)

    results_tmp_lbl = tk.Label(master = resultFrame, text = "Results will appear here", font=("Cambria", 20))
    results_tmp_lbl.place(x=0, y=60)

    #FIXME: Save results to database


def openCreateWindow():

    createWindow = tk.Toplevel(config.root)
    createWindow.title("Create Test")
    createWindow.geometry("1000x500")

    # FIXME: temporary labels until create test capacity determined
    create_tmp_lbl = tk.Label(master = createWindow, text = "Create test options will appear in this window", font=("Cambria", 20))
    create_tmp_lbl.pack()


def openResultsWindow():

    resultsWindow = tk.Toplevel(config.root)
    resultsWindow.title("Review Results")
    resultsWindow.geometry("1000x500")

    # FIXME: temporary labels until results capacity determined
    results_tmp_lbl = tk.Label(master = resultsWindow, text = "Results will appear in this window", font=("Cambria", 20))
    results_tmp_lbl.pack()




