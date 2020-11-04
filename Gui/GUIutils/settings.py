DBServerIP = {
	'Central-remote'		 :  '0.0.0.0',
	'local'					 :  '127.0.0.1',
	'OSU-remote'			 :  '128.146.38.1',
}


# Note: First element of list will be shown as default value
DBNames = {
	'All'					 :  ['phase2pixel_test', 'DBName2', 'DBName3'],
	'Central-remote'		 :  ['phase2pixel_test', 'DBName2', 'DBName3'],
	'local'					 :  ['phase2pixel_test'],
	'OSU-remote'			 :  ['phase2pixel_test'],
}

FirmwareList =  {
	'fc7.board.1' 			 :  '192.168.1.80',
	'fc7.board.2'			 :  '192.168.1.81',
}

ConfigFiles = {
	'Latency'                :  '/Configuration/CMSIT.xml',
	'PixelAlive'             :  '/Configuration/CMSIT.xml',
	'NoiseScan'              :  '/Configuration/CMSIT.xml',
	'SCurveScan'             :  '/Configuration/CMSIT.xml',
	'GainScan'               :  '/Configuration/CMSIT.xml',
	'ThresholdEqualization'  :  '/Configuration/CMSIT.xml',
	'GainOptimization'       :  '/Configuration/CMSIT.xml',
	'ThresholdMinimization'  :  '/Configuration/CMSIT.xml',
	'ThresholdAdjustment'    :  '/Configuration/CMSIT.xml',
	'InjectionDelay'         :  '/Configuration/CMSIT.xml',
	'ClockDelay'             :  '/Configuration/CMSIT.xml',
	'Physics'                :  '/Configuration/CMSIT.xml',
	'AllScan'                :  '/Configuration/CMSIT.xml',
}

Test = {
	'AllScan'                :  'pixelalive',
	'StandardStep1'          :  'pixelalive',
	'StandardStep2'          :  'pixelalive',
	'StandardStep3'          :  'pixelalive',
	'StandardStep4'          :  'pixelalive',
	'Latency'                :  'latency',
	'PixelAlive'             :  'pixelalive',
	'NoiseScan'              :  'noise',
	'SCurveScan'             :  'scurve',
	'GainScan'               :  'gain',
	'ThresholdEqualization'  :  'threqu',
	'GainOptimization'       :  'gainopt',
	'ThresholdMinimization'  :  'thrmin',
	'ThresholdAdjustment'    :  'thradj',
	'InjectionDelay'         :  'injdelay',
	'ClockDelay'             :  'clockdelay',
	'Physics'                :  'physics',
}

SingleTest = ['Latency','PixelAlive','NoiseScan','SCurveScan','GainScan',
					 'ThresholdEqualization','GainOptimization','ThresholdMinimization',
					 'ThresholdAdjustment','InjectionDelay','ClockDelay','Physics']

CompositeTest = ['AllScan','StandardStep1','StandardStep2','StandardStep3','StandardStep4']
CompositeList = {
	'AllScan': ['Latency','PixelAlive', 'GainScan'],
	'StandardStep1': ['Latency','PixelAlive'],
	'StandardStep2': ['Latency','PixelAlive', 'SCurveScan'],
	'StandardStep3': ['Latency','PixelAlive'],
	'StandardStep4': ['Latency','PixelAlive']
}
firstTimeList = ['AllScan', 'StandardStep1', 'PixelAlive']
header = ['Source', 'Module_ID', 'User', 'Test', 'Time', 'Grade', 'DQMFile']