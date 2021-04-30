import pyvisa as visa

import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class PowerSupply():
	def __init__(self,model = "Keithley", boardnumber = 0, primaryaddress = 24):
		self.Model = model
		self.Status = "OFF"
		#Not used for pyvisa
		#self.BoardNumber = int(boardnumber)
		#self.PrimaryAddress = int(primaryaddress)
		self.setResourceManager()
		
	
	def setResourceManager(self):
		self.ResourcesManager = visa.ResourceManager('@py')

	def listResources(self):
		try:
			self.ResourcesList = self.ResourcesManager.list_resources()
			return self.ResourcesList
		except Exception as err:
			logger.error("Failed to list all resources: {}".format(err))
			self.ResourcesList = ()
			return self.ResourcesList

	def setInstrument(self,resourceName):
		if "Keithley" in resourceName:
			pass
		try:
			self.Instrument = self.ResourcesManager.open_resource("{}".format(resourceName))
		except Exception as err:
			logger.error("Failed to open resource {0}: {1}".format(resourceName,err))

	def getInfo(self):
		self.Instrument.query(b'*IDN?')
		
	def startRemoteCtl(self):
		pass