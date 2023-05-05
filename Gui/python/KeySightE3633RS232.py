import logging
import time
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def InitialDevice(device):
	try:
		#device.write("*RST")
		device.write(":SYSTEM:REMOTE")
		#SetRemote(device)
	except Exception as err:
		logger.error("Error occured while restore defaults: {}".format(err))
	# Select voltage source mode

def Reset(device):
	try:
		device.write("*RST")
	except Exception as err:
		logger.error("Error while resetting {}".format(err))

def GetInfo(device):
	try:
		info =  device.query("*IDN?")
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

def ApplyCurrent(device, voltage=0.0, current=0.0):
	try:
		device.write("VOLT {}".format(voltage))
		device.write("CURR {}".format(current))
	except Exception as err:
		logger.error("Error occured while apply V = {0} V, I = {1} Amp".format(voltage,current))

def SetVoltage(device, voltage = 0.0, VoltProtection = 0.0):
	# Set Voltage range 2V and output to 1.78V
	try:
		#reply = device.write(":SOURCE:VOLTAGE:PROTECTION:STATE ON")
		#state = device.query(":SOURCE:VOLTAGE:PROTECTION:STATE?")
		#if not state:
		#	logging.info("Voltage protection state: OFF")
		#reply = device.write(":SOURCE:VOLTAGE:PROTECTION:LEV {0}".format(VoltProtection))
		#time.sleep(0.3)
		#reply = device.query(":SOURCE:VOLTAGE:PROTECTION:LEV?")
		#print(reply)
		#time.sleep(0.05)
		reply = device.write(":SOURCE:VOLTAGE:LEV:IMM {0}".format(voltage))
	except Exception as err:
		logger.error("Error occured while setting voltage level: {}".format(err))

def SetCurrent(device, current, isMax = False):
	try:
		if isMax:
			reply = device.write(":SOURCE:CURRENT:LEV:IMM 10")
		else:
			reply = device.write(":SOURCE:CURRENT:LEV:IMM {0}".format(current))
	except Exception as err:
		logger.error("Error occured while setting current level: {}".format(err))

def SetVoltageProtection(device, voltRange = 0):
	try:
		reply = device.write(":SOURCE:VOLTAGE:PROTECTION:LEV {0}".format(voltRange))
	except Exception as err:
		logger.error("Error occured while setting voltage level: {}".format(err))

def setComplianceLimit(device, compcurrent = 0.0):
	## Current limit should be passed as argument
	try:
		#device.write(":SOURCE:CURR:PROTECTION:STATE ON")
		#state = device.query(":SOURCE:CURR:PROTECTION:STATE?")
		#if not state:
		#	logging.info("Current protection state: OFF")
		device.write(":SOURCE:CURR:PROTECTION:LEV {0}".format(compcurrent))
	except Exception as err:
		logger.error("Error occured while setting compliance: {}".format(err))

def ReadVoltage(device):
	try:
		MeasureVolt = device.query("MEASURE:VOLTAGE?")
		#MeasureVolt = float(Measure.split(',')[0])
		return float(MeasureVolt)
	except Exception as err:
		logger.error("Error occured while reading voltage value: {}".format(err))

def ReadCurrent(device):
	try:
		MeasureCurr = device.query("MEASURE:CURRENT?")
		#MeasureCurr = float(Measure.split(',')[0])
		return float(MeasureCurr)
	except Exception as err:
		logger.error("Error occured while reading current value: {}".format(err))

def Status(device):
	try:
		status = device.query("STAT:QUES:COND?")
		return status
	except Exception as err:
		logger.error("Error occured while getting status: {}".format(err))
