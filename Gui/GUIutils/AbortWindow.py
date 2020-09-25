'''
  LoginFrame.py
  brief                 Login page for GUI
  author                Kai
  version               0.1
  date                  18/09/20
  Support:              email to wei.856@osu.edu
'''

import config
import tkinter as tk

from tkinter import ttk
from functools import partial

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