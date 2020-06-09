'''
  config.py
  \brief                 Global config file for gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import tkinter as tk
from datetime import datetime

database = "testGui.db"
root = tk.Tk()
current_user = ''
review_module_id = -1
result_module_id = -1
result_row_id = -1
new_test_name = tk.StringVar(root)
new_test_name.set('pixelalive')
new_test_moduleID = -1
new_test_grade = -1