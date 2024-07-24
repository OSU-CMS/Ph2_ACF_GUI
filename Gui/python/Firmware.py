from Gui.GUIutils.settings import (
    ModuleLaneMap,
)
from Gui.python.logging_config import logger


class QtChip:
    def __init__(self, chipID = "", chipLane = "", chipVDDA = 0, chipVDDD = 0, chipStatus = True):
        self.__chipID = chipID
        self.__chipLane = chipLane
        self.__chipVDDA = chipVDDA
        self.__chipVDDD = chipVDDD
        self.__chipStatus = chipStatus

    def setID(self, id: str):
        self.__chipID = id

    def getID(self):
        return self.__chipID

    def setLane(self, lane: str):
        self.__chipLane = lane
    
    def getLane(self):
        return self.__chipLane

    def setVDDA(self, pVDDAtrim: int):
        self.__chipVDDA = pVDDAtrim
    
    def getVDDA(self):
        return self.__chipVDDA

    def setVDDD(self, pVDDDtrim: int):
        self.__chipVDDD = pVDDDtrim
    
    def getVDDD(self):
        return self.__chipVDDD

    def setStatus(self, pStatus: bool):
        self.__chipStatus = pStatus

    def getStatus(self):
        return self.__chipStatus
    
    def __str__(self):
        return f"    ChipID: {self.__chipID}, LaneID: {self.__chipLane}, ChipVDDD: {self.__chipVDDD}, ChipVDDA: {self.__chipVDDA}, ChipStatus: {self.__chipStatus}"


class QtModule:
    def __init__(self, moduleName = "", moduleType = "", moduleVersion = "", FMCPort = ""):
        self.__moduleName = moduleName
        self.__moduleType = moduleType
        self.__moduleVersion = moduleVersion
        self.__FMCPort = FMCPort
        self.__chipDict = {} #{ChipID : QtChip()}, deviates from pattern to make usage easier, laneID is not commonly used
        
        if self.__moduleType != "":
            #if module type was specified, initialize chips to default
            self.__setupChips()
    
    def setModuleName(self, moduleName: str):
        self.__moduleName = moduleName
    
    def getModuleName(self):
        return self.__moduleName
    
    def setModuleType(self, moduleType: str):
        if moduleType not in ModuleLaneMap.keys():
            print(f"Module type '{moduleType}' is not familiar. Defaulting to 'CROC SCC'.")
            self.__moduleType = "CROC SCC"
        else:
            self.__moduleType = moduleType
        self.__setupChips()
    
    def getModuleType(self):
        return self.__moduleType
    
    def setModuleVersion(self, moduleVersion: str):
        self.__moduleVersion = moduleVersion
    
    def getModuleVersion(self):
        return self.__moduleVersion
    
    def setFMCPort(self, FMCPort: str):
        self.__FMCPort = FMCPort
    
    def getFMCPort(self):
        return self.__FMCPort
    
    def __setupChips(self):
        self.__chipDict.clear()
        
        for LaneID, ChipID in ModuleLaneMap[self.__moduleType].items():
            FEChip = QtChip(
                chipID = ChipID,
                chipLane = LaneID,
                chipVDDA = 8,
                chipVDDD = 8,
                chipStatus = True
            )
            self.__chipDict[ChipID] = FEChip

    def getChips(self):
        return self.__chipDict
    
    def getEnabledChips(self):
        return {chipID : chip for chipID, chip in self.__chipDict.items() if chip.getStatus()}
    
    """
    These two functions define the parent object accessible from the current object. It's a shortcut
    to avoid rewriting some code that assumes modules know everything about themselves. 
    """
    def setOpticalGroup(self, opticalgroup):
        self.__parent = opticalgroup
    
    def getOpticalGroup(self):
        return self.__parent
    
    def __str__(self):
        chips_str = "\n".join([f"      {chipID}: {str(chip)}" for chipID, chip in self.__chipDict.items()])
        return f"  ModuleName: {self.__moduleName}, ModuleType: {self.__moduleType} {self.__moduleVersion}, FMCPort: {self.__FMCPort}\n    Chips:\n{chips_str}"


class QtOpticalGroup:
    def __init__(self, OpticalGroupID = "0", FMCID = ""):
        self.__OpticalGroupID = OpticalGroupID
        self.__FMCID = FMCID
        self.__moduleDict = {} #{FMCPort : QtModule()}
    
    def setOpticalGroupID(self, OpticalGroupID: str):
        self.__OpticalGroupID = OpticalGroupID
    
    def getOpticalGroupID(self):
        return self.__OpticalGroupID
    
    def setFMCID(self, FMCID: str):
        self.__FMCID = FMCID
    
    def getFMCID(self):
        return self.__FMCID
    
    def addModule(self, FMCPort: str, module: QtModule):
        self.__moduleDict[FMCPort] = module
    
    def removeModule(self, module: QtModule):
        for key, value in self.__moduleDict:
            if value == module:
                del self.__moduleDict[key]
    
    def removeModuleByIndex(self, FMCPort: str):
        del self.__moduleDict[FMCPort]
    
    def removeAllModules(self):
        self.__moduleDict.clear()
    
    def getModuleByIndex(self, FMCPort: str):
        return self.__moduleDict[FMCPort]
    
    def getAllModules(self):
        return self.__moduleDict
    
    """
    These two functions define the parent object accessible from the current object. It's a shortcut
    to avoid rewriting some code that assumes modules know everything about themselves. 
    """
    def setBeBoard(self, beboard):
        self.__parent = beboard
    
    def getBeBoard(self):
        return self.__parent
    
    def __str__(self):
        modules_str = "\n".join([f"    {FMCPort}: {str(module)}" for FMCPort, module in self.__moduleDict.items()])
        return f"  FMCID: {self.__FMCID}, OGID: {self.__OpticalGroupID}\n  Modules:\n{modules_str}"


class QtBeBoard:
    def __init__(self, BeBoardID = "0", boardName = "", ipAddress = "0.0.0.0"):
        self.__BeBoardID = BeBoardID
        self.__boardName = boardName
        self.__ipAddress = ipAddress
        self.__OGDict = {} #{FMCID : QtOpticalGroup()}
        #self.__fpgaConfigName = ""

    def setBoardID(self, boardID: str):
        self.__BeBoardID = boardID

    def getBoardID(self):
        return self.__BeBoardID
    
    def setBoardName(self, boardName: str):
        self.__boardName = boardName

    def getBoardName(self):
        return self.__boardName

    def setIPAddress(self, ipAddress: str):
        self.__ipAddress = ipAddress

    def getIPAddress(self):
        return self.__ipAddress

    def addOpticalGroup(self, FMCID: str, OpticalGroup: QtOpticalGroup):
        OpticalGroup.setOpticalGroupID(str(len(self.__OGDict)))
        self.__OGDict[FMCID] = OpticalGroup

    def removeOpticalGroup(self, OG: QtOpticalGroup):
        for key, value in self.__OGDict.items():
            if value == OG:
                del self.__OGDict[key]

    def removeOpticalGroupByIndex(self, FMCID: str):
        del self.__OGDict[FMCID]

    def removeAllOpticalGroups(self):
        self.__OGDict.clear()

    def getOpticalGroupByIndex(self, FMCID: str):
        return self.__OGDict[FMCID]

    def getAllOpticalGroups(self):
        return self.__OGDict
    
    def __str__(self):
        optical_groups_str = "\n".join([f"  {FMCID}: {str(opticalGroup)}" for FMCID, opticalGroup in self.__OGDict.items()])
        return f"BoardName: {self.__boardName}, BoardID: {self.__BeBoardID}, IPAddress: {self.__ipAddress}\nOpticalGroups:\n{optical_groups_str}"
    
    ############ Helper Functions for Convenience ############
    
    def getModules(self):
        modules = []
        for opticalgroup in self.getAllOpticalGroups().values():
            for module in opticalgroup.getAllModules().values():
                modules.append(module)
        return modules
    
    def removeModules(self):
        for og in self.__OGDict.values():
            og.removeAllModules()
    
    def getModuleData(self):
        #Implemented under the assumption that every module will be of the same type/version
        for OG in self.getAllOpticalGroups().values():
            for module in OG.getAllModules().values():
                return {'type':module.getModuleType(), 'version':module.getModuleVersion()}
    
    ##########################################################