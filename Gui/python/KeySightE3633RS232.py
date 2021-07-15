import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def InitailDevice(device):
	try:
		device.write("*RST")
	except Exception as err:
		logger.error("Error occured while restore defaults: {}".format(err))
	# Select voltage source mode

def GetInfo(device):
	try:
		print(device.query("*IDN?"))
	except Exception as err:
		logger.error("Error occured while requesting the identification: {}".format(err))

def TurnOn(device):
	try:
		device.write(":STATE  ON")
	except Exception as err:
		logger.error("Error occured while turning on the device: {}".format(err))

def TurnOff(device):
	try:
		device.write(":STATE OFF")
	except  Exception as err:
		logger.error("Error occured while turning off the device: {}".format(err))

def SetVoltage(device, voltage = 0.0, VoltProtection = 0.0):
	# Set Voltage range 2V and output to 1.78V
	try:
		device.write("SOURCE:VOLTAGE:PROTECTION:LEV {0}".format(VoltProtection))
		device.write("SOURCE:VOLTAGE:LEV:IMM:AMP {0}".format(voltage))
	except Exception as err:
		logger.error("Error occured while setting voltage level: {}".format(err))

def setComplianceLimit(device, compcurrent = 0.0):
	## Current limit should be passed as argument
	try:
		device.write(":SOURCE:CURR:PROTECTION:LEV 8")
	except Exception as err:
		logger.error("Error occured while setting compliance: {}".format(err))

def ReadVoltage(device):
	try:
		device.write(' MEASURE:VOLT ?')
		device.read_termination = '\r'
		device.write(':READ?')
		Measure = device.read()
		MeasureVolt = float(Measure.split(',')[0])
		return MeasureVolt
	except Exception as err:
		logger.error("Error occured while reading voltage value: {}".format(err))

def ReadCurrent(device):
	try:
		device.write(' MEASURE:CURR ?')
		device.read_termination = '\r'
		device.write(':READ?')
		Measure = device.read()
		MeasureCurr = float(Measure.split(',')[0])
		return MeasureCurr
	except Exception as err:
		logger.error("Error occured while reading current value: {}".format(err))
