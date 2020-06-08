'''
  database.py
  \brief                 Functions for database interfacing
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import config
import sqlite3
from sqlite3 import Error

def createDatabaseConnection(db_file):
    conn = None
    try:    
        conn = sqlite3.connect(db_file)
    except Error as e:
        print(e)
    return conn


def createTestsTable():

    conn = createDatabaseConnection(config.database)
    sql = '''   CREATE TABLE IF NOT EXISTS tests (
                    id integer PRIMARY KEY,
                    username text,
                    testname text,
                    date text, 
                    grade integer,
                    comment text
                ); '''
    try:
        c = conn.cursor()
        c.execute(sql)
    except Error as e:
        print(e)


def createTestEntry(runInfo):
    sql = '''   INSERT INTO tests(username,testname,date,grade,comment)
                VALUES(?,?,?,?,?)  '''
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor()
    cur.execute(sql, runInfo)
    conn.commit()
    return cur.lastrowid


def updateTestEntry(newInfo):
    sql = '''   UPDATE tests
                SET username = ?,
                    testname = ?,
                    date = ?,
                    grade = ?,
                    description = ?
                WHERE id = ?    '''
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor()
    cur.execute(sql, newInfo)
    conn.commit()


def deleteTestEntry(id):
    sql = 'DELETE FROM tests WHERE id=?'
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor()
    cur.execute(sql, (id,))
    conn.commit()


def retrieveAllTestTasks():
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor() 
    cur.execute('SELECT * FROM tests')
    return cur.fetchall()


def createModesTable():

    conn = createDatabaseConnection(config.database)
    sql = '''   CREATE TABLE IF NOT EXISTS modes (
                    id integer PRIMARY KEY,
                    name text
                ); '''
    try:
        c = conn.cursor()
        c.execute(sql)
    except Error as e:
        print(e)


def createModeEntry(modeInfo):
    sql = '''   INSERT INTO modes(name)
                VALUES(?)  '''
    print(modeInfo)
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor()
    cur.execute(sql, modeInfo)
    conn.commit()
    return cur.lastrowid


def retrieveAllModes():
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor() 
    cur.execute('SELECT * FROM modes')
    return cur.fetchall()


def clearAllModes():
    sql = 'DELETE FROM modes'
    conn = createDatabaseConnection(config.database)
    cur = conn.cursor() 
    cur.execute(sql)
    conn.commit()