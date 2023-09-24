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

  
