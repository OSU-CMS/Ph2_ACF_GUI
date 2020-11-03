'''
  DBConnection.py
  brief                 Setting up connection to database
  author                Kai Wei
  version               0.1
  date                  30/09/20
  Support:              email to wei.856@osu.edu
'''

import mysql.connector
import subprocess
import os

from subprocess import Popen, PIPE
from Gui.GUIutils.ErrorWindow import *
from PyQt5.QtWidgets import (QMessageBox)
from Gui.GUIutils.settings import *
from Gui.GUIutils.guiUtils import *


DB_TestResult_Schema = ['Module_ID, Account, CalibrationName, ExecutionTime, Grading, DQMFile']

# mySQL databse server as test, may need to extend to other DB format

def StartConnection(TryUsername, TryPassword, TryHostAddress, TryDatabase, master):
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
		ErrorWindow(master, "Error:Unable to establish connection to host:" + str(TryHostAddress) + ", please check the username/password and host address")
		return
	return connection

def QtStartConnection(TryUsername, TryPassword, TryHostAddress, TryDatabase):
	# For test, no connection to DB is made and output will not be registered
	#if TryHostAddress == "0.0.0.0":
	#	return "DummyDB"

	# Try connecting to DB on localhost with unspecific host address
	if not TryHostAddress:
		TryHostAddress = '127.0.0.1'

	if not TryDatabase:
		TryDatabase = 'phase2pixel_test'
	try:
		connection = mysql.connector.connect(user=str(TryUsername), password=str(TryPassword),host=str(TryHostAddress),database=str(TryDatabase))
	except (ValueError,RuntimeError, TypeError, NameError,mysql.connector.Error):
		msg = QMessageBox()
		msg.information(None,"Error","Unable to establish connection to host:" + str(TryHostAddress) + ", please check the username/password and host address", QMessageBox.Ok)
		return
	return connection

def checkDBConnection(dbconnection):
	if dbconnection == "Offline":
		statusString = "<---- offline Mode ---->"
		colorString = "color:red"
	elif dbconnection.is_connected():
		statusString = "<---- DB Connection established ---->"
		colorString = "color: green"
	else:
		statusString = "<---- DB Connection broken ---->"
		colorString = "color: red"
	return statusString, colorString

def createCalibrationEntry(dbconnection, modeInfo):
	sql_query = '''   INSERT INTO calibrationlist( ID, CalibrationName )
				VALUES(?)  '''
	cur = dbconnection.cursor()
	cur.execute(sql_query, modeInfo)
	dbconnection.commit()
	return cur.lastrowid

def getAllTests(dbconnection):
	if dbconnection == "Offline":
		remoteList = []
	elif dbconnection.is_connected():
		remoteList = retrieveAllTests(dbconnection)
		remoteList = [list(i) for i in remoteList]
	else:
		QMessageBox().information(None, "Warning", "Database connection broken", QMessageBox.Ok)
		remoteList = []
	localList = list(Test.keys())
	remoteList = [remoteList[i][0] for i in range(len(remoteList))]
	for test in remoteList:
		if not test in localList:
			localList.append(test)
	return sorted(set(localList), key = localList.index)

def retrieveAllTests(dbconnection):
	if dbconnection.is_connected() == False:
		return 
	cur = dbconnection.cursor() 
	cur.execute('SELECT * FROM calibrationlist')
	return cur.fetchall()

def retriveTestTableHeader(dbconnection):
	cur = dbconnection.cursor()
	cur.execute('DESCRIBE results_test')
	return cur.fetchall()

def retrieveAllTestResults(dbconnection):
	cur = dbconnection.cursor()
	cur.execute('SELECT * FROM results_test')
	return cur.fetchall()

def retrieveModuleTests(dbconnection, module_id):
	sql_query = ''' SELECT * FROM results_test WHERE  Module_id = {0} '''.format(str(module_id))
	cur = dbconnection.cursor()
	cur.execute(sql_query)
	return cur.fetchall()

def retrieveModuleLastTest(dbconnection, module_id):
	sql_query = ''' SELECT * FROM results_test T 
					INNER JOIN (
						SELECT Module_ID, max(ExecutionTime) as MaxDate
						from results_test T WHERE Module_ID = {0}
						group by Module_ID
					) LATEST ON T.Module_ID = LATEST.Module_ID AND T.ExecutionTime = LATEST.MaxDate
				'''.format(str(module_id))
	cur = dbconnection.cursor()
	cur.execute(sql_query)
	return cur.fetchall()

def insertTestResult(dbconnection, record):
	sql_query = ''' INSERT INTO results_test ({},{},{},{},{},{})
					VALUES ({},{},{},{},{},{})
				'''.format(*DB_TestResult_Schema,*record)
	cur = dbconnection.cursor()
	cur.execute(sql_query)
	dbconnection.commit()
	return cur.lastrowid

def getLocalTests(module_id):
	getDirectories = subprocess.run('find {0} -mindepth 2 -maxdepth 2 -type d'.format(os.environ.get("DATA_dir")), shell=True, stdout=subprocess.PIPE)
	dirList = getDirectories.stdout.decode('utf-8').rstrip('\n').split('\n')
	localTests = []
	for dirName in dirList:
		if "_Module{0}_".format(str(module_id or '')) in dirName:
			test = formatter(dirName)
			localTests.append(test)
	return localTests

def getLocalRemoteTests(dbconnection, module_id = None):
	if isActive(dbconnection):
		if module_id:
			remoteTests = retrieveModuleTests(dbconnection, module_id)
			remoteTests = [list(i) for i in remoteTests]
		else:
			remoteTests = retrieveAllTestResults(dbconnection)
			remoteTests = [list(i) for i in remoteTests]
	else:
		remoteTests = []

	localTests = getLocalTests(module_id)

	if remoteTests != []:
		RemoteSet = set([ele[3] for ele in remoteTests])
	else:
		RemoteSet = set([])
	if localTests != []:
		LocalSet = set([ele[3] for ele in localTests])
	else:
		LocalSet = set([])
	OnlyRemoteSet = RemoteSet.difference(LocalSet)
	InterSet = RemoteSet.intersection(LocalSet)
	OnlyLocalSet = LocalSet.difference(RemoteSet)

	allTests = [header]

	for localTest in localTests:
		if localTest[3] in OnlyLocalSet:
			allTests.append(['Local']+localTest)
	
	for remoteTest in remoteTests:
		if remoteTest[3] in OnlyRemoteSet:
			allTests.append(['Remote']+remoteTest)
		
		if remoteTest[3] in InterSet:
			allTests.append(['Synced']+remoteTest)

	return allTests

