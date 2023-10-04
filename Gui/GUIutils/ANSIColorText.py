
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
  ANSIColorText.py
  brief                 Text handler for Ph2 ACF GUI
  author                Kai Wei
  version               0.1
  date                  09/24/20
  Support:              email to wei.856@osu.edu
'''
import tkinter as tk
import re

class AnsiColorText(tk.Text):
	foreground_colors = {
			'bright' : {
							'30' : 'Black',
							'31' : 'Red',
							'32' : 'Green',
							'33' : 'Brown',
							'34' : 'Blue',
							'35' : 'Purple',
							'36' : 'Cyan',
							'37' : 'White'
							},
			'dim'    :  {
							'30' : 'DarkGray',
							'31' : 'LightRed',
							'32' : 'LightGreen',
							'33' : 'Yellow',
							'34' : 'LightBlue',
							'35' : 'Magenta',
							'36' : 'Pink',
							'37' : 'White'
							}
		}

	background_colors= {
			'bright' : {
							'40' : 'Black',
							'41' : 'Red',
							'42' : 'Green',
							'43' : 'Brown',
							'44' : 'Blue',
							'45' : 'Purple',
							'46' : 'Cyan',
							'47' : 'White'
							},
			'dim'    :  {
							'40' : 'DarkGray',
							'41' : 'LightRed',
							'42' : 'LightGreen',
							'43' : 'Yellow',
							'44' : 'LightBlue',
							'45' : 'Magenta',
							'46' : 'Pink',
							'47' : 'White'
							}
		}

	color_pat = re.compile('\x01?\x1b\[([\d+;]*?)m\x02?')
	inner_color_pat = re.compile("^(\d+;?)+$")

	def __init__(self, *args, **kwargs):
		tk.Text.__init__(self, *args, **kwargs)
		self.known_tags = set([])
		self.register_tag("37", "White", "Black")
		self.reset_to_default_attribs()

	def reset_to_default_attribs(self):
		self.tag = '37'
		self.bright = 'bright'
		self.foregroundcolor = 'White'
		self.backgroundcolor = 'Black'

	def register_tag(self, txt, foreground, background):
		self.tag_config(txt, foreground=foreground, background=background)
		self.known_tags.add(txt)

	def write(self, text, is_editable=False):
		segments = AnsiColorText.color_pat.split(text)
		if segments:
			for text in segments:
				if AnsiColorText.inner_color_pat.match(text):
					if text not in self.known_tags:
						parts = text.split(";")
						for part in parts:
							if part in AnsiColorText.foreground_colors[self.bright]:
								self.foregroundcolor = AnsiColorText.foreground_colors[self.bright][part]
							elif part in AnsiColorText.background_colors[self.bright]:
								self.backgroundcolor = AnsiColorText.background_colors[self.bright][part]
							else:
								for ch in part:
									if ch == '0' :
										self.reset_to_default_attribs()
									if ch == '1' :
										self.bright = 'bright'
									if ch == '2' :
										self.bright = 'dim'

					self.register_tag(text,
										foreground=self.foregroundcolor,
										background=self.backgroundcolor)
					self.tag = text
				elif text == '':
					self.tag = '37' # black
				else:
					self.insert(tk.END,text,self.tag)
