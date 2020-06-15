'''
  acfGui.py
  \brief                 Interface for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk
import gui
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

##########################################################################
##### login frame ########################################################
##########################################################################

cms_img = Image.open('cmsicon.png')
cms_img = cms_img.resize((70, 70), Image.ANTIALIAS)
cms_photo = ImageTk.PhotoImage(cms_img)

osu_img = Image.open('osuicon.png')
osu_img = osu_img.resize((160, 70), Image.ANTIALIAS)
osu_photo = ImageTk.PhotoImage(osu_img)

def loginUser():
	if username_entry.get() == '':
		gui.displayErrorWindow("Error: Please enter a valid username")
		return 
	config.current_user = username_entry.get()

	login_label['text'] = 'Welcome {0}'.format(config.current_user)
	login_label['font'] = ('Helvetica', 25, 'bold')

	username_label.grid_remove()
	username_entry.grid_remove()
	login_button.grid_remove()
	logout_button.grid(row=2, column=0, columnspan=2)
	optionsFrame.grid(row=1, column=1, sticky="es")
	loginFrame.grid(row=1, column=0,  sticky="ws")
	loginFrame['width'] = 300
	loginFrame.columnconfigure(0, weight=1, minsize=150)
	loginFrame.columnconfigure(1, weight=1, minsize=150)

def logoutUser():
	config.current_user = ''

	login_label['text'] = 'Please login'
	login_label['font'] = ('Helvetica', 20, 'bold')
	username_label.grid()
	username_entry.grid()
	login_button.grid()
	
	logout_button.grid_remove()
	optionsFrame.grid_remove()
	loginFrame.grid(row=1, column=0,  sticky="news")
	loginFrame['width'] = 600
	loginFrame.columnconfigure(0, weight=1, minsize=300)
	loginFrame.columnconfigure(1, weight=1, minsize=300)

loginFrame = tk.Frame(config.root, width=600, height=250)
loginFrame.grid(row=1, column=0, columnspan=2, sticky="nsew")
loginFrame.rowconfigure(0, weight=1, minsize=35)
loginFrame.rowconfigure(1, weight=1, minsize=25)
loginFrame.rowconfigure(2, weight=1, minsize=40)
loginFrame.rowconfigure(3, weight=1, minsize=30)
loginFrame.rowconfigure(4, weight=1, minsize=65)
loginFrame.rowconfigure(5, weight=1, minsize=55)
loginFrame.columnconfigure(0, weight=1, minsize=300)
loginFrame.columnconfigure(1, weight=1, minsize=300)
loginFrame.grid_propagate(False)

login_label = tk.Label(master = loginFrame, text = "Please login", font=("Helvetica", 20, 'bold'))
login_label.grid(row=1, column=0, columnspan=2, sticky="n")

username_label = tk.Label(master = loginFrame, text = "Username", font=("Helvetica", 15))
username_label.grid(row=2, column=0, sticky='e')

username_entry = tk.Entry(master = loginFrame, text='')
username_entry.grid(row=2, column=1, sticky='w')

osu_label = tk.Label(master=loginFrame, image=osu_photo)
osu_label.grid(row=4, column=0, sticky='ewns')

cms_label = tk.Label(master=loginFrame, image=cms_photo)
cms_label.grid(row=4, column=1, sticky='ewns')

login_button = ttk.Button(
	master = loginFrame, 
	text = "Log in", 
	command = loginUser
)

login_button.grid(row=3, column=0, columnspan=2)

logout_button = ttk.Button(
	master = loginFrame, 
	text = "Log out", 
	command = logoutUser
)

##########################################################################
##### options frame ######################################################
##########################################################################

def checkReviewModuleID():
	try:
		config.review_module_id = int(module_num_entry.get())
	except:
		gui.displayErrorWindow("Error: Please enter valid module ID")
		return
	gui.openReviewModuleWindow()
	
optionsFrame = tk.Frame(config.root, width=300, height=250)
# optionsFrame.grid(row=1, column=1, sticky="es")
optionsFrame.rowconfigure(0, weight=1, minsize=5)
optionsFrame.rowconfigure(1, weight=1, minsize=40)
optionsFrame.rowconfigure(2, weight=1, minsize=40)
optionsFrame.rowconfigure(3, weight=1, minsize=40)
optionsFrame.rowconfigure(4, weight=1, minsize=40)
optionsFrame.rowconfigure(5, weight=1, minsize=40)
optionsFrame.rowconfigure(6, weight=1, minsize=40)
optionsFrame.rowconfigure(7, weight=1, minsize=5)
optionsFrame.columnconfigure(0, weight=1, minsize=100)
optionsFrame.columnconfigure(1, weight=1, minsize=100)
optionsFrame.columnconfigure(2, weight=1, minsize=100)
optionsFrame.grid_propagate(False)

start_button = ttk.Button(
	master = optionsFrame, 
	text = "Start new test", 
	command = gui.openStartWindow
)
start_button.grid(row=1, column=0, columnspan=3, sticky='we')

create_button = ttk.Button(
	master = optionsFrame,
	text = "Create new test", 
	command = gui.openCreateWindow
)
create_button.grid(row=2, column=0, columnspan=3, sticky='ew')

results_button = ttk.Button(
	master = optionsFrame,
	text = "Review results", 
	command = gui.openResultsWindow
)
results_button.grid(row=3, column=0, columnspan=3, sticky='ew')

review_module_label = tk.Label(master = optionsFrame, text = "Review a specific module", font=("Helvetica", 17))
review_module_label.grid(row=4, column=0, columnspan=3, sticky='we')

module_num_label = tk.Label(master = optionsFrame, text = "   Module ID", font=("Helvetica", 15))
module_num_label.grid(row=5, column=0, sticky='w')

module_num_entry = tk.Entry(master = optionsFrame, text='')
module_num_entry.grid(row=5, column=1)

review_button = ttk.Button(
	master = optionsFrame,
	text = "Review", 
	command = checkReviewModuleID
)
review_button.grid(row=5, column=2)

config.root.mainloop()