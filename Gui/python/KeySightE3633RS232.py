import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def InitialDevice(device):
	try:
		device.write(":SYSTEM:REMOTE")
		device.write("*RST")		
	except Exception as err:
		logger.error("Error occured while restore defaults: {}".format(err))
	# Select voltage source mode

def GetInfo(device):
	try:
		info =  device.query("*IDN?")
		print(info)
		return info
	except Exception as err:
		logger.error("Error occured while requesting the identification: {}".format(err))

def TurnOn(device):
	try:
		device.write(":OUTPUT ON")
	except Exception as err:
		logger.error("Error occured while turning on the device: {}".format(err))

def TurnOff(device):
	try:
		device.write(":OUTPUT OFF")
	except  Exception as err:
		logger.error("Error occured while turning off the device: {}".format(err))

def SetVoltage(device, voltage = 0.0, VoltProtection = 0.0):
	# Set Voltage range 2V and output to 1.78V
	try:
		device.write(":SOURCE:VOLTAGE:PROTECTION:STATE ON")
		state = device.query(":SOURCE:VOLTAGE:PROTECTION:STATE?")
		if not state:
			logging.info("Voltage protection state: OFF")
		device.write(":SOURCE:VOLTAGE:PROTECTION:LEV {0}".format(VoltProtection))
		device.write(":SOURCE:VOLTAGE:LEV:IMM:AMP {0}".format(voltage))
	except Exception as err:
		logger.error("Error occured while setting voltage level: {}".format(err))

def setComplianceLimit(device, compcurrent = 0.0):
	## Current limit should be passed as argument
	try:
		device.write(":SOURCE:CURR:PROTECTION:STATE ON")
		state = device.query(":SOURCE:CURR:PROTECTION:STATE?")
		if not state:
			logging.info("Current protection state: OFF")
		device.write(":SOURCE:CURR:PROTECTION:LEV {0}".format(compcurrent))
	except Exception as err:
		logger.error("Error occured while setting compliance: {}".format(err))

def ReadVoltage(device):
	try:
		MeasureVolt = device.query("MEASURE:VOLTAGE?")
		#MeasureVolt = float(Measure.split(',')[0])
		return MeasureVolt
	except Exception as err:
		logger.error("Error occured while reading voltage value: {}".format(err))

def ReadCurrent(device):
	try:
		MeasureCurr = device.query("MEASURE:VOLTAGE?")
		#MeasureCurr = float(Measure.split(',')[0])
		return MeasureCurr
	except Exception as err:
		logger.error("Error occured while reading current value: {}".format(err))
