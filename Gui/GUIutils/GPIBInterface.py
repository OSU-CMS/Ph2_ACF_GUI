import pyvisa as visa

import importlib
import subprocess
import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class PowerSupply():
	def __init__(self,model = "Keithley", boardnumber = 0, primaryaddress = 24, powertype = "HV"):
		self.Model = model
		self.Status = "OFF"
		self.deviceMap = {}
		self.Instrument = None
		self.PowerType = powertype
		self.PoweringMode = None
		self.CompCurrent = None
		#Not used for pyvisa
		#self.BoardNumber = int(boardnumber)
		#self.PrimaryAddress = int(primaryaddress)
		self.setResourceManager()
		self.setSCPIParser()

	def setPowerType(self,powertype):
		if powertype != "HV" and powertype != "LV":
			logger.error("Power Type: {} not supported".format(powertype))
		else:
			self.PowerType = powertype

	def isHV(self):
		if self.PowerType == "HV":
			return True
		else:
			return False

	def isLV(self):
		if self.PowerType == "LV":
			return True
		else:
			return False
	
	def setPoweringMode(self, powermode="direct"):
		self.PoweringMode = powermode

	def setCompCurrent(self, compcurrent = 1.05):
		self.CompCurrent = compcurrent
	
	def setResourceManager(self):
		self.ResourcesManager = visa.ResourceManager('@py')

	def setSCPIParser(self):
		if self.Model == "Keithley" and self.isHV():
			self.SCPI = importlib.import_module("Gui.python.Keithley2400RS232")

		elif self.Model == "KeySight" and self.isLV():
			self.SCPI = importlib.import_module("Gui.python.KeySightE3633RS232")


	def listResources(self):
		try:
			self.ResourcesList = self.ResourcesManager.list_resources()
			self.getDeviceName()
			return list(self.deviceMap.keys())
		except Exception as err:
			logger.error("Failed to list all resources: {}".format(err))
			self.ResourcesList = ()
			return self.ResourcesList

	def setInstrument(self,resourceName):
		try:
			self.Instrument = self.ResourcesManager.open_resource("{}".format(self.deviceMap[resourceName]),read_termination='\r',write_termination='\r')
			self.setSCPIParser()
		except Exception as err:
			logger.error("Failed to open resource {0}: {1}".format(resourceName,err))

	def getDeviceName(self):
		self.deviceMap = {}
		for device in self.ResourcesList:
			try:
				pipe = subprocess.Popen(['udevadm', 'info', ' --query', 'all', '--name', device.lstrip("ASRL").rstrip("::INSTR"), '--attribute-walk'], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
				raw_output = pipe.communicate()[0]
				vendor_list = [info for info in  raw_output.splitlines() if b'ATTRS{idVendor}' in info or b'ATTRS{vendor}' in info]
				product_list = [info for info in  raw_output.splitlines() if b'ATTRS{idProduct}' in info or b'ATTRS{product}' in info]
				idvendor = vendor_list[0].decode("UTF-8").split("==")[1].lstrip('"').rstrip('"').replace('0x','')
				idproduct = product_list[0].decode("UTF-8").split("==")[1].lstrip('"').rstrip('"').replace('0x','')
				deviceId = "{}:{}".format(idvendor,idproduct)
				pipeUSB = subprocess.Popen(['lsusb','-d',deviceId], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
				usbInfo = pipeUSB.communicate()[0]
				deviceName = usbInfo.decode("UTF-8").split(deviceId)[-1].lstrip(' ').rstrip('\n')
				if deviceName == None:
					logger.warning("No device name found for {}:".format(device))
					self.deviceMap[device] = device
				else:
					self.deviceMap[deviceName] = device
			except Exception as err:
				logger.error("Error found:{}".format(err))
				self.deviceMap[device] = device

	def getInfo(self):
		try:
			info = self.SCPI.GetInfo(self.Instrument)
		except Exception as err:
			logging.error("Failed to get instrument information:{}".format(err))

	def startRemoteCtl(self):
		pass

	def TurnOn(self):
		try:
			self.SCPI.InitailDevice(self.Instrument)
			self.SCPI.SetVoltage(self.Instrument)
			self.SCPI.setComplianceLimit(self.Instrumentt)
			self.SCPI.TurnOn(self.Instrument)
		except Exception as err:
			logging.error("Failed to turn on the sourceMeter:{}".format(err))

	def TurnOff(self):
		try:
			self.SCPI.TurnOff(self.Instrument)
		except Exception as err:
			logging.error("Failed to turn off the sourceMeter:{}".format(err))

	def ReadVoltage(self):
		self.SCPI.ReadVoltage(self.Instrument)

	def ReadCurret(self):
		self.SCPI.ReadCurrent(self.Instrument)

	def RampingUp(self, hvTarget = 0.0, stepLength = 1.0):
		try:
			if self.isHV():
				self.SCPI.RampingUpVoltage(self.Instrument,hvTarget,stepLength)
			else:
				logging.info("Not a HV power supply, abort")
		except Exception as err:
			logging.error("Failed to ramp the voltage to {0}:{1}".format(hvTarget,err))

if __name__ == "__main__":
	power = PowerSupply()
	deviceList = power.listResources()
	print(deviceList)
