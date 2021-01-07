from Gui.GUIutils.settings import *

class QtChip():
	def __init__(self):
		self.__chipID = ""


class QtModule():
	def __init__(self, **kwargs):
		self.__moduleID =  ""
		self.__moduleType = "SingleSCC"
		self.__chipDict = {}
		for key,value in kwargs.items():
			if key == "id":
				self.__moduleID = str(value)
			if key == "type":
				self.__moduleType = str(value)

		self.setupChips()

	def setModuleID(self, id):
		self.__moduleID = id
	
	def getModuleID(self):
		return self.__moduleID

	def setModuleType(self, fwType):
		if fwType in ModuleType.values():
			self.__moduleType = fwType
		else:
			self.__moduleType = "SingleSCC"
		self.setupChips()

	def getModuleType(self):
		return self.__moduleType

	def setupChips(self, **kwargs):
		self.__chipDict = {}
		if "chips" in kwargs.keys():
			pass
			return
		for i in range(BoxSize[self.__moduleType]):
			FEChip = QtChip()
			self.__chipDict[i] = FEChip
	

class QtBeBoard():
	def __init__(self, boardName = ""):
		self.__boardName= ""
		self.__ipAddress = "0.0.0.0"
		self.__moduleDict = {}
		self.__fpgaConfigName = ""

	def setBoardName(self, name):
		self.__boardName = name

	def getBoardName(self):
		return self.__boardName

	def setIPAddress(self, ipAddress):
		self.__ipAddress = ipAddress
		
	def getIPAddress(self):
		return self.__ipAddress

	def setFPGAConfig(self, fpgaConfig):
		self.__fpgaConfigName = fpgaConfig
		return True
	
	def addModule(self, key, module):
		if module not in self.__moduleDict.values():
			self.__moduleDict[key] = module
			return True
		else:
			return False

	def getAllModules(self):
		return self.__moduleDict

	def getModuleByIndex(self, key):
		if key in self.__moduleDict.keys():
			return self.__moduleDict[key]
		else:
			return None

	def removeModule(self, module):
		for key, item in self.__moduleDict.items():
			if module == item:
				self.__moduleDict.pop(key)
				return  True
		return False

	def removeModuleByKey(self, removeKey):
		for key in self.__moduleDict.keys():
			if removeKey == key:
				self.__moduleDict.pop(key)
				return True
		return False

	def removeAllModule(self):
		for key in self.__moduleDict.keys():
			self.__moduleDict.pop(key)
		