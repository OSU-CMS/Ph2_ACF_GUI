import pyvisa as visa

import subprocess
import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class PowerSupply():
	def __init__(self,model = "Keithley", boardnumber = 0, primaryaddress = 24):
		self.Model = model
		self.Status = "OFF"
		self.deviceMap = {}
		#Not used for pyvisa
		#self.BoardNumber = int(boardnumber)
		#self.PrimaryAddress = int(primaryaddress)
		self.setResourceManager()
		self.setSCPIParser()
		
	
	def setResourceManager(self):
		self.ResourcesManager = visa.ResourceManager('@py')

	def setSCPIParser(self):
		import Gui.python.Keithley2400RS232 as SCPI

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
			self.Instrument = self.ResourcesManager.open_resource("{},read_termination='\r',write_termination='\r'".format(self.deviceMap[resourceName]))
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
				deviceName = usbInfo.decode("UTF-8").split(deviceId)[-1].lstrip(' ')
				if deviceName == None:
					logger.warning("No device name found for {}:".format(device))
					self.deviceMap[device] = device
				else:
					self.deviceMap[deviceName] = device
			except Exception as err:
				logger.error("Error found:{}".format(err))
				self.deviceMap[device] = device

	def getInfo(self):
		self.Instrument.query(b'*IDN?')
		
	def startRemoteCtl(self):
		pass

	def TurnOn(self):
		SCPI.InitailDevice(self.Instrument)
		SCPI.SetVoltage(self.Instrument)
		SCPI.setComplianceLimit(self.Instrument)
		SCPI.TurnOn(self.Instrument)

	def TurnOff(self):
		SCPI.TurnOff(self.Instrument)

	def ReadVoltage(self):
		SCPI.ReadVoltage(self.Instrument)

if __name__ == "__main__":
	power = PowerSupply()
	deviceList = power.listResources()
	print(deviceList)
