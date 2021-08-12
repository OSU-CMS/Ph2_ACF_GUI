import pyvisa as visa

import importlib
import subprocess
import logging

from Gui.GUIutils.settings import *
from Configuration.XMLUtil import *

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
		self.CompCurrent = 0.0
		self.UsingPythonInterface = False
		self.XMLConfig = None
		self.Port = None
		self.DeviceNode = None
		#Not used for pyvisa
		#self.BoardNumber = int(boardnumber)
		#self.PrimaryAddress = int(primaryaddress)
		self.initPowerSupply()
		self.setResourceManager()
		#self.setSCPIParser()

	def initPowerSupply(self):
		if os.environ.get("PowerSupplyArea") != None:
			if os.path.isdir(os.environ.get("PowerSupplyArea")):
				self.UsingPythonInterface = False
			else:
				self.UsingPythonInterface = True
		else:
			self.UsingPythonInterface = True

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

	def setPowerModel(self, model):
		self.Model = model
	
	def setPoweringMode(self, powermode="direct"):
		self.PoweringMode = powermode

	def setCompCurrent(self, compcurrent = 1.05):
		self.CompCurrent = compcurrent
	
	def setResourceManager(self):
		self.ResourcesManager = visa.ResourceManager('@py')

	def setSCPIParser(self):
		if self.UsingPythonInterface == False:
			return None
		if self.isHV():
			self.SCPI = importlib.import_module(HVPowerSupplyModel[self.Model])

		elif self.isLV():
			self.SCPI = importlib.import_module(LVPowerSupplyModel[self.Model])

	def setDeviceXMLConfig(self):
		ChannelFront = Channel()
		PowerSupply0 = PowerSupplyNode()
		PowerSupply0.ID = "{0}{1}".format(self.PowerType,self.Model.split()[0])
		PowerSupply0.Port = self.Port
		PowerSupply0.Model = self.Model.split()[0]
		PowerSupply0.Terminator = PowerSupplyModel_Termination[self.Model]
		PowerSupply0.addChannel(ChannelFront)
		Device0 = Device()
		Device0.setPowerSupply(PowerSupply0)
		self.DeviceNode = Device0
		return Device0
		

	def generateXMLConfig(self):
		if self.UsingPythonInterface == False:
			try:
				XMLConfigFile = os.environ.get('GUI_dir')+"/Gui/.tmp/{0}PowerSupply.xml".format(self.PowerType)
				deviceConfig = self.setDeviceXMLConfig()
				GeneratePowerSupplyXML(deviceConfig,outputFile=XMLConfigFile)
				self.XMLConfig = XMLConfigFile
				return True
			except Exception as err:
				logger.error("Error: XML configuration not generated properly with {0}".format(err))
				return None
		else:
			logger.info("Using python interface for power supply control")
			return None


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
		if self.UsingPythonInterface == True:
			try:
				self.Instrument = self.ResourcesManager.open_resource("{}".format(self.deviceMap[resourceName]))
				self.Instrument.read_termination=PowerSupplyModel_Termination[self.Model]
				self.Instrument.write_termination=PowerSupplyModel_Termination[self.Model]
				self.Port = resourceName.lstrip("ASRL").rstrip("::INSTR")
				self.setSCPIParser()
			except Exception as err:
				logger.error("Failed to open resource {0}: {1}".format(resourceName,err))

		else:
			self.Port=resourceName.lstrip("ASRL").rstrip("::INSTR")
			self.generateXMLConfig()



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
		if self.UsingPythonInterface == True:
			try:
				info = self.SCPI.GetInfo(self.Instrument)
				return info
			except Exception as err:
				logging.error("Failed to get instrument information:{}".format(err))
				return "No valid device"
		else:
			try:
				pass
			except Exception as err:
				logging.error("Failed to get instrument information:{}".format(err))
				return "No valid device"

	def startRemoteCtl(self):
		pass

	def TurnOn(self):
		if self.UsingPythonInterface == True:
			try:
				self.SCPI.InitialDevice(self.Instrument)
				Voltage = 0.0
				VoltProtection = 1.0
				if self.PowerType ==  "LV" and self.PoweringMode == "direct":
					Voltage = 1.18
					VoltProtection = 1.2
				if self.PowerType == "LV" and self.PoweringMode == "LDO":
					Voltage = 1.78
					VoltProtection = 1.8
				self.SCPI.SetVoltage(self.Instrument, voltage = Voltage, VoltProtection = VoltProtection)
				self.SCPI.setComplianceLimit(self.Instrument,self.CompCurrent)
				self.SCPI.TurnOn(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
		else:
			pass

	def TurnOff(self):
		if self.UsingPythonInterface == True:
			try:
				self.SCPI.TurnOff(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn off the sourceMeter:{}".format(err))
		else:
			pass

	def ReadVoltage(self):
		if self.UsingPythonInterface == True:
			self.SCPI.ReadVoltage(self.Instrument)
		else:
			pass

	def ReadCurret(self):
		if self.UsingPythonInterface == True:
			self.SCPI.ReadCurrent(self.Instrument)
		else:
			pass

	def RampingUp(self, hvTarget = 0.0, stepLength = 1.0):
		try:
			if self.isHV():
				self.SCPI.RampingUpVoltage(self.Instrument,hvTarget,stepLength)
			else:
				logging.info("Not a HV power supply, abort")
		except Exception as err:
			logging.error("Failed to ramp the voltage to {0}:{1}".format(hvTarget,err))

if __name__ == "__main__":
	power = PowerSupply(powertype = "LV")
	deviceList = power.listResources()
	print(deviceList)
