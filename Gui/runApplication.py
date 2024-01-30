
import logging

# Customize the logging configuration
logging.basicConfig(
   level=logging.INFO,
   format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
   filename='my_project.log',  # Specify a log file
   filemode='w'  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

'''
  runApplication.py
  brief                 Interface for GUI Application
  author                Kai Wei
  version               0.1
  date                  10/02/20
  Support:              email to wei.856@osu.edu
'''

import tkinter as tk
import Gui.GUIutils.Application as App
import Gui.GUIutils.LoginFrame as login
import database
from PIL import ImageTk, Image
from tkinter import ttk



app = App.Application()
app.SetGeometry()
app.SetTitle()

#app.root.title('Ph2_ACF Grading System')
#app.root.geometry('{}x{}'.format(600, 300))
#app.root.rowconfigure(0, weight=1, minsize=50)
#app.root.rowconfigure(1, weight=1, minsize=250)
#app.root.columnconfigure(0, weight=1, minsize=300)
#app.root.columnconfigure(1, weight=1, minsize=300)

#title_label = tk.Label(master = app.root, text = "Phase 2 Pixel Grading System", relief=tk.GROOVE, font=('Helvetica', 25, 'bold'))
#title_label.grid(row=0, column=0, columnspan=2, sticky="new")

login.LoginFrame(app)
app.root.mainloop()
