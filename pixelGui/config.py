'''
  acfGui.py
  \brief                 Interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/05/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk

testnames = [ # FIXME: add to database
'latency scan', 'pixelalive', 'noise scan', 'scurve scan',
'gain scan', 'threshold equalization', 'gain optimization',
'threshold minimization', 'threshold adjustment', 'injection delay scan',
'clock delay scan', 'physics'
]

root = tk.Tk()
testname = ''

startFrame = tk.Frame(root, width=295, height=295, padx=5, pady=5)
otherFrame = tk.Frame(root, width=300, height=300)

username_en = tk.Entry(master = startFrame, text="")

testname = tk.StringVar(startFrame)
testname.set("pixelalive") # default value
w = tk.OptionMenu(startFrame, testname, *testnames)

desc_tx = tk.Text(master=startFrame, borderwidth=1, relief=tk.RAISED)