import logging
import time
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def InitialDevice(device):
	try:
		print("initializing device")
		device.write("*RST")
	except Exception as err:
		logger.error("Error occured while restore defaults: {}".format(err))
	# Select voltage source mode
	try:
		print("this was where the thing was fixed")
		device.write(":SOURCE:FUNCTION VOLT")
		device.write(":SOURCE:VOLTAGE:MODE FIX")
	except Exception as err:
		logger.error("Error occured while setting voltage source mode: {}".format(err))

def GetInfo(device):
	try:
		info =  device.query("*IDN?")
		print(info)
		return info
	except Exception as err:
		logger.error("Error occured while requesting the identification: {}".format(err))

def TurnOn(device):
	try:
		device.write(":OUTPUT  ON")
	except Exception as err:
		logger.error("Error occured while turning on the device: {}".format(err))

def TurnOff(device):
	try:
		status = device.query(":OUTPUT?")
		print("HV status is {0}".format(status))
		#device.write(":SOURCE:VOLTAGE:LEV 0")
		device.write(":OUTPUT OFF")
	except  Exception as err:
		logger.error("Error occured while turning off the device: {}".format(err))

def SetVoltageProtection(device,voltProtection = 0.0):
	try:
		device.write(":SOURCE:VOLTAGE:RANGE {0}".format(voltProtection))
	except Exception as err:
		logger.error("Error occured while setting voltage range: {}".format(err))

def SetVoltage(device, voltage = 0.0):
	# Set Voltage range 2V and output to 1.78V
	try:
		device.write(":SOURCE:VOLTAGE:LEV {0}".format(voltage))
	except Exception as err:
		logger.error("Error occured while setting voltage level: {}".format(err))

def setComplianceLimit(device, compcurrent = 0.0):
	try:
		device.write(":SENSE:CURR:PROT {0}".format(compcurrent))
	except Exception as err:
		logger.error("Error occured while setting compliance: {}".format(err))
def ReadOutputStatus(device):
	device.write(":OUTPUT?")
	outputstatus = device.read()
	return outputstatus

def ReadVoltage(device):
	try:
		device.write(":FORM:ELEM VOLT")
		device.write(' :SENSE:FUNCTION "VOLT" ')
		#device.read_termination = '\r'
		device.write(':READ?')
		Measure = device.read()
		MeasureVolt = float(Measure)
		return MeasureVolt
	except Exception as err:
		logger.error("Error occured while reading voltage value: {}".format(err))

def ReadCurrent(device):
	try:
		device.write(":FORM:ELEM CURR")
		device.write(' :SENSE:FUNCTION "CURR" ')
		#device.read_termination = '\r'
		device.write(':READ?')
		Measure = device.read()
		MeasureCurr = float(Measure)
		return MeasureCurr
	except Exception as err:
		logger.error("Error occured while reading current value: {}".format(err))

def RampingUpVoltage(device, hvTarget, stepLength):
	try:
		device.write(":FORM:ELEM VOLT")
		device.write(' :SENSE:FUNCTION "VOLT" ')
		#device.read_termination = '\r'
		device.write(':READ?')
		Measure = device.read()
		currentVoltage = float(Measure)

		if hvTarget < currentVoltage:
			stepLength = -abs(stepLength)
		
		for voltage in range(int(currentVoltage), int(hvTarget), int(stepLength)):
			device.write(":SOURCE:VOLTAGE:LEV {0}".format(voltage))
			time.sleep(0.3)
		device.write(":SOURCE:VOLTAGE:LEV {0}".format(hvTarget))
	except Exception as err:
		logger.error("Error occured while ramping up the voltage to {0}, {1}".format(hvTarget,err))