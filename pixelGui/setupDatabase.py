'''
  setupDatabase.py
  \brief                 Setup database for pixel grading gui
  \author                Brandon Manley
  \version               0.1
  \date                  06/08/20
  Support:               email to manley.329@osu.edu
'''

import config
import gui
import database

if __name__ == "__main__":
    # setup test entries table
    database.createTestsTable()
    database.createTestEntry(('bmanley', 'test1', '20', 98, 'this is a test'))
    database.createTestEntry(('otheruser', 'test2', '20', 12, 'this is a second test'))
    print(database.retrieveAllTestTasks())

    # setup mode entries table
    database.createModesTable()
    database.clearAllModes()
    for runName in config.testnames:
        database.createModeEntry((runName,))
    print(database.retrieveAllModes())