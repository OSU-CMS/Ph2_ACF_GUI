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

	test_entries = [
		(1942, 'bmanley', 'pixelalive', '9Jun2020', 19),
		(12, 'johndoe', 'threshold optimization', '6Jun2020', 97),
		(978, 'tester', 'full', '30Feb2018', 83),
		(57234, 'bfrancis', 'new test1', '9Jun2020', 54)
	]

	for test_entry in test_entries:
		database.createTestEntry(test_entry)

	print(database.retrieveAllTestTasks())

	# setup mode entries table
	testnames = [ 
	'full','latency scan', 'pixelalive', 'noise scan', 'scurve scan',
	'gain scan', 'threshold equalization', 'gain optimization',
	'threshold minimization', 'threshold adjustment', 'injection delay scan',
	'clock delay scan', 'physics'
	]

	database.createModesTable()
	database.deleteAllModes()

	for runName in testnames:
		database.createModeEntry((runName,))

	print(database.retrieveAllModes())