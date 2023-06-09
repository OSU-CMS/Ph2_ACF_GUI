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


class BeBoardModule():
  def __init__(self):
    self.Id = str(0)
    self.boardType = "RD53"
    self.eventType = "VR"
    self.id = "cmsinnertracker.crate0.slot0" 
    self.uri = "chtcp-2.0://localhost:10203?target={0}:50001".format(ip_address)
    #self.uri = "chtcp-2.0://localhost:10203?target=192.168.1.80:50001" 
    self.address_table = "file://${PH2ACF_BASE_DIR}/settings/address_tables/CMSIT_address_table.xml"
    self.FEModuleList = []
    self.RegisterSettings = {}
    self.OpticalGroupList =  []

  def reset(self):
    self.FEModuleList = []
    self.RegisterSettings = {}
    self.OpticalGroupList =  []

  def SetBeBoard(self, Id, boardType, eventType = "VR"):
    self.Id = str(Id)
    self.boardType = boardType
    self.eventType = eventType

  def SetURI(self, ipAddress="0.0.0.0"):
    self.uri = "chtcp-2.0://localhost:10203?target={0}:50001".format(ipAddress)

  def AddFEModule(self, FEModule):
    self.FEModuleList.append(FEModule)

  def AddOGModule(self, OGModule):
    self.OpticalGroupList.append(OGModule)

  def SetConnection(self, beboardid, uri, address_table):
    self.id = beboardid
    self.uri = uri
    self.address_table = address_table

  def SetRegisterValue(self, RegisterSettings):
    self.RegisterSettings = RegisterSettings

class OGModule():
  def __init__(self):
    self.Id="0" 
    self.FMCId="0"
    self.isOpticalLink = False
    self.HyBridList = []
  def SetOpticalGrp(self, Id, FMCId, isOptLink=False):
    self.Id=Id
    self.FMCId=FMCId
    self.isOpticalLink = isOptLink

  def AddHyBrid(self, HybridModule):
    self.HyBridList.append(HybridModule)

class FEModule():
  def __init__(self):
    self.FeId="0" 
    self.FMCId="0" 
    self.ModuleId="0" 
    self.Status="1"
    self.File_Path = "./"
    self.FEList = []
    self.globalSetting = {}
  def SetFEFilePath(self,File_Path):
    self.File_Path = File_Path
  def SetFEModule(self, FeId, FMCId, ModuleId, Status):
    self.FeId = str(FeId)
    self.FMCId = str(FMCId) 
    self.ModuleId = str(ModuleId)
    self.Status = str(Status)
  def AddFE(self, FE):
    self.FEList.append(FE)
  def ConfigureGlobal(self, globalSetting):
    self.globalSetting = globalSetting

class HyBridModule():
  def __init__(self):
    self.HyBridType = "RD53"
    self.Id="0" 
    self.Status="1"
    self.Name = "Serial1"
    self.File_Path = "./"
    self.FEList = []
    self.globalSetting = {}
  def SetHyBridType(self, Type):
    self.HyBridType = str(Type)
  def SetHyBridName(self, name):
    self.Name = str(name)
  def SetFEFilePath(self,File_Path):
    self.File_Path = File_Path
  def SetHyBridModule(self, Id, Status):
    self.Id = str(Id)
    self.Status = str(Status)
  def AddFE(self, FE):
    self.FEList.append(FE)
  def ConfigureGlobal(self, globalSetting):
    self.globalSetting = globalSetting

class FE():
  def __init__(self):
    self.Id="0" 
    self.Lane="0" 
    self.configfile="CMSIT_RD53.txt"
    self.settingList = {}
    self.VDDAtrim = "8"
    self.VDDDtrim = "8"
    

  def SetFE(self, Id, Lane, configfile = "CMSIT_RD53.txt"):
    self.Id = str(Id)
    self.Lane = str(Lane)
    self.configfile = configfile
  def ConfigureFE(self, settingList):
    self.settingList = settingList
   

class MonitoringModule():
  def __init__(self,boardtype):
    self.Type=boardtype 
    if 'RD53A' in boardtype:
      self.Enable="1" 
    else:
      self.Enable="0"
    self.SleepTime=5000
    self.MonitoringList = {}
  def SetType(self, Type):
    self.Type=Type
  def SetEnable(self, enable):
    self.Enable=enable
  def SetSleepTime(self, sleepTime):
    self.SleepTime=sleepTime
  def SetMonitoringList(self, monitoringList):
    self.MonitoringList=monitoringList

def SetNodeAttribute(Node, Dict):
   for key in Dict:
     Node.set(key,Dict[key])
   return Node

def SetNodeValue(Node, Dict):
   for key in Dict:
     Node_setting = ET.SubElement(Node, 'Setting')
     Node_setting.set('name',key)
     Node_setting.text =  str(Dict[key])
     ET.tostring(Node_setting)
   return Node

def FindSubNode(Node,NodeString):
  for child in Node:
    if child.tag == 'name' and child.attrib == NodeString:
      return child
  child = ET.SubElement(Node,"Register")
  child = SetNodeAttribute(child,{'name':NodeString})
  return child

def GetRegNode(Node, NodeString):
  if '.' in NodeString:
    SubNode, FollowingNode = NodeString.split('.',1)
  else:
    SubNode = NodeString
    FollowingNode = None
  Node_Sub = FindSubNode(Node,SubNode)
  if FollowingNode != None:
    Node_Sub = GetRegNode(Node_Sub,FollowingNode)
  return Node_Sub
    

def SetNodeRegister(Node, Dict):
  for key in Dict:
    Node_Reg = GetRegNode(Node,key)
    Node_Reg.text = str(Dict[key])
  return Node

def SetMonitoring(Node, MonitoringItem):
  Node_Monitoring = ET.SubElement(Node, 'Monitoring')
  Node_Monitoring = SetNodeAttribute(Node_Monitoring,{'type':MonitoringItem.Type,'enable':MonitoringItem.Enable})
  Node_SleepTime  = ET.SubElement(Node_Monitoring, 'MonitoringSleepTime')
  Node_SleepTime.text = str(MonitoringItem.SleepTime)
  for element in MonitoringItem.MonitoringList.keys():
      Node_MonitoringElements = ET.SubElement(Node_Monitoring, 'MonitoringElement')
      Node_MonitoringElements = SetNodeAttribute(Node_MonitoringElements,{'device':"RD53",'register':element,'enable':MonitoringItem.MonitoringList[element]})
  return Node

def GenerateHWDescriptionXML(HWDescription,outputFile = "CMSIT_gen.xml", boardtype = "RD53A", isOpticalLink = False):
  Node_HWInterface = ET.Element('HwDescription')
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
    print('beboard ip is {0}'.format(BeBoard.uri))
    Node_connection = SetNodeAttribute(Node_connection,{'id':BeBoard.id,'uri':BeBoard.uri,'address_table':BeBoard.address_table})
    OpticalGroupList = BeBoard.OpticalGroupList

    # Config front-end Modules
    for OGModule in OpticalGroupList:
      Node_OGModule = ET.SubElement(Node_BeBoard, 'OpticalGroup')
      Node_OGModule = SetNodeAttribute(Node_OGModule,{'Id':OGModule.Id,'FMCId':OGModule.FMCId})
      if isOpticalLink:
        #FIXME: Add additional stuff for optical link here.
        Node_OpFiles = ET.SubElement(Node_OGModule, 'lqGBT_Files')
        Node_OpFiles = SetNodeAttribute(Node_OpFiles,{'path':"${PWD}/../settings/lpGBTFiles/"})
        Node_lqGBT = ET.SubElement(Node_OGModule, 'lqGBT')
        Node_lqGBT = SetNodeAttribute( Node_lqGBT, {'Id':'0','version':'1','configfile':'CMSIT_LqGBT-v1.txt','ChipAddress':'0x70','RxDataRate':'1280','RxHSLPolarity':'0','TxDataRate':'160','TxHSLPolarity':'1'})
        Node_lqGBTsettings = ET.SubElement(Node_lqGBT, 'Settings')
        
      HyBridList = OGModule.HyBridList
      

      for HyBridModule in HyBridList:
        Node_HyBrid = ET.SubElement(Node_OGModule, 'Hybrid')
        StatusStr = 'Status'
        if "v4-08" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-09" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-10" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-11" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-13" in Ph2_ACF_VERSION:
            StatusStr = 'enable'
        if "v4-14" in Ph2_ACF_VERSION:
            StatusStr = 'enable'

        Node_HyBrid = SetNodeAttribute(Node_HyBrid,{'Id':HyBridModule.Id,StatusStr:HyBridModule.Status,'Name':HyBridModule.Name})
        #### This is where the RD53_Files is setup ####
        ##FIXME Add in logic to change depending on version of Ph2_ACF -> Done!
        
        HyBridModule.SetHyBridType('RD53')  #This part should stay as just RD53 (no A or B)
        print("This is the Hybrid Type: ", HyBridModule.HyBridType)
        Node_FEPath = ET.SubElement(Node_HyBrid, HyBridModule.HyBridType+'_Files')
        Node_FEPath = SetNodeAttribute(Node_FEPath,{'file':HyBridModule.File_Path})
        FEList = HyBridModule.FEList
        ### This is where the RD53 block is being made ###
        for FE in FEList:
          BeBoard.boardType =  boardtype 
          print("This is the board type: ", BeBoard.boardType)
          Node_FE = ET.SubElement(Node_HyBrid, BeBoard.boardType)
          Node_FE = SetNodeAttribute(Node_FE,{'Id':FE.Id,'Lane':FE.Lane,'configfile':FE.configfile})
          if 'RD53B' in boardtype:
            Node_FELaneConfig = ET.SubElement(Node_FE,"LaneConfig")
            Node_FELaneConfig = SetNodeAttribute(Node_FELaneConfig,{'primary':"1",'outputLanes':"0001",'singleChannelInputs':"0000",'dualChannelInput':"0000"})
          Node_FESetting = ET.SubElement(Node_FE,"Settings")
          FE.settingList['VOLTAGE_TRIM_ANA'] = FE.VDDAtrim
          FE.settingList['VOLTAGE_TRIM_DIG'] = FE.VDDDtrim
          Node_FESetting = SetNodeAttribute(Node_FESetting,FE.settingList)
        Node_FEGlobal = ET.SubElement(Node_HyBrid,"Global")
        Node_FEGlobal = SetNodeAttribute(Node_FEGlobal,HyBridModule.globalSetting)

      #Config back-end registers
    Node_BeBoard = SetNodeRegister(Node_BeBoard,BeBoard.RegisterSettings)

  Node_Settings = ET.SubElement(Node_HWInterface, 'Settings')
  Node_Settings = SetNodeValue(Node_Settings, HWDescription.Settings)

  Node_MonitoringSettings = ET.SubElement(Node_HWInterface, 'MonitoringSettings')
  MonitoringList = HWDescription.MonitoringList
  for MonitoringItem in MonitoringList:
    Node_Monitoring = SetMonitoring(Node_MonitoringSettings, MonitoringItem)
    
  xmlstr = prettify(Node_HWInterface)
  with open(outputFile, "w") as f:
    f.write(str(xmlstr))
    #f.write(ET.tostring(Node_HWInterface, 'utf-8'))
    f.close()  

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

  
