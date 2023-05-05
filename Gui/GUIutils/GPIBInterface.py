import pyvisa as visa

import importlib
import subprocess
import logging


from Gui.GUIutils.settings import *
from Configuration.XMLUtil import *
from Gui.python.TCP_Interface import *
from Gui.siteSettings import *

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

if GPIB_DebugMode:
	visa.log_to_screen()

class PowerSupply():
	def __init__(self,model = "Keithley", boardnumber = 0, primaryaddress = 24, powertype = "HV",serverIndex = 0):
		self.Model = model
		self.Status = "OFF"
		self.deviceMap = {}
		self.Instrument = None
		self.PowerType = powertype
		self.PoweringMode = None
		self.ModuleType = None
		self.CompCurrent = 0.0
		self.UsingPythonInterface = False
		self.XMLConfig = None
		self.Port = None
		self.DeviceNode = None
		self.Answer = None
		self.maxTries = 10
		self.ServerIndex = serverIndex
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
	
	def setModuleType(self, moduletype = None):
		self.ModuleType = moduletype
	
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
			self.sethwInterfaceParser()
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

			except Exception as err:
				logger.error("Failed to open resource {0}: {1}".format(resourceName,err))

		else:
			self.Port=resourceName.lstrip("ASRL").rstrip("::INSTR")
			self.generateXMLConfig()
			self.createInterface(self.XMLConfig)

	def createInterface(self, xmlFile):
		self.hwInterface = TCP_Interface(os.environ.get('GUI_dir')+"/power_supply", xmlFile, self.ServerIndex)
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
				self.Answer = None
				nTry = 0
				cmd = "GetInfo,PowerSupplyId:" + self.ID
				self.hwInterface.executeCommand(cmd)
				while self.Answer == None and nTry < self.maxTries:
					time.sleep(0.1)
					nTry += 1
				return self.Answer
			except Exception as err:
				logging.error("Failed to get instrument information:{}".format(err))
				return "No valid device"

	def startRemoteCtl(self):
		pass

	def TurnOn(self):
		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.InitialDevice(self.Instrument)
				Voltage = 0.0
				Current = 0.0
				if self.PowerType ==  "LV" and self.PoweringMode == "SLDO":
					Voltage = ModuleVoltageMapSLDO[self.ModuleType]
					Current = ModuleCurrentMap[self.ModuleType]
				elif self.PowerType ==  "LV" and self.PoweringMode == "Direct":
					Voltage = ModuleVoltageMap[self.ModuleType]
					Current = ModuleCurrentMap[self.ModuleType]
					#self.hwInterface.setComplianceLimit(self.Instrument,self.CompCurrent)
				self.hwInterface.ApplyCurrent(self.Instrument, voltage = Voltage, current = Current)
				#self.hwInterface.setComplianceLimit(self.Instrument,self.CompCurrent)
				self.hwInterface.TurnOn(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
		else:
			try:
				self.InitialDevice()
				time.sleep(1)
				self.Answer = None
				nTry = 0
				Voltage = 0.0
				VoltProtection = 1.0
				if self.PowerType ==  "LV" and self.PoweringMode == "Direct":
					Voltage = ModuleVoltageMap[self.ModuleType]
					Current = ModuleCurrentMap[self.ModuleType]
					self.CompCurrent = Current
				if self.PowerType == "LV" and self.PoweringMode == "SLDO":
					Voltage = ModuleVoltageMapSLDO[self.ModuleType]
					Current = ModuleCurrentMap[self.ModuleType]
					self.CompCurrent = Current
				cmd = "SetCurrent,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(Current)
				# Setting Voltage
				cmd = "SetVoltageCompliance,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(Voltage)
				self.hwInterface.executeCommand(cmd)
				time.sleep(1)
				# Setting current compliance
				#cmd = "SetCurrentCompliance,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(self.CompCurrent)
				self.hwInterface.executeCommand(cmd)
				time.sleep(1)
				cmd = "TurnOn,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
				self.hwInterface.executeCommand(cmd)

				time.sleep(1)
				cmd = "GetOutputVoltage,PowerSupplyId:" + self.ID + ",ChannelId:Front"
				self.hwInterface.executeCommand(cmd)
				while self.Answer == None and nTry < self.maxTries:
					time.sleep(0.1)
					nTry += 1
				self.Answer = -99 if self.Answer == None else self.Answer
				if abs(float(self.Answer) - float(Voltage)) > 0.1:
					logging.warning("Voltage not consistent with setting value")
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
				return None

	# TurnOnLV is only used for SLDO Scan
	def TurnOnLV(self):
		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.TurnOn(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
		else:
			try:
				cmd = "TurnOn,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
				self.hwInterface.executeCommand(cmd)
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter via TCP:{}".format(err))


	def InitialDevice(self):
		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.InitialDevice(self.Instrument)
			except Exception as err:
				logging.error("Failed to initial the device:{}".format(err))
		else:
			try:
				cmd = "Initialize,PowerSupplyId:" + self.ID + ",PowerSupplyType:" +self.Model.split()[0]
				self.hwInterface.executeCommand(cmd)
			except Exception as err:
				logging.error("Failed to initial the device through TCP:{}".format(err))


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

	def ReadOutputStatus(self):
		if self.UsingPythonInterface == True:
			try:
				HVoutputstatus = self.hwInterface.ReadOutputStatus(self.Instrument)
				return HVoutputstatus
			except Exception as err:
				return None
		else:
			try:
				cmd = "GetStatus,PowerSupplyId:" + self.ID
				HVoutputstatus = self.hwInterface.executeCommand(cmd)
				return HVoutputstatus
			except Exception as err:
				return None
	
	def ReadVoltage(self):
		if self.UsingPythonInterface == True:
			try:
				voltage = self.hwInterface.ReadVoltage(self.Instrument)
				return voltage
			except Exception as err:
				return None
		else:
			try:
				self.Answer = None
				nTry = 0
				cmd = "GetOutputVoltage,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
				self.hwInterface.executeCommand(cmd)
				while self.Answer == None and nTry < self.maxTries:
					time.sleep(0.1)
					nTry += 1
					self.hwInterface.executeCommand(cmd)
				if self.Answer != None:
					return float(self.Answer)
				else:
					return -1
			except Exception as err:
				logging.error("Failed to retrive voltage")

	def SetVoltage(self, voltage = 0.0):
		if self.PowerType == "HV":
			return

		if self.UsingPythonInterface ==  True:
			try:
				self.hwInterface.SetVoltage(self.Instrument,voltage)
			except Exception as err:
				logging.err("Failed to set {} voltage to {}".format(self.PowerType, voltage))
		else:
			try:
				cmd = "SetVoltage,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(voltage)
				self.hwInterface.executeCommand(cmd)
				readBack = self.ReadVoltage()
				if abs(readBack - voltage) > 0.05:
					logging.warning("Inconsistence detected between target voltage and measured voltage")
			except Exception as err:
				logging.err("Failed to set {} voltage to {}".format(self.PowerType, voltage))

	def SetRange(self, voltRange):
		if not self.isLV():
			logging.info("Try to setVoltage for non-LV power supply")
			return

		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.SetVoltageProtection(self.Instrument,voltRange)
			except Exception as err:
				logging.error("Failed to set range for the sourceMeter:{}".format(err))
				return None
		else:
			cmd = "setOverVoltageProtection,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(voltRange)
			self.hwInterface.executeCommand(cmd)

	def SetCurrent(self, current = 0.0):
		if self.PowerType == "HV":
			return

		if self.UsingPythonInterface ==  True:
			try:
				self.hwInterface.SetCurrent(self.Instrument,current)
			except Exception as err:
				logging.error("Failed to set {} current to {}".format(self.PowerType, current))
		else:
			try:
				cmd = "SetCurrent,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(current)
				self.hwInterface.executeCommand(cmd)
				readBack = self.ReadCurrent()
				if abs(readBack - current) > 0.05:
					logging.warning("Inconsistence detected between target voltage and measured voltage")
			except Exception as err:
				logging.error("Failed to set {} current to {}".format(self.PowerType, current))

	def ReadCurrent(self):
		if self.UsingPythonInterface == True:
			try:
				current = self.hwInterface.ReadCurrent(self.Instrument)
				return current
			except Exception as err:
					pass
		else:
			try:
				self.Answer = None
				nTry = 0
				cmd = "GetCurrent,PowerSupplyId:" + self.ID + ",ChannelId:Front"
				self.hwInterface.executeCommand(cmd)
				while self.Answer == None and nTry < self.maxTries:
					time.sleep(0.1)
					nTry += 1
					self.hwInterface.executeCommand(cmd)

				if self.Answer != None:
					return float(self.Answer)
				else:
					return -1
			except Exception as err:
				logging.error("Failed to retrive current")
	
	def TurnOnHV(self):
		if not self.isHV():
			logging.info("Try to turn on non-HV as high voltage")
			return

		if self.UsingPythonInterface == True:
			try:
				self.hwInterface.InitialDevice(self.Instrument)
				HVstatus = self.hwInterface.ReadOutputStatus(self.Instrument)
				if '1' in HVstatus:
					print('found HV status {0}'.format(HVstatus))
					self.TurnOffHV()
				self.hwInterface.SetVoltage(self.Instrument)
				self.hwInterface.TurnOn(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn on the sourceMeter:{}".format(err))
				return None
		else:
			#cmd = "Initialize,PowerSupplyId:" + self.ID + ",PowerSupplyType:" +self.Model.split()[0]
			#self.hwInterface.executeCommand(cmd)
			#self.InitialDevice()
			#cmd = "GetStatus,PowerSupplyID:" + self.ID
			#HVoutputstatus = self.hwInterface.executeCommand(cmd)
			#HVoutputstatus = self.ReadOutputStatus()
			#if '1' in HVoutputstatus:
			#	print('HV status was ON.  Turning OFF HV now.')
			#	self.TurnOffHV()
			self.ReadVoltage()
			cmd = "TurnOn,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
			self.hwInterface.executeCommand(cmd)
			

	def TurnOffHV(self):
		if not self.isHV():
			logging.info("Try to turn off non-HV as high voltage")
			return
		
		if self.UsingPythonInterface == True:
			try:
				HVstatus = self.hwInterface.ReadOutputStatus(self.Instrument)
				if '0' in HVstatus:
					return
				currentVoltage = self.hwInterface.ReadVoltage(self.Instrument)
				stepLength = 3
				if 0 < currentVoltage:
					stepLength = -3
		
				for voltage in range(int(currentVoltage),0,stepLength):
					self.hwInterface.SetVoltage(self.Instrument, voltage)
					time.sleep(0.3)
				self.hwInterface.SetVoltage(self.Instrument)
				self.hwInterface.TurnOff(self.Instrument)
			except Exception as err:
				logging.error("Failed to turn off the sourceMeter:{}".format(err))
				return None
		else:
			#self.InitialDevice()
			#self.ReadOutputStatus()
			#currentvoltage = self.ReadVoltage()
			#print('output voltage is {0}'.format(currentvoltage))
			#stepLength = 3
			#if currentvoltage != 0:
			#	for voltage in range(currentvoltage, 0 - stepLength, -stepLength):
			#		self.SetHVVoltage(voltage)
			#		time.sleep(0.3)

			cmd = "TurnOff,PowerSupplyId:" + self.ID + ",ChannelId:Front" 
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
			cmd = "setOverVoltageProtection,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(voltRange)
			self.hwInterface.executeCommand(cmd)

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
			#cmd = "rampToVoltage,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value" + str(voltage)
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
		


	def RampingUp(self, hvTarget = 0.0, stepLength = 0.0):
		if self.isHV():
			try:
				if self.UsingPythonInterface == True:
					HVstatus = self.hwInterface.ReadOutputStatus(self.Instrument)
					if '1' in HVstatus:
						self.TurnOffHV()
					self.hwInterface.InitialDevice(self.Instrument)
					self.SetHVComplianceLimit(defaultHVCurrentCompliance)
					self.hwInterface.SetVoltage(self.Instrument)
					self.hwInterface.TurnOn(self.Instrument)
					self.hwInterface.RampingUpVoltage(self.Instrument,hvTarget,stepLength)

				else:
					self.InitialDevice()
					self.SetHVComplianceLimit(defaultHVCurrentCompliance)
					currentvoltage = self.ReadVoltage()
					if currentvoltage != 0:
						for voltage in range(currentvoltage, 0 - stepLength, -stepLength):
							self.SetHVVoltage(voltage)
							time.sleep(0.3)
						self.TurnOffHV()
					self.TurnOnHV()
					for voltage in range(0, hvTarget + stepLength, stepLength):
						self.SetHVVoltage(voltage)
						time.sleep(0.3)
						cmd = "GetCurrent,PowerSupplyId:" + self.ID + ",ChannelId:Front"
						cmd = self.customized(cmd)
						self.hwInterface.executeCommand(cmd)
			
			except Exception as err:
				logging.error("Failed to ramp the voltage to {0}:{1}".format(hvTarget,err))
		else:
				logging.info("Not a HV power supply, abort")

	def customized(self,cmd):
		if "Keith" in self.Model:
			cmd = "K2410:" + cmd
		return cmd

	def Status(self):
		if not "KeySight" in self.Model:
			return -1
		
		if self.UsingPythonInterface == True:
			try:
				reply = self.hwInterface.Status(self.Instrument)
				return 1
			except Exception as err:
				logging.error("Failed to get the status code:{}".format(err))
				return None
		else:
			# Setting current compliance
			cmd = "SetCurrentCompliance,PowerSupplyId:" + self.ID + ",ChannelId:Front,Value:"+ str(compliance)
			self.hwInterface.executeCommand(cmd)
			#return self.Answer
			return 1

	def Reset(self):
		if not "KeySight" in self.Model:
			return -1
		if self.UsingPythonInterface == True:
			try:
				reply = self.hwInterface.Reset(self.Instrument)
				return 1
			except Exception as err:
				logging.error("Failed to get the status code:{}".format(err))
				return None
		else:
			pass
			return 1

if __name__ == "__main__":
	power = PowerSupply(powertype = "LV")
	deviceList = power.listResources()
	print(deviceList)
