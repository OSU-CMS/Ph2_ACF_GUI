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

calibration = {
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

SingleCalibration = ['Latency','PixelAlive','NoiseScan','SCurveScan','GainScan',
					 'ThresholdEqualization','GainOptimization','ThresholdMinimization',
					 'ThresholdAdjustment','InjectionDelay','ClockDelay','Physics']

CompositeCalibration = ['AllScan','StandardStep1','StandardStep2','StandardStep3','StandardStep4']