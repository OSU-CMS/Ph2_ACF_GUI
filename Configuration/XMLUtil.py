import lxml.etree as ET
import os
from xml.dom import minidom
from Configuration.Settings.GlobalSettings import *
from Configuration.Settings.FESettings import *
from Configuration.Settings.HWSettings import *
from Configuration.Settings.MonitoringSettings import *
from Configuration.Settings.RegisterSettings import *
from Gui.GUIutils.settings import *

import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

Ph2_ACF_VERSION = os.environ.get("Ph2_ACF_VERSION")

def prettify(elem):
    """Return a pretty-printed XML string for the Element.
    """
    rough_string = ET.tostring(elem)
    #rough_string = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")

def LoadXML(filename="CMSIT.xml"):
  tree = ET.parse(filename)
  root = tree.getroot()
  return root,tree

def ShowXMLTree(XMLroot, depth=0):
  depth += 1
  print("--"*(depth-1), "|", XMLroot.tag, XMLroot.attrib, XMLroot.text)
  for child in XMLroot:
    ShowXMLTree(child,depth)

def ModifyBeboard(XMLroot, BeboardModule):
  def __init__(self):
    print("Nothing Done")

class HWDescription():
  def __init__(self):
    self.BeBoardList = []
    self.Settings = {}
    self.MonitoringList =  []
    print("Setting HWDescription")

  def AddBeBoard(self, BeBoardModule):
    self.BeBoardList.append(BeBoardModule)

  def AddSettings(self, Settings):
    self.Settings = Settings

  def AddMonitoring(self, MonitoringModule):
    self.MonitoringList.append(MonitoringModule)


  def reset(self):
    self.BeBoardList = []
    self.Settings = {}

class Module():
    def __init__(self,serialNo,moduleId,chipInfo,status="1"):
        self.serialNo = serialNo
        self.moduleId = moduleId #input for “FMC port:” in GUI
        self.status = status # control by a click mark in gui start window.
        self.Files = "./" # following the xml file we have now
        self.moduleType = None #I need to add a method get module type automatically. #module type is RD53a?? or tfpx
        self.chipType = None
        self.test = None
        self.chipInfo = chipInfo
        self.chipList = []
        self.setModuleType()
        self.SetChipType()
        self.adding_chips()
        


    def adding_chips(self):
        IdLaneDict=ModuleLaneMap[self.moduleType]
        numberOfChips = len(IdLaneDict)
        print("self.chipInfo:" + str(self.chipInfo))
        for chip in self.chipInfo:    
            if not chip.getStatus():
                continue
            VDDA=chip.getVDDA()
            VDDD=chip.getVDDD()
            ChipId = chip.getID()
            Lane = chip.getLane()
            ConfigfileName = f"CMSIT_RD53_{self.serialNo}_{self.moduleId}_{ChipId}.txt"   #follow the guide in GenerateXMLConfig() in guituil.py
            self.chipList.append([ChipId,Lane,ConfigfileName,VDDA,VDDD])
      

    def setModuleType(self):
        if 'ZH' in self.serialNo:
            self.moduleType = "TFPX Quad"
        if 'SCC' in self.serialNo:
            self.moduleType = "SingleSCC"
        if 'RH' in self.serialNo:
            self.moduleType = "CROC 1x2"
        # there should be more module type
    
    def getModuleType(self):
        return self.moduleType

    def SetChipType(self):
        if 'CROC' in self.moduleType:
            self.chipType = "RD53B"
        else:
            self.chipType = "RD53A"

    def getChipType(self):
        return self.chipType
        
    
    def get_Chip_info(self):
        return self.chipList
    
    def getting_globalSetting(self):
        if self.moduleType == "RD53A":
            GO_Dict = globalSettings_DictA[self.test]
        else:
            GO_Dict = globalSettings_DictB[self.test]
        return GO_Dict
            
    def setTest(self,testName):
        self.test = testName
        
class OGModule():
    def __init__(self,OGID,FMCID):
        self.Id=OGID
        self.FMCId=FMCID
        self.isOpticalLink = False
        #self.HyBridList = []
        self.moduleList = []
        self.test = None

    def SetOpticalGrp(self, Id, FMCId, isOptLink=False):
        self.Id=Id
        self.FMCId=FMCId
        self.isOpticalLink = isOptLink

    def adding_module(self,NewModule,chipInfo):
        NewModule.test =self.test
        NewModule.chipInfo = chipInfo
        self.moduleList.append(NewModule)

    #def AddHyBrid(self, HybridModule):
    #    self.HyBridList.append(HybridModule)

class board():
    def __init__(self,boardID,testName):
        #self.moduleList = []
        self.OGList = {}
        #self.OpticalGroupDict = {'Id':"0",'FMCId':"0"}
        self.boardID = boardID
        self.boardType = "RD53"
        self.eventType = "VR"
        self.test = testName
        self.ip_address = '0.0.0.0'
        self.id = "cmsinnertracker.crate0.slot0" 
        self.uri = "chtcp-2.0://localhost:10203?target={0}:50001".format(self.ip_address)
        self.address_table = "file://${PH2ACF_BASE_DIR}/settings/address_tables/CMSIT_address_table.xml"

    def addOG(self,OGID="0",FMCID="0"):
        OG0=OGModule(OGID,FMCID)
        OG0.test = self.test
        #dict = {OGID:OG0}
        self.OGList[OGID] = OG0

    def GetTest(self): 
        return self.test

    def add_connection(self,address_table,connectionID,uri):
        self.address_table = address_table
        self.connectionID = connectionID
        self.uri = uri

class XMLGenerator:
    
    def loadingXML(self,board1,board2=None,isOpticalLink=False): 
        test = board1.GetTest() #adding this method to board class(TBD)
        xml = self.buildingRoot("HwDescription")
        boardList = []
        boardList.append(board1)

        StatusStr = 'Status'
        if "v4-11" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-13" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-14" in Ph2_ACF_VERSION:
            StatusStr = 'enable'

        if board2 != None:
            boardList.append(board2)
        for i in range(len(boardList)):
            #adding beboard subelement
            beboardElement = self.add_node(xml,"BeBoard",{"Id" : boardList[i].boardID, "boardType" : boardList[i].boardType, "eventType" :"VR"})
            #adding connection subelement
            connectionElement = self.add_node(beboardElement,"connection",{"address_table" : boardList[i].address_table, "id" : "cmsinnertracker.crate0.slot0" , "uri" : boardList[i].uri})
            
            
            specificBoard = boardList[i]
            for OG in specificBoard.OGList.values():
                #OGList is a dictionary where keys are OGIG and value is the corresponding OGmodule Class
                connectionElement = self.add_node(beboardElement, "OpticalGroup",{"FMCId":OG.FMCId ,"Id":OG.Id})
                #loop over modules that are conneting to a same fc board
                for module in OG.moduleList:
                    hybridElement = self.add_node(connectionElement,"Hybrid",{"Id" : module.moduleId, "Name" : module.serialNo, StatusStr:module.status})
                    if isOpticalLink:
                        Node_OpFiles = self.add_node(connectionElement, 'lqGBT_Files',{'path':"${PWD}/"})
                        Node_lqGBT = self.add_node(connectionElement, 'lqGBT',{'Id':'0','version':'1','configfile':'CMSIT_LqGBT-v1.txt','ChipAddress':'0x70','RxDataRate':'1280','RxHSLPolarity':'0','TxDataRate':'160','TxHSLPolarity':'1'})
                        Node_lqGBTsettings = self.add_node(Node_lqGBT, 'Settings')
                    self.add_node(hybridElement, "RD53_Files" ,{"file" : module.Files})
                    #loop over chips
                    type = module.moduleType
                    chiptype = module.chipType
                    for chip in module.chipList:
                        ChipElement = self.add_node(hybridElement,chiptype, {"Id" : chip[0], "Lane" : chip[1] , "configfile" : chip[2]})
                        #adding FESetting/ one setting per chip
                        VDDA = chip[3]
                        VDDD = chip[4]
                        print("adding FEsetttings debug")
                        self.addFESetting(ChipElement,type,test,VDDA,VDDD)
                
                    #adding global setting/  one setting per module
                    self.addGOSettings(hybridElement,chiptype,test) #Q: ask matt does muduole type is RD53 or CROC

            #adding Register setting/ one setting per board
            self.addRegisterSetting(beboardElement)

        #adding HWSetting
        #specificBoard.OGList.values()
        self.addHWSetting(xml,chiptype,test)

        #adding MonitorSetting
        self.addMonitorSetting(xml,chiptype,"1","1000")

        return xml


    def buildingRoot(self, root_node): #used in usingXMLGen.py
        spcialRoot = etree.Element(root_node)
        return spcialRoot

    def add_element(self, parent, element):
        parent.append(element)


    def add_node(self, parent, tag, attrib=None, nsmap=None, **extra):
        element = etree.SubElement(parent, tag, attrib=attrib, nsmap=nsmap, **extra)
        return element

    #adding Setting parameters
    #monitor setting
    def addMonitorSetting(self,parent,boardtype,enable,sleeptime):
        # Create the Monitoring element with attributes
        monitoring_title = etree.SubElement(parent, "MonitoringSettings")
        monitoring_elem = etree.SubElement(monitoring_title, "Monitoring", enable="1", type="RD53A")

        # Create MonitoringSleepTime element
        monitoring_sleep_time_elem = etree.SubElement(monitoring_elem, "MonitoringSleepTime")
        monitoring_sleep_time_elem.text = sleeptime
        self.innerMonitorSetting(monitoring_elem,boardtype)

    def innerMonitorSetting(self,monitoring_elem,boardtype):
        if boardtype == "RD53A":
            MonDict = MonitoringListB
        else:
            MonDict = MonitoringListB
        # Create MonitoringElement elements based on the MonitoringListA dictionary
        for register, enable in MonDict.items():
            etree.SubElement(monitoring_elem, "MonitoringElement", device="RD53", enable=enable, register=register)
    
    #HWSetting
    def addInnerHWSetting(self,parent,chiptype,test):
        
        if chiptype == "RD53A":
            HWDict = HWSettings_DictA[test]
        else:
            HWDict = HWSettings_DictB[test]

        for name, value in HWDict.items():
            setting_elem = etree.SubElement(parent, "Setting", name=name)
            setting_elem.text = str(value)
    
    def addHWSetting(self,parent,boardtype,test):
        self.setting_title = etree.SubElement(parent, "Settings")
        self.addInnerHWSetting(self.setting_title,boardtype,test)

        

    #Register Setting
    def create_subelements_Register(self,parent, path, value):
        elements = path.split('.')
        for elem in elements:
            parent = etree.SubElement(parent, "Register", name=elem)
        parent.text = str(value)

    def addRegisterSetting(self,parent):
        for path, value in RegisterSettings.items():
            self.create_subelements_Register(parent, path, value)
        
    def addFESetting(self,parent,boardType,test,VDDAtrim=None,VDDDtrim=None):
        if boardType == "RD53A":
            FEDict = FESettings_DictA[test]
        else:
            FEDict = FESettings_DictB[test]
        
        if VDDAtrim == None:
            pass
        else:
            FEDict['VOLTAGE_TRIM_ANA'] = VDDAtrim
        
        if VDDDtrim == None:
            pass
        else:
            FEDict['VOLTAGE_TRIM_DIG'] = VDDDtrim


        self.add_node(parent,"Settings",FEDict)

    def addGOSettings(self,parent,chipType,test):
        if chipType == "RD53A":
            GODict = globalSettings_DictA[test]
        else:
            GODict = globalSettings_DictB[test]
            
        #calling add_node in the xml class
        self.add_node(parent,"Global",GODict)


class Device():
  def __init__(self):
    self.PowerSupply = None

  def setPowerSupply(self, powersupply):
    self.PowerSupply = powersupply
  
class PowerSupplyNode():
  def __init__(self):
    self.__ID          =   "MyPowerSupply"
    self.__InUse       =   "Yes"
    self.__Model       =   "Keithley" 
    self.__Connection  =   "Serial"
    self.__Port        =   "/dev/ttyUSB0"
    self.__BaudRate    =   "9600"
    self.__FlowControl =   "false"
    self.__Parity      =   "false"
    self.__RemoveEcho  =   "false"
    self.__Terminator  =   "CRLF"
    self.__Suffix      =   "CRLF"
    self.__Timeout     =   "1"
    self.__Channels    = []

  @property
  def ID(self):
    return self.__ID
  @ID.setter
  def ID(self, id):
    if not isinstance(id, str):
      logger.error('ID must be a string!')
    self.__ID = id

  @property
  def InUse(self):
    return self.__InUse
  @InUse.setter
  def InUse(self, inuse):
    if not isinstance(inuse, str):
      logger.error('InUse must be a string!')
    if inuse != "Yes":
      self.__ID = "No"
    else:
      self.__ID = "Yes"
  
  @property
  def Model(self):
    return self.__Model 
  @Model.setter
  def Model(self, model):
    if not isinstance(model, str):
      logger.error('Model must be a string')
    self.__Model = model

  @property
  def Connection(self):
    return self.__Connection
  @Connection.setter
  def Connection(self, connection):
    if not isinstance(connection, str):
      logger.error('Connection must be a string')
    self.__Connection = connection

  @property
  def Port(self):
    return self.__Port
  @Port.setter
  def Port(self, port):
    if not isinstance(port, str):
      logger.error('Port must be a string')
    self.__Port = port

  @property
  def BaudRate(self):
    return self.__BaudRate
  @BaudRate.setter
  def BaudRate(self, baudrate):
    if not isinstance(baudrate, str):
      logger.error('BaudRate must be a string')
    self.__BaudRate = baudrate

  @property
  def FlowControl(self):
    return self.__FlowControl
  @FlowControl.setter
  def FlowControl(self, flowcontrol):
    if not isinstance(flowcontrol, str):
      logger.error('FlowControl must be a string')
    self.__FlowControl = flowcontrol

  @property
  def Parity(self):
    return self.__Parity
  @Parity.setter
  def Parity(self, parity):
    if not isinstance(parity, str):
      logger.error('Parity must be a string')
    self.__Parity = parity

  @property
  def RemoveEcho(self):
    return self.__RemoveEcho
  @RemoveEcho.setter
  def RemoveEcho(self, removeecho):
    if not isinstance(removeecho, str):
      logger.error('RemoveEcho must be a string')
    self.__RemoveEcho = removeecho

  @property
  def Terminator(self):
    return self.__Terminator
  @Terminator.setter
  def Terminator(self, terminator):
    if not isinstance(terminator, str):
      logger.error('Terminator must be a string')
    self.__Terminator = terminator

  @property
  def Suffix(self):
    return self.__Suffix
  @Suffix.setter
  def Suffix(self, suffix):
    if not isinstance(suffix, str):
      logger.error('Suffix must be a string')
    self.__Suffix = suffix

  @property
  def Timeout(self):
    return self.__Timeout
  @Timeout.setter
  def Timeout(self, timeout):
    if not isinstance(timeout, str):
      logger.error('Timeout must be a string')
    self.__Timeout = timeout

  def addChannel(self, channel):
    if not isinstance(channel, Channel):
      logger.error("Object added is not instance of Class Channel")
    self.__Channels.append(channel)
  
  def getChannels(self):
    return self.__Channels

class Channel():
  def __init__(self):
    self.ID= "Front"
    self.Channel="FRON"
    self.InUse ="Yes"
  
  def setID(self,id):
    if not isinstance(id, str):
      logger.error("ID should be a string")
    else:
      self.ID = id
  
  def getID(self):
    return self.ID

  def setChannel(self,channel):
    if not isinstance(channel, str):
      logger.error("Channel should be a string")
    else:
      self.Channel = channel
  
  def getChannel(self):
    return self.Channel

  def setInUse(self,inuse):
    if not isinstance(inuse, str):
      logger.error("InUse should be a string")
    elif inuse == "Yes":
      self.InUse = "Yes"
    else:
      self.InUse = "No"
  
  def getInUse(self):
    return self.InUse

def GeneratePowerSupplyXML(device, outputFile = "PowerSupplyConfig.xml"):
  if not isinstance(device, Device):
    logger.error("Input object is not an instance of class PowerSupply")

  Node_Device = ET.Element('Devices')
  Node_PowerSupply = ET.SubElement(Node_Device, 'PowerSupply')
  PowerSupply = device.PowerSupply
  AttributionList = ["ID","InUse","Model","Connection","Port","BaudRate","FlowControl","Parity","RemoveEcho","Terminator","Suffix","Timeout"]
  AttributionDict = {}
  for attribute in AttributionList:
    AttributionDict[attribute] = getattr(PowerSupply, attribute)
  Node_PowerSupply = SetNodeAttribute(Node_PowerSupply,AttributionDict)
  ChannelList = PowerSupply.getChannels()
  for channel in ChannelList:
    Node_Channel = ET.SubElement(Node_PowerSupply, 'Channel')
    Node_Channel = SetNodeAttribute(Node_Channel,{"ID":channel.getID(),"Channel":channel.getChannel(),"InUse":channel.getInUse()})
  
  xmlstr = prettify(Node_Device)
  with open(outputFile, "w") as f:
    f.write(str(xmlstr))
    f.close() 

##################################################
##  For OLD XML format
##################################################
'''
def GenerateHWDescriptionXML(HWDescription,outputFile = "CMSIT_gen.xml"):
  Node_HWInterface = ET.Element('HWInterface')
  BeBoardList = HWDescription.BeBoardList
  for BeBoard in BeBoardList:
    Node_BeBoard = ET.SubElement(Node_HWInterface, 'BeBoard')
    #Node_BeBoard.Set('Id',BeBoard.Id)
    #Node_BeBoard.Set('boardType',BeBoard.boardType)
    #Node_BeBoard.Set('eventType',BeBoard.eventType)
    Node_BeBoard = SetNodeAttribute(Node_BeBoard,{'Id':BeBoard.Id,'boardType':BeBoard.boardType,'eventType':BeBoard.eventType})

    Node_connection = ET.SubElement(Node_BeBoard, 'connection')
    #Node_connection.Set('id',BeBoard.id)
    #Node_connection.Set('uri',BeBoard.uri)
    #Node_connection.Set('address_table',BeBoard.address_table)
    Node_connection = SetNodeAttribute(Node_connection,{'id':BeBoard.id,'uri':BeBoard.uri,'address_table':BeBoard.address_table})
    FEModuleList = BeBoard.FEModuleList

    # Config front-end Modules
    for FEModule in FEModuleList:
      Node_FEModule = ET.SubElement(Node_BeBoard, 'Module')
      Node_FEModule = SetNodeAttribute(Node_FEModule,{'FeId':FEModule.FeId,'FMCId':FEModule.FMCId,'ModuleId':FEModule.ModuleId,'Status':FEModule.Status})
      Node_FEPath = ET.SubElement(Node_FEModule, BeBoard.boardType+'_Files')
      Node_FEPath = SetNodeAttribute(Node_FEPath,{'file':FEModule.File_Path})

      FEList = FEModule.FEList
      for FE in FEList:
        Node_FE = ET.SubElement(Node_FEModule, BeBoard.boardType)
        Node_FE = SetNodeAttribute(Node_FE,{'Id':FE.Id,'Lane':FE.Lane,'configfile':FE.configfile})
        Node_FESetting = ET.SubElement(Node_FE,"Settings")
        Node_FESetting = SetNodeAttribute(Node_FESetting,FE.settingList)
      Node_FEGlobal = ET.SubElement(Node_FEModule,"Global")
      Node_FEGlobal = SetNodeAttribute(Node_FEGlobal,FEModule.globalSetting)

    #Config back-end registers
    Node_BeBoard = SetNodeRegister(Node_BeBoard,BeBoard.RegisterSettings)

  Node_Settings = ET.SubElement(Node_HWInterface, 'Settings')
  Node_Settings = SetNodeValue(Node_Settings, HWDescription.Settings)

  xmlstr = prettify(Node_HWInterface)
  with open(outputFile, "w") as f:
    f.write(str(xmlstr))
    #f.write(ET.tostring(Node_HWInterface, 'utf-8'))
    f.close()  
'''
        

if __name__ == "__main__":
  #root,tree = LoadXML()
  #ShowXMLTree(root)

  
  # Config Front-end Chip
  FE0_0_0 = FE()
  FE0_0_0.SetFE(0,0,"RD53.txt")
  FE0_0_0.ConfigureFE(FESettings)

  FE0_0_1 = FE()
  FE0_0_1.SetFE(1,1,"RD53.txt")
  FE0_0_1.ConfigureFE(FESettings)
  # Config Front-end Module
  #FEModule0_0 = FEModule()
  #FEModule0_0.AddFE(FE0_0_0)
  #FEModule0_0.ConfigureGlobal(globalSettings)

  # Config HyBrid
  HyBrid0_0 = HyBridModule()
  HyBrid0_0.AddFE(FE0_0_0)
  HyBrid0_0.AddFE(FE0_0_1)
  HyBrid0_0.ConfigureGlobal(globalSettings)
  # Config Optical Group
  OpticalGroup0_0 = OGModule()
  OpticalGroup0_0.AddHyBrid(HyBrid0_0)

  # Config BeBoardModule
  BeBoardModule0 = BeBoardModule()
  #BeBoardModule0.AddFEModule(FEModule0_0)
  BeBoardModule0.AddOGModule(OpticalGroup0_0)
  BeBoardModule0.SetRegisterValue(RegisterSettings)

  # Config HWDescription
  HWDescription0 = HWDescription()
  HWDescription0.AddBeBoard(BeBoardModule0)
  HWDescription0.AddSettings(HWSettings)

  GenerateHWDescriptionXML(HWDescription0,"CMSIT_gen_OG.xml")

  HWDescription0 = HWDescription()
  #HWDescription0.reset()
  HWDescription0.AddBeBoard(BeBoardModule0)
  HWDescription0.AddSettings(HWSettings)
  GenerateHWDescriptionXML(HWDescription0,"CMSIT_gen_OG_v2.xml")


  ChannelFront = Channel()
  PowerSupply0 = PowerSupplyNode()
  PowerSupply0.addChannel(ChannelFront)
  Device0 = Device()
  Device0.setPowerSupply(PowerSupply0)

  GeneratePowerSupplyXML(Device0,"HVPowerSupply.xml")

  
