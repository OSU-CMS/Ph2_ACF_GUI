'''
  acfGui.py
  \brief                 Functions for interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk 
from tkinter import ttk
import config
import database
from datetime import datetime

def abortTest():
    #FIXME: add dialog box confirming abort
    #FIXME: indicate in database that test aborted??
    pass


def saveTest():
    #FIXME: check test is finished running
    currentDate = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
    testingInfo = (config.testname.get(), config.username, currentDate, config.currentTestGrade, config.comment)

    print(testingInfo)
    try:
        database.createTestEntry(testingInfo)
    except:
        # FIXME: show dialog box indicating error
        print("Error: could not save")


def openRunWindow():
    runWindow = tk.Toplevel(config.root)
    runWindow.title("Grading Run")
    runWindow.geometry("1000x500")

    config.username = config.username_en.get()
    config.comment = config.desc_tx.get("1.0", tk.END)

    if config.testname.get() == "":
        err_lbl = tk.Label(master = runWindow, text = "ERROR: Please enter a valid test name", font=("Cambria", 15))
        err_lbl.pack()
        return
    if config.username == "":
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

    user_lbl = tk.Label(master = descFrame, text = "User: {0}".format(config.username), font=("Cambria", 15))
    user_lbl.place(x=0, y=33)

    abort_btn = ttk.Button(
        master = descFrame,
        text = "Abort Test",
        command = abortTest
    )
    abort_btn.place(x=0, y=58)

    save_btn = ttk.Button(
        master = descFrame,
        text = "Save Test",
        command = saveTest
    )
    save_btn.place(x=120, y=58)

    #FIXME: RUN TEST

    # FIXME: temporary labels until backend communication configured
    console_tmp_lbl = tk.Label(master = consoleFrame, text = "Console output will appear here", font=("Cambria", 20))
    console_tmp_lbl.place(x=0, y=60)

    results_tmp_lbl = tk.Label(master = resultFrame, text = "Results will appear here", font=("Cambria", 20))
    results_tmp_lbl.place(x=0, y=60)



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