'''
  acfGui.py
  \brief                 Global config file for gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk
from datetime import datetime

testnames = [ 
'full','latency scan', 'pixelalive', 'noise scan', 'scurve scan',
'gain scan', 'threshold equalization', 'gain optimization',
'threshold minimization', 'threshold adjustment', 'injection delay scan',
'clock delay scan', 'physics'
]

root = tk.Tk()
username = ''
currentTestGrade = -1
comment = ''

startFrame = tk.Frame(root, width=295, height=295, padx=5, pady=5)
otherFrame = tk.Frame(root, width=300, height=300)

username_en = tk.Entry(master = startFrame, text="")

testname = tk.StringVar(startFrame)
testname.set("pixelalive") # default value

desc_tx = tk.Text(master=startFrame, borderwidth=1, relief=tk.RAISED)

database = "testGui.db"