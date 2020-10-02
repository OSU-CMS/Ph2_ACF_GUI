'''
  CreateWindow.py
  brief                 Create test options
  author                Kai Wei
  version               0.1
  date                  09/24/20
  Support:              email to wei.856@osu.edu
'''

import config
import tkinter as tk

from Gui.GUIutils.Application import *
from Gui.GUIutils.ErrorWindow import *

#FIXME: check if this window is needed
class CreateWindow(tk.Toplevel):
	def __init__(self, parent, dbconnection):
		tk.Toplevel.__init__(self)
		self.parent = parent

		if self.parent.current_user == '':
			ErrorWindow(self.parent, "Error: Please login")
			return

		self.master = self.parent.root
		self['bg'] = 'white'
		self.title("Create Test")
		self.geometry("1000x500")

		create_tmp_lbl = tk.Label(master = self, text = "Create test options will appear in this window", font=("Helvetica", 20))
		create_tmp_lbl.pack()