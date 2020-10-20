'''
  Application.py
  brief                 Global settings for gui
  author                Kai
  version               0.1
  date                  02/10/20
  Support:              email to wei.856@osu.edu
'''

import tkinter as tk
from tkinter import ttk
from datetime import datetime
import os
from PIL import ImageTk, Image

from Gui.GUIutils.guiUtils  import *

phase2_repo_dir = '/home/bmanley/Ph2_ACF/'
#database = "test.db"

class Application():
	def __init__(self):
		self.root = tk.Tk()
		self.scwidth = self.root.winfo_screenwidth()
		self.scheight = self.root.winfo_screenheight()

		#self.root.tk.call('tk', 'scaling', 0.1)

		self.current_user = ''

		self.all_results = [] 
		self.all_results = []
		self.result_sort_attr = tk.StringVar(self.root)
		self.result_sort_attr.set('date')
		self.result_sort_dir = tk.StringVar(self.root)
		self.result_sort_dir.set('increasing')

		self.result_filter_attr = tk.StringVar(self.root)
		self.result_filter_attr.set('user')
		self.result_filter_eq = tk.StringVar(self.root)
		self.result_filter_eq.set('=')

		# currently reviewing module info
		self.review_best_plot = ''
		self.review_latest_plot = ''
		self.review_module_id = -1
		self.review_sort_attr = tk.StringVar(self.root)
		self.review_sort_attr.set('date')
		self.review_sort_dir = tk.StringVar(self.root)
		self.review_sort_dir.set('increasing')

		self.review_filter_attr = tk.StringVar(self.root)
		self.review_filter_attr.set('user')
		self.review_filter_eq = tk.StringVar(self.root)
		self.review_filter_eq.set('=')

		self.module_results = []

		# currently viewing result info
		self.result_row_id = -1
		self.plot_images = []

		# currently testing module info
		self.current_test_name = tk.StringVar(self.root)
		self.current_test_name.set('PixelAlive')
		self.current_module_id = -1
		self.current_test_num = -1
		self.current_test_grade = -1
		self.current_test_plot = ''
		self.current_test_plot_location = ''

		# testing
		self.buttonStyle = ttk.Style()
		self.buttonStyle.configure('Test.TButton', font=('Helvetica', 15), background='white', foreground='white', highlightbackground='white')

		self.labelStyle = ttk.Style()
		self.labelStyle.configure('Important.TLabel', font=('Helvetica', 25, 'Bold'), fg='black', bg='white')
	
	def SetGeometry(self):
		self.root.geometry('{}x{}'.format(scaleInvWidth(self.root, 0.7), scaleInvHeight(self.root, 0.7)))
		self.root.rowconfigure(0, weight=1, minsize=50)
		self.root.rowconfigure(1, weight=1, minsize=250)
		self.root.columnconfigure(0, weight=1, minsize=300)
		self.root.columnconfigure(1, weight=1, minsize=300)

	def SetTitle(self):
		self.root.title('Ph2_ACF Grading System')
		title_label = tk.Label(master = self.root, text = "Phase 2 Pixel Grading System", relief=tk.GROOVE, font=('Helvetica', 25, 'bold'))
		title_label.grid(row=0, column=0, columnspan=2, sticky="new")	
