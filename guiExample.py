import tkinter as tk 
import example

window = tk.Tk()

ncols = 2
nrows = 2

for row in range(nrows):
	window.rowconfigure(row, weight=1, minsize=50)

for col in range(ncols):
	window.columnconfigure(col, weight=1, minsize=50)

def runExample1():
	test_label["text"] = example.displaymessage(-1)

def runExample2():
	test_label["text"] = example.displaymessage(1)

test_button1 = tk.Button(
	master = window, 
	text = "Run C++ test 1", 
	relief = tk.SUNKEN, 
	command = runExample1
)

test_button2 = tk.Button(
	master = window, 
	text = "Run C++ test 2", 
	relief = tk.SUNKEN, 
	command = runExample2
)

test_label = tk.Label(master = window, text = "Output will appear here")

test_label.grid(column=1, sticky = "e")
test_button1.grid(row=0, column=0, sticky = "nw")
test_button2.grid(row=1, column=0, sticky = "sw")

window.mainloop()