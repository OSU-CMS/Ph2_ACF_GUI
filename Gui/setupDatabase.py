
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
		(1942, 'bmanley', 'pixelalive', '12/06/2019 10:25:27', 19),
		(1942, 'bmanley', 'latency scan', '03/06/2020 18:01:14', 92),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:27', 48),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:25', 87),
		(1942, 'bmanley', 'gain', '09/06/2020 08:25:27', 52),
		(1942, 'bmanley', 'threshold optimization', '12/06/2009 10:25:27', 33),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 100),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 06:13:34', 42),
		(12, 'johndoe', 'threshold optimization', '10/06/2020 08:25:27', 55),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 9),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 17),
		(978, 'tester', 'full', '01/06/2020 08:25:27', 83),
		(978, 'tester', 'full', '01/06/2020 04:25:27', 85),
		(20, 'dev', 'pixelalive', '11/06/2020 08:25:27', -1),
		(20, 'dev', 'pixelalive', '11/06/2020 07:25:27', 100),
		(20, 'dev', 'pixelalive', '11/06/2020 10:22:27', 93),
		(20, 'dev', 'full', '11/06/2020 10:25:30', 62),
		(1942, 'bmanley', 'pixelalive', '12/06/2019 10:25:27', 19),
		(1942, 'bmanley', 'latency scan', '03/06/2020 18:01:14', 92),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:27', 48),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:25', 87),
		(1942, 'bmanley', 'gain', '09/06/2020 08:25:27', 52),
		(1942, 'bmanley', 'threshold optimization', '12/06/2009 10:25:27', 33),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 100),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 06:13:34', 42),
		(12, 'johndoe', 'threshold optimization', '10/06/2020 08:25:27', 55),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 9),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 17),
		(978, 'tester', 'full', '01/06/2020 08:25:27', 83),
		(978, 'tester', 'full', '01/06/2020 04:25:27', 85),
		(20, 'dev', 'pixelalive', '11/06/2020 08:25:27', -1),
		(20, 'dev', 'pixelalive', '11/06/2020 07:25:27', 100),
		(20, 'dev', 'pixelalive', '11/06/2020 10:22:27', 93),
		(20, 'dev', 'full', '11/06/2020 10:25:30', 62),
		(1942, 'bmanley', 'pixelalive', '12/06/2019 10:25:27', 19),
		(1942, 'bmanley', 'latency scan', '03/06/2020 18:01:14', 92),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:27', 48),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:25', 87),
		(1942, 'bmanley', 'gain', '09/06/2020 08:25:27', 52),
		(1942, 'bmanley', 'threshold optimization', '12/06/2009 10:25:27', 33),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 100),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 06:13:34', 42),
		(12, 'johndoe', 'threshold optimization', '10/06/2020 08:25:27', 55),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 9),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 17),
		(978, 'tester', 'full', '01/06/2020 08:25:27', 83),
		(978, 'tester', 'full', '01/06/2020 04:25:27', 85),
		(20, 'dev', 'pixelalive', '11/06/2020 08:25:27', -1),
		(20, 'dev', 'pixelalive', '11/06/2020 07:25:27', 100),
		(20, 'dev', 'pixelalive', '11/06/2020 10:22:27', 93),
		(20, 'dev', 'full', '11/06/2020 10:25:30', 62),
		(1942, 'bmanley', 'pixelalive', '12/06/2019 10:25:27', 19),
		(1942, 'bmanley', 'latency scan', '03/06/2020 18:01:14', 92),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:27', 48),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:25', 87),
		(1942, 'bmanley', 'gain', '09/06/2020 08:25:27', 52),
		(1942, 'bmanley', 'threshold optimization', '12/06/2009 10:25:27', 33),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 100),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 06:13:34', 42),
		(12, 'johndoe', 'threshold optimization', '10/06/2020 08:25:27', 55),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 9),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 17),
		(978, 'tester', 'full', '01/06/2020 08:25:27', 83),
		(978, 'tester', 'full', '01/06/2020 04:25:27', 85),
		(20, 'dev', 'pixelalive', '11/06/2020 08:25:27', -1),
		(20, 'dev', 'pixelalive', '11/06/2020 07:25:27', 100),
		(20, 'dev', 'pixelalive', '11/06/2020 10:22:27', 93),
		(20, 'dev', 'full', '11/06/2020 10:25:30', 62),
		(1942, 'bmanley', 'pixelalive', '12/06/2019 10:25:27', 19),
		(1942, 'bmanley', 'latency scan', '03/06/2020 18:01:14', 92),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:27', 48),
		(1942, 'bmanley', 'pixelalive', '01/06/2020 08:25:25', 87),
		(1942, 'bmanley', 'gain', '09/06/2020 08:25:27', 52),
		(1942, 'bmanley', 'threshold optimization', '12/06/2009 10:25:27', 33),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 100),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 06:13:34', 42),
		(12, 'johndoe', 'threshold optimization', '10/06/2020 08:25:27', 55),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 9),
		(12, 'johndoe', 'threshold optimization', '11/06/2020 08:25:27', 17),
		(978, 'tester', 'full', '01/06/2020 08:25:27', 83),
		(978, 'tester', 'full', '01/06/2020 04:25:27', 85),
		(20, 'dev', 'pixelalive', '11/06/2020 08:25:27', -1),
		(20, 'dev', 'pixelalive', '11/06/2020 07:25:27', 100),
		(20, 'dev', 'pixelalive', '11/06/2020 10:22:27', 93),
		(20, 'dev', 'full', '11/06/2020 10:25:30', 62)
	]

	database.deleteAllTests() 

	for test_entry in test_entries:
		database.createTestEntry(test_entry)

	print(database.retrieveAllTestTasks())

	# setup mode entries table
	testnames = [ 
	'pretest', 'full', 'latency scan', 'pixelalive', 'noise scan', 'scurve scan',
	'gain scan', 'threshold equalization', 'gain optimization',
	'threshold minimization', 'threshold adjustment', 'injection delay scan',
	'clock delay scan', 'physics'
	]

	database.createModesTable()
	database.deleteAllModes()

	for runName in testnames:
		database.createModeEntry((runName,))

	print(database.retrieveAllModes())