import pyvisa as visa

import importlib
import subprocess
import logging

from Gui.GUIutils.settings import *
from Configuration.XMLUtil import *
from Gui.python.TCP_Interface import *

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
		self.Answer = None
		#Not used for pyvisa
		#self.BoardNumber = int(boardnumber)
		#self.PrimaryAddress = int(primaryaddress)
		self.initPowerSupply()
		self.setResourceManager()
		#self.sethwInterfaceParser()

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
		return self.PowerType == "HV"

	def isLV(self):
		return self.PowerType == "LV"

	def setPowerModel(self, model):
		self.Model = model
	
	def setPoweringMode(self, powermode="Direct"):
		self.PoweringMode = powermode

	def setCompCurrent(self, compcurrent = 1.05):
		self.CompCurrent = compcurrent
	
	def setResourceManager(self):
		self.ResourcesManager = visa.ResourceManager('@py')

	def sethwInterfaceParser(self):
		if self.UsingPythonInterface == False:
			return None
		if self.isHV():
			self.hwInterface = importlib.import_module(HVPowerSupplyModel[self.Model])

		elif self.isLV():
			self.hwInterface = importlib.import_module(LVPowerSupplyModel[self.Model])

	def setDeviceXMLConfig(self):
		try:
			ChannelFront = Channel()
			PowerSupply0 = PowerSupplyNode()
			self.ID = "{0}{1}".format(self.PowerType,self.Model.split()[0])
			PowerSupply0.ID = self.ID
			PowerSupply0.Port = self.Port
			PowerSupply0.Model = self.Model.split()[0]
			PowerSupply0.Suffix = PowerSupplyModel_XML_Termination[self.Model]
			PowerSupply0.Terminator = PowerSupplyModel_XML_Termination[self.Model]
			PowerSupply0.addChannel(ChannelFront)
			Device0 = Device()
			Device0.setPowerSupply(PowerSupply0)
			self.DeviceNode = Device0
			return Device0
		except Exception as err:
			logger.error("Error: Device not set properly in XML format with {0}".format(err))
			return None
		

	def generateXMLConfig(self):
		if self.UsingPythonInterface == False:
			try:
				XMLConfigFile = os.environ.get('GUI_dir')+"/power_supply/{0}PowerSupply.xml".format(self.PowerType)
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
				if resourceName in self.deviceMap.keys():
					self.Instrument = self.ResourcesManager.open_resource("{}".format(resourceName))
					self.Port = self.deviceMap[resourceName].lstrip("ASRL").rstrip("::INSTR")
				elif resourceName in self.deviceMap.values():
					self.Instrument = self.ResourcesManager.open_resource("{}".format(resourceName))
				else:
					self.Instrument = self.ResourcesManager.open_resource("{}".format(resourceName))
				self.Instrument.read_termination=PowerSupplyModel_Termination[self.Model]
				self.Instrument.write_termination=PowerSupplyModel_Termination[self.Model]
				self.sethwInterfaceParser()
			except Exception as err:
				logger.error("Failed to open resource {0}: {1}".format(resourceName,err))

		else:
			self.Port=resourceName.lstrip("ASRL").rstrip("::INSTR")
			self.generateXMLConfig()
			self.createInterface(self.XMLConfig)

	def createInterface(self, xmlFile):
		self.hwInterface = TCP_Interface(os.environ.get('GUI_dir')+"/power_supply", xmlFile)
		self.hwInterface.update.connect(self.hwUpdate)

	def hwUpdate(self, pHead, pAnswer):
		if pAnswer is not None:
			self.Answer = pAnswer
			logger.info("TCP: PowerSupply {} - {}:{}".format(self.PowerType, pHead,pAnswer))
		else:
			self.Answer = None

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
					self.deviceMap[device] = deviceName
			except Exception as err:
				logger.error("Error found:{}".format(err))
				self.deviceMap[device] = device

	def getInfo(self):
		if self.UsingPythonInterface == True:
			try:
				info = self.hwInterface.GetInfo(self.Instrument)
				return info
			except Exception as err:
				logging.error("Failed to get instrument information:{}".format(err))
				return "No valid device"
		else:
			try:
				cmd = "GetInfo,PowerSupplyId:" + self.ID
				self.hwInterface.executeCommand(cmd)
				return self.Answer
			except Exception as err:
				logging.error("Failed to get instrument information:{}".format(err))
				return "No valid device"

	def startRemoteCtl(self):
		pass

	def TurnOn(self):
		self.setCompCurrent()
		if self.UsingPythonInterface == True:
			#try:
				self.hwInterface.InitialDevice(self.Instrument)
				Voltage = 0.0
				VoltProtection = 1.0
				if self.PowerType ==  "LV" and self.PoweringMode == "Direct":
					Voltage = 1.28
					VoltProtection = 1.3
				if self.PowerType == "LV" and self.PoweringMode == "SLDO":
					Voltage = 1.78
					VoltProtection = 1.8
				if self.PowerType == "LV":
					logging.info("Setting LV to {}".format(Voltage))
					#self.Instrument.write(":SOURCE:VOLTAGE:PROTECTION:LEV {0}".format(VoltProtection))
					#self.Instrument.write(":SOURCE:VOLTAGE:LEV:IMM {0}".format(Voltage))
				self.hwInterface.SetVoltage(self.Instrument, voltage = Voltage, VoltProtection = VoltProtection)
				#self.hwInterface.setComplianceLimit(self.Instrument,self.CompCurrent)
				self.hwInterface.TurnOn(self.Instrument)
			#except Exception as err:
			#	logging.error("Failed to turn on the sourceMeter:{}".format(err))
		else:
			try:
				Voltage = 0.0
				VoltProtection = 1.0
				if self.PowerType ==  "LV" and self.PoweringMode == "Direct":
					Voltage = 1.18
					VoltProtection = 1.2
				if self.PowerType == "LV" and self.PoweringMode == "SLDO":
					Voltage = 1.78
					VoltProtection = 1.8
				# Setting Voltage
				cmd = "SetVoltage,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(Voltage)
				self.hwInterface.executeCommand(cmd)
				# Setting current compliance
				cmd = "SetCurrentCompliance,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(self.CompCurrent)
				self.hwInterface.executeCommand(cmd)
				cmd = "TurnOn,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
				self.hwInterface.executeCommand(cmd)
				return self.Answer
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
				return None


	def TurnOff(self):
		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.TurnOff(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn off the sourceMeter:{}".format(err))
		else:
			try:
				cmd = "TurnOff,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
				self.hwInterface.executeCommand(cmd)
			except Exception as err:
				logging.error("Failed to turn off the sourceMeter:{}".format(err))

	def ReadVoltage(self):
		if self.UsingPythonInterface == True:
			try:
				voltage = self.hwInterface.ReadVoltage(self.Instrument)
				return voltage
			except Exception as err:
				return None
		else:
			pass

	def SetVoltage(self, voltage = 0.0):
		if self.powertype == "HV":
			return

		if self.UsingPythonInterface ==  True:
			try:
				pass
			except Exception as err:
				logging.err("Failed to set {} voltage to {}".format(self.powertype, voltage))
		else:
			try:
				pass
			except Exception as err:
				logging.err("Failed to set {} voltage to {}".format(self.powertype, voltage))


	def ReadCurrent(self):
		if self.UsingPythonInterface == True:
			try:
				current = self.hwInterface.ReadCurrent(self.Instrument)
				return current
			except Exception as err:
					pass
		else:
			try:
				cmd = "GetCurrent,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
				current = self.hwInterface.executeCommand(cmd)
				return current
			except Exception as err:
				logging.error("Failed to retrive current")
	
	def TurnOnHV(self):
		if not self.isHV():
			logging.info("Try to turn on non-HV as high voltage")
			return

		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.SetVoltage(self.Instrument)
				self.hwInterface.TurnOn(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
				return None
		else:
			cmd = "TurnOn,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
			self.hwInterface.executeCommand(cmd)

	def SetHVRange(self, voltRange):
		if not self.isHV():
			logging.info("Try to setVoltage for non-HV power supply")
			return

		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.SetVoltageProtection(self.Instrument,voltRange)
			except Exception as err:
				logging.error("Failed to set range for the sourceMeter:{}".format(err))
				return None
		else:
			pass

	def SetHVVoltage(self, voltage):
		if not self.isHV():
			logging.info("Try to setVoltage for non-HV power supply")
			return

		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.SetVoltage(self.Instrument,voltage)
			except Exception as err:
				logging.error("Failed to set HV target the sourceMeter:{}".format(err))
				return None
		else:
			cmd = "SetVoltage,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(voltage)
			self.hwInterface.executeCommand(cmd)
	
	def SetHVComplianceLimit(self, compliance):
		if not self.isHV():
			logging.info("Try to setVoltage for non-HV power supply")
			return

		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.setComplianceLimit(self.Instrument,compliance)
			except Exception as err:
				logging.error("Failed to set compliance limit for the sourceMeter:{}".format(err))
				return None
		else:
			# Setting current compliance
			cmd = "SetCurrentCompliance,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(compliance)
			self.hwInterface.executeCommand(cmd)
		


	def RampingUp(self, hvTarget = 0.0, stepLength = 1.0):
		try:
			if self.isHV():
				self.hwInterface.RampingUpVoltage(self.Instrument,hvTarget,stepLength)
			else:
				logging.info("Not a HV power supply, abort")
		except Exception as err:
			logging.error("Failed to ramp the voltage to {0}:{1}".format(hvTarget,err))

	def customized(self,cmd):
		if "Keith" in self.Model:
			cmd = "K2410:" + cmd
		return cmd

if __name__ == "__main__":
	power = PowerSupply(powertype = "LV")
	deviceList = power.listResources()
	print(deviceList)
