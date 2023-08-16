import Gui.Config.staticSettings as settings


class Chip:
    def __init__(self):
        self.__chipID = ""
        self.__chipLane = ""
        self.__chipVDDA = ""
        self.__chipVDDD = ""
        self.__chipStatus = True

    def setID(self, id):
        self.__chipID = str(id)

    def getID(self):
        return self.__chipID

    def setLane(self, lane):
        self.__chipLane = str(lane)

    def setVDDA(self, pVDDAtrim):
        self.__chipVDDA = str(pVDDAtrim)

    def setVDDD(self, pVDDDtrim):
        self.__chipVDDD = str(pVDDDtrim)

    def setStatus(self, pStatus):
        self.__chipStatus = pStatus

    def getVDDA(self):
        return self.__chipVDDA

    def getVDDD(self):
        return self.__chipVDDD

    def getStatus(self):
        return self.__chipStatus

    def getLane(self):
        return self.__chipLane


# Dedicated for OpticalGroup and
class Module:
    def __init__(self, **kwargs):
        self.__moduleName = "SerialNumber1"
        self.__moduleID = "0"
        self.__moduleType = "SingleSCC"
        self.__FMCID = "0"
        self.__OGID = "0"
        self.__chipDict = {}
        self.__VDDAMap = {}  # Format is {chipID : VDDA value}
        self.__VDDDMap = {}  # Format is {chipID : VDDD value}
        self.__ChipStatusMap = {}  # Format is {chipID : enable flag}

        for key, value in kwargs.items():
            if key == "id":
                self.__moduleID = str(value)
            if key == "type":
                self.__moduleType = str(value)

        # FIXME: This need to pass along a dictionary of chipID : [VDDA,VDDD]
        # self.setupChips()  #commented because I think it is redundant, but not 100% sure.

    def setModuleName(self, name):
        self.__moduleName = name

    def getModuleName(self):
        return self.__moduleName

    def setModuleID(self, id):
        self.__moduleID = id

    def getModuleID(self):
        return self.__moduleID

    def setFMCID(self, fmcId):
        self.__FMCID = fmcId

    def getFMCID(self):
        return self.__FMCID

    def setOpticalGroupID(self, ogId):
        self.__OGID = ogId

    def getOpticalGroupID(self):
        return self.__OGID

    def setModuleType(self, fwType):
        if fwType in ModuleType.values():
            self.__moduleType = fwType
        else:
            self.__moduleType = "SingleSCC"
        self.setupChips()

    def getModuleType(self):
        return self.__moduleType

    def setChipVDDD(self, pChipID, pVDDDtrim):
        self.__VDDDMap[pChipID] = pVDDDtrim
        # self.setupChips()

    def setChipVDDA(self, pChipID, pVDDAtrim):
        self.__VDDAMap[pChipID] = pVDDAtrim

    def setChipStatus(self, pChipID, pStatus):
        self.__ChipStatusMap[pChipID] = pStatus

    # FIXME: This function needs to accept a dictionary of chipID : [VDDA, VDDD].
    def setupChips(self, **kwargs):
        self.__chipDict = {}
        if "chips" in kwargs.keys():
            pass
            return
        # for key,value in kwargs.items():
        # 	if key=='VDDA':

        for i in ModuleLaneMap[self.__moduleType].keys():
            FEChip = Chip()
            # FEChip.setID(8)
            LaneID = str(i)
            chipNumber = ModuleLaneMap[self.__moduleType][LaneID]
            FEChip.setID(ModuleLaneMap[self.__moduleType][LaneID])
            FEChip.setLane(LaneID)
            FEChip.setVDDA(self.__VDDAMap[chipNumber])
            FEChip.setVDDD(self.__VDDDMap[chipNumber])
            FEChip.setStatus(self.__ChipStatusMap[chipNumber])

            self.__chipDict[i] = FEChip

    def getChips(self):
        return self.__chipDict


class OpticalGroup:
    def __init__(self):
        self.__FMCID = "0"
        self.__OGID = "0"
        self.__moduleDict = {}

    def setFMCID(self, fmcId):
        self.__FMCID = fmcId

    def getFMCID(self):
        return self.__FMCID

    def setOpticalGroupID(self, ogId):
        self.__OGID = ogId

    def getOpticalGroupID(self):
        return self.__OGID

    def setupModule(self, **kwargs):
        self.__moduleDict = {}
        if "module" in kwargs.keys():
            pass
            return
        for i in ModuleLaneMap[self.__moduleType].keys():
            FEChip = Chip()
            # FEChip.setID(8)
            FEChip.setID(i)
            FEChip.setLane(i)
            self.__chipDict[i] = FEChip

    def getChips(self):
        return self.__chipDict


class FC7:
    def __init__(self, boardName=""):
        self.__boardName = ""
        self.__ipAddress = "0.0.0.0"
        self.__moduleDict = {}
        self.__fpgaConfigName = ""

    def setName(self, name):
        self.__boardName = name

    def getBoardName(self):
        return self.__boardName

    def setBoardName(self, name):
        self.__boardName = name

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
                return True
        return False

    def removeModuleByKey(self, removeKey):
        for key in self.__moduleDict.keys():
            if removeKey == key:
                self.__moduleDict.pop(key)
                return True
        return False

    def removeAllModule(self):
        self.__moduleDict.clear()
