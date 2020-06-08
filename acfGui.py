'''
  acfGui.py
  \brief                 Interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk
from tkinter import ttk
import example
import gui
import config
import database

config.root.title('Ph2_ACF Grading System')
config.root.geometry('{}x{}'.format(600, 300))
config.root.rowconfigure(0, weight=1, minsize=50)
config.root.rowconfigure(1, weight=1, minsize=150)
config.root.columnconfigure(0, weight=1, minsize=100)
config.root.columnconfigure(1, weight=1, minsize=100)

config.startFrame.grid(row=1, column=0, sticky="ws")
config.otherFrame.grid(row=1, column=1, sticky="es")

config.startFrame.grid_propagate(False)
config.otherFrame.grid_propagate(False)

config.startFrame.rowconfigure(0, weight=1, minsize=50)
config.startFrame.rowconfigure(1, weight=1, minsize=50)
config.startFrame.rowconfigure(2, weight=1, minsize=50)
config.startFrame.rowconfigure(3, weight=1, minsize=50)
config.startFrame.columnconfigure(0, weight=1, minsize=130)
config.startFrame.columnconfigure(1, weight=1, minsize=130)

config.otherFrame.rowconfigure(0, weight=1, minsize=100)
config.otherFrame.rowconfigure(1, weight=1, minsize=100)
config.otherFrame.rowconfigure(2, weight=1, minsize=100)
config.otherFrame.columnconfigure(0, weight=1, minsize=300)

wel_lbl = tk.Label(master = config.root, text = "Phase 2 Pixel Grading System", font=("Helvetica", 25))
wel_lbl.grid(row=0, column=0, columnspan=2, sticky="new")

##########################################################################
##### start frame ########################################################
##########################################################################

startTest_lbl = tk.Label(master = config.startFrame, text = "Start a new test", font=("Cambria", 15))
startTest_lbl.grid(row=0, column=0, columnspan=2, sticky="n")

username_lbl = tk.Label(master = config.startFrame, text = "User name", font=("Cambria", 13))
username_lbl.grid(row=1, column=0, sticky="ew")

config.username_en.grid(row=1, column=1, sticky="we")

testname_lbl = tk.Label(master = config.startFrame, text = "Test name", font=("Cambria", 13))
testname_lbl.grid(row=2, column=0, sticky="we")

currentModes = []
for mode in list(database.retrieveAllModes()):
	currentModes.append(mode[1])
print(currentModes)
w = tk.OptionMenu(config.startFrame, config.testname, *currentModes)
w.grid(row=2, column=1, sticky="we")

desc_lbl = tk.Label(master = config.startFrame, text = "Description", font=("Cambria", 13))
desc_lbl.grid(row=3, column=0, sticky="ew")

config.desc_tx.grid(row=3, column=1, sticky="we")

start_btn = ttk.Button(
	master = config.startFrame, 
	text = "Start Test", 
	command = gui.openRunWindow
)
start_btn.grid(row=4, column=1, sticky="s")

##########################################################################
##### create/view frame ##################################################
##########################################################################

other_lbl = tk.Label(master = config.otherFrame, text = "", font=("Cambria", 15))
other_lbl.grid(row=3, column=0, sticky="n")

create_btn = ttk.Button(
	master = config.otherFrame, 
	text = "Create Test",
	command = gui.openCreateWindow
)
create_btn.place(x=50, y=75)

view_btn = ttk.Button(
	master = config.otherFrame,
	text = "View Previous Results",
	command = gui.openResultsWindow
)
view_btn.place(x=50, y=115)

config.root.mainloop()