'''
  DBConnection.py
  brief                 Setting up connection to database
  author                Kai Wei
  version               0.1
  date                  30/09/20
  Support:              email to wei.856@osu.edu
'''

import mysql.connector
from mysql.connector import Error
import subprocess
import os
from itertools import compress

from subprocess import Popen, PIPE
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
		TryDatabase = 'SampleDB'
	try:
		connection = mysql.connector.connect(user=str(TryUsername), password=str(TryPassword),host=str(TryHostAddress),database=str(TryDatabase))
	except (ValueError,RuntimeError, TypeError, NameError,mysql.connector.Error):
		msg = QMessageBox()
		msg.information(None,"Error","Unable to establish connection to host:" + str(TryHostAddress) + ", please check the username/password and host address", QMessageBox.Ok)
		return "Offline"
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
		#remoteList = retrieveAllTests(dbconnection)
		remoteList = ()
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

def getLocalTests(module_id, columns=[]):
	getDirectories = subprocess.run('find {0} -mindepth 2 -maxdepth 2 -type d'.format(os.environ.get("DATA_dir")), shell=True, stdout=subprocess.PIPE)
	dirList = getDirectories.stdout.decode('utf-8').rstrip('\n').split('\n')
	localTests = []
	if module_id:
		for dirName in dirList:
			if "_Module{0}_".format(str(module_id)) in dirName:
				try:
					#getFiles = subprocess.run('find {0} -mindepth 1  -maxdepth 1 -type f -name "*.root"  '.format(dirName), shell=True, stdout=subprocess.PIPE)
					#fileList = getFiles.stdout.decode('utf-8').rstrip('\n').split('\n')
					#if fileList  == [""]:
					#	continue
					test = formatter(dirName,columns,part_id=str(module_id))
					localTests.append(test)
				except Exception as err:
					print("Error detected while formatting the directory name, {}".format(repr(err)))
	else:
		for dirName in dirList:
			#getFiles = subprocess.run('find {0} -mindepth 1  -maxdepth 1 -type f -name "*.root"  '.format(dirName), shell=True, stdout=subprocess.PIPE)
			#fileList = getFiles.stdout.decode('utf-8').rstrip('\n').split('\n')
			#if fileList  == [""]:
			#	continue
			if "_Module" in dirName:
				moduleList = [module for module in dirName.split('_') if "Module"  in module]
				for module in moduleList:
					try:
						test = formatter(dirName,columns,part_id=str(module.lstrip("Module")))
						localTests.append(test)
					except Exception as err:
						print("Error detected while formatting the directory name, {}".format(repr(err)))
	return localTests

def getLocalRemoteTests(dbconnection, module_id = None, columns = []):
	if isActive(dbconnection):
		if module_id == None:
			remoteTests = retrieveGenericTable(dbconnection, "module_tests", columns = columns) #changed table name
			remoteTests = [list(i) for i in remoteTests]
		else:
			remoteTests = retrieveWithConstraint(dbconnection, "module_tests", part_id = int(module_id), columns = columns) #changed table name
			remoteTests = [list(i) for i in remoteTests]
	else:
		remoteTests = []

	localTests = getLocalTests(module_id, columns)
	#localTests = []
	try:
		timeIndex = columns.index("date")
	except:
		timeIndex = -1

	if remoteTests != [] and  timeIndex != -1:
		RemoteSet = set([ele[timeIndex] for ele in remoteTests])
	else:
		RemoteSet = set([])
	if localTests != [] and  timeIndex != -1:
		LocalSet = set([ele[timeIndex] for ele in localTests])
	else:
		LocalSet = set([])

	OnlyRemoteSet = RemoteSet.difference(LocalSet)
	InterSet = RemoteSet.intersection(LocalSet)
	OnlyLocalSet = LocalSet.difference(RemoteSet)

#	allTests = [["Source"]+columns+["localFile"]] #commented to test
	allTests = [["Source"]+columns]
	if(timeIndex != -1):
		for remoteTest in remoteTests:
			if remoteTest[timeIndex] in OnlyRemoteSet:
				allTests.append(['Remote']+remoteTest + [""])
		for localTest in localTests:
			if localTest[timeIndex] in OnlyLocalSet:
				allTests.append(['Local']+localTest)
			if localTest[timeIndex] in InterSet:
				allTests.append(['Synced']+localTest)
	
	
	else:
		for localTest in localTests:
			allTests.append(['']+localTest)
	
		for remoteTest in remoteTests:
			allTests.append(['']+remoteTest + [""])

	return allTests


#####################################################
## Functions for Purdue schema
#####################################################

SampleDB_Schema = {
	"people"	:	["username","name","full_name","email","institute","password","timezone","permissions"],
	"institute" :	["institute","description","timezone"],
}

def getTableList(dbconnection):
	sql_query = '''show tables'''
	cur = dbconnection.cursor()
	cur.execute(sql_query)
	alltuple =  cur.fetchall()
	tablelist = list(map(lambda x: alltuple[x][0], range(0,len(alltuple))))
	return tablelist

def describeTable(dbconnection, table,  KeepAutoIncre = False):
	try:
		sql_query = ''' DESCRIBE {} '''.format(table)
		cur = dbconnection.cursor()
		cur.execute(sql_query)
		alltuple =  cur.fetchall()
		auto_incre_filter = list(map(lambda x: alltuple[x][5] != "auto_increment" or KeepAutoIncre , range(0,len(alltuple))))
		header = list(map(lambda x: alltuple[x][0], range(0,len(alltuple))))
		return list(compress(header, auto_incre_filter))
	except mysql.connector.Error as error:
		print("Failed describing MySQL table: {}".format(error))
		return []

def retrieveWithConstraint(dbconnection, table, *args, **kwargs):
	try:
		constraints = []
		values =  []
		columnList = []
		for key, value in kwargs.items():
			if key == "columns" and type(value) == type(columnList):
				columnList = value
			else:
				values.append(value)
				constraints.append(''' {}=%s  '''.format(key))

		if len(columnList) > 0:
			sql_query = ''' SELECT  ''' +  ",".join(columnList) + ''' FROM {} WHERE {}'''.format(table," AND ".join(constraints))
		else:
			sql_query = ''' SELECT  * FROM {} WHERE {}'''.format(table," AND ".join(constraints))
		
		cur = dbconnection.cursor()
		cur.execute(sql_query,tuple(values))

		alltuple =  cur.fetchall()
		allList = [list(i) for i in alltuple]
		return allList
	except mysql.connector.Error as error:
		print("Failed retrieving MySQL table:{}".format(error))
		return []

def retrieveWithConstraintSyntax(dbconnection, table, syntax, **kwargs):
	try:
		columnList = []
		for key, value in kwargs.items():
			if key == "columns" and type(value) == type(columnList):
				columnList = value
		if len(columnList) > 0:
			sql_query = ''' SELECT  ''' + ",".join(columnList) + ''' FROM  ''' + '''{}'''.format(table)
		else:	
			sql_query = ''' SELECT  * FROM {}'''.format(table)

		sql_query = ''' SELECT  * FROM {} WHERE {}'''.format(str(table),str(syntax))
		cur = dbconnection.cursor()
		cur.execute(sql_query)

		alltuple =  cur.fetchall()
		allList = [list(i) for i in alltuple]
		return allList
	except mysql.connector.Error as error:
		print("Failed retrieving MySQL table:{}".format(error))
		return []

def retrieveGenericTable(dbconnection, table, **kwargs):
	try:
		columnList = []
		for key, value in kwargs.items():
			if key == "columns" and type(value) == type(columnList):
				columnList = value
		if len(columnList) > 0:
			sql_query = ''' SELECT  ''' + ",".join(columnList) + ''' FROM  ''' + '''{}'''.format(table)
		else:	
			sql_query = ''' SELECT  * FROM {}'''.format(table)
		cur = dbconnection.cursor()
		cur.execute(sql_query)
		alltuple =  cur.fetchall()
		allList = [list(i) for i in alltuple]
		return allList
	except Exception as error:
		print("Failed retrieving MySQL table:{}".format(error))
		return []
	
def insertGenericTable(dbconnection, table, args, data):
	try:
		pre_query = '''INSERT INTO ''' + str(table) + ''' ('''+ ",".join(["{}"]*len(args))+''')
					VALUES ('''+ ",".join(['%s']*len(args))+''')'''
		sql_query = pre_query.format(*args)
		#print (sql_query)
		#print (data)
		cur = dbconnection.cursor()
		cur.execute(sql_query,data)
		dbconnection.commit()
		return True
	except Exception as error:
		print("Failed inserting MySQL table {}:  {}".format(table, error))
		return False

def createNewUser(dbconnection, args, data):
	try:
		pre_query = '''INSERT INTO people ('''+ ",".join(["{}"]*len(args))+''')
					VALUES ('''+ ",".join(['%s']*len(args))+''')'''
		sql_query = pre_query.format(*args)
		cur = dbconnection.cursor()
		cur.execute(sql_query,tuple(data))
		dbconnection.commit()
		return True
	except Exception as err:
		print(err)
		return False

def describeInstitute(dbconnection):
	sql_query = ''' DESCRIBE institute '''
	cur = dbconnection.cursor()
	cur.execute(sql_query)
	alltuple =  cur.fetchall()
	header = list(map(lambda x: alltuple[x][0], range(0,len(alltuple))))
	return header

def retrieveAllInstitute(dbconnection):
	sql_query = ''' SELECT * FROM institute '''
	cur = dbconnection.cursor()
	cur.execute(sql_query)
	alltuple =  cur.fetchall()
	allInstitutes = [list(i) for i in alltuple]
	return allInstitutes

def updateGenericTable(dbconnection, table, column, data, **kwargs):
	# A tricky way
	try:
		constraints = []
		values =  []
		for key, value in kwargs.items():
			values.append(value)
			constraints.append(''' {}=%s  '''.format(key))
		sql_query = '''UPDATE ''' + str(table) + ''' SET '''+ ",".join( list(column[i]+"=%s" for i in range(len(column))) )+'''
					WHERE (''' + " AND ".join(constraints) + ''')'''
		cur = dbconnection.cursor()
		cur.execute(sql_query,tuple(data+values))
		dbconnection.commit()
		return True
	except mysql.connector.Error as error:
		print("Failed inserting MySQL table {}:  {}".format(table, error))
		return False


##########################################################################
##  Functions for column selection
##########################################################################

def getByColumnName(column_name, header, databody):
	try:
		index = header.index(column_name)
	except:
		print("column_name not found")
	output = list(map(lambda x: databody[x][index], range(0,len(databody))))
	return output

##########################################################################
##  Functions for column selection (END)
##########################################################################

