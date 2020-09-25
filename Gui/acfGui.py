'''
  acfGui.py
  brief                 Interface for pixel grading gui
  author                Brandon Manley
  version               0.1
  date                  06/08/20
  Support:              email to manley.329@osu.edu
'''

import tkinter as tk
#import GUIutils.guiUtils as gui
import GUIutils.LoginFrame as login
import config
import database
from PIL import ImageTk, Image
from tkinter import ttk

config.root.title('Ph2_ACF Grading System')
config.root.geometry('{}x{}'.format(600, 300))
config.root.rowconfigure(0, weight=1, minsize=50)
config.root.rowconfigure(1, weight=1, minsize=250)
config.root.columnconfigure(0, weight=1, minsize=300)
config.root.columnconfigure(1, weight=1, minsize=300)

title_label = tk.Label(master = config.root, text = "Phase 2 Pixel Grading System", relief=tk.GROOVE, font=('Helvetica', 25, 'bold'))
title_label.grid(row=0, column=0, columnspan=2, sticky="new")

login.LoginFrame()
config.root.mainloop()

#class Application(tk.Tk):
#	def __init__(self):
#		self.root = tk.Tk()
#		self.root.title('Ph2_ACF Grading System')
#		self.root.geometry('{}x{}'.format(600, 300))
#		self.root.rowconfigure(0, weight=1, minsize=50)
#		self.root.rowconfigure(1, weight=1, minsize=250)
#		self.root.columnconfigure(0, weight=1, minsize=300)
#		self.root.columnconfigure(1, weight=1, minsize=300)

#		self.title_label = tk.Label(master = config.root, text = "Phase 2 Pixel Grading System", relief=tk.GROOVE, font=('Helvetica', 25, 'bold'))
#		self.title_label.grid(row=0, column=0, columnspan=2, sticky="new")
#		gui.LoginFrame()
#		self.root.mainloop()

#if __name__ == '__main__':
#    Application()