'''
  DBConnection.py
  brief                 Setting up connection to database
  author                Kai Wei
  version               0.1
  date                  30/09/20
  Support:              email to wei.856@osu.edu
'''

import mysql.connector
from Gui.GUIutils.ErrorWindow import *

# mySQL databse server as test, may need to extend to other DB format

def StartConnection(TryUsername, TryPassword, TryHostAddress, TryDatabase):
	# For test, no connection to DB is made and output will not be registered
	if TryHostAddress == "0.0.0.0":
		return "DummyDB"

	# Try connecting to DB on localhost with unspecific host address
	if not TryHostAddress:
		TryHostAddress = '127.0.0.1'

	if not TryDatabase:
		TryDatabase = 'phase2pixel_test'
	try:
		connection = mysql.connector.connect(user=str(TryUsername), password=str(TryPassword),host=str(TryHostAddress),database=str(TryDatabase))
	except (ValueError,RuntimeError, TypeError, NameError,mysql.connector.Error):
		ErrorWindow(self.parent, "Error:Unable to establish connection to host:" + str(TryHostAddress) + ", please check the username/password and host address")
		return
	return connection

def createCalibrationEntry(dbconnection, modeInfo):
    sql_query = '''   INSERT INTO calibrationlist( ID, CalibrationName )
                VALUES(?)  '''
    cur = dbconnection.cursor()
    cur.execute(sql_query, modeInfo)
    dbconnection.commit()
    return cur.lastrowid

def retrieveAllCalibrations(dbconnection):
    cur = dbconnection.cursor() 
    cur.execute('SELECT * FROM calibrationlist')
    return cur.fetchall()

def retrieveAllTestResults(dbconnection):
	cur = dbconnection.cursor()
	cur.execute('SELECT * FROM results_test')
	return cur.fetchall()
