import lxml.etree as ET
from xml.dom import minidom
from Configuration.Settings.GlobalSettings import *
from Configuration.Settings.FESettings import *
from Configuration.Settings.HWSettings import *
from Configuration.Settings.RegisterSettings import *

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
    print("Setting HWDescription")

  def AddBeBoard(self, BeBoardModule):
    self.BeBoardList.append(BeBoardModule)

  def AddSettings(self, Settings):
    self.Settings = Settings

  def reset(self):
    self.BeBoardList = []
    self.Settings = {}


class BeBoardModule():
  def __init__(self):
    self.Id = str(0)
    self.boardType = "RD53"
    self.eventType = "VR"
    self.id = "cmsinnertracker.crate0.slot0" 
    self.uri = "chtcp-2.0://localhost:10203?target=192.168.1.80:50001" 
    self.address_table = "file://../settings/address_tables/CMSIT_address_table.xml"
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
    self.HyBridList = []
  def SetOpticalGrp(self, Id, FMCId):
    self.Id=Id
    self.FMCId=FMCId

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
    self.File_Path = "./"
    self.FEList = []
    self.globalSetting = {}
  def SetHyBridType(self, Type):
    self.HyBridType = str(Type)
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
  def SetFE(self, Id, Lane, configfile = "CMSIT_RD53.txt"):
    self.Id = str(Id)
    self.Lane = str(Lane)
    self.configfile = configfile
  def ConfigureFE(self, settingList):
    self.settingList = settingList

def SetNodeAttribute(Node, Dict):
   for key in Dict:
     Node.set(key,Dict[key])
   return Node

def SetNodeValue(Node, Dict):
   for key in Dict:
     Node_setting = ET.SubElement(Node, 'Setting')
     Node_setting.set('name',key)
     Node_setting.text =  str(Dict[key]) 
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
    OpticalGroupList = BeBoard.OpticalGroupList

    # Config front-end Modules
    for OGModule in OpticalGroupList:
      Node_OGModule = ET.SubElement(Node_BeBoard, 'OpticalGroup')
      Node_OGModule = SetNodeAttribute(Node_OGModule,{'Id':OGModule.Id,'FMCId':OGModule.FMCId})
      HyBridList = OGModule.HyBridList

      for HyBridModule in HyBridList:
        Node_HyBrid = ET.SubElement(Node_OGModule, 'HyBrid')
        Node_FEPath = ET.SubElement(Node_HyBrid, HyBridModule.HyBridType+'_Files')
        Node_FEPath = SetNodeAttribute(Node_FEPath,{'file':HyBridModule.File_Path})
        FEList = HyBridModule.FEList
        for FE in FEList:
          Node_FE = ET.SubElement(Node_HyBrid, BeBoard.boardType)
          Node_FE = SetNodeAttribute(Node_FE,{'Id':FE.Id,'Lane':FE.Lane,'configfile':FE.configfile})
          Node_FESetting = ET.SubElement(Node_FE,"Settings")
          Node_FESetting = SetNodeAttribute(Node_FESetting,FE.settingList)
        Node_FEGlobal = ET.SubElement(Node_HyBrid,"Global")
        Node_FEGlobal = SetNodeAttribute(Node_FEGlobal,HyBridModule.globalSetting)

      #Config back-end registers
    Node_BeBoard = SetNodeRegister(Node_BeBoard,BeBoard.RegisterSettings)

  Node_Settings = ET.SubElement(Node_HWInterface, 'Settings')
  Node_Settings = SetNodeValue(Node_Settings, HWDescription.Settings)

  xmlstr = prettify(Node_HWInterface)
  with open(outputFile, "w") as f:
    f.write(str(xmlstr))
    #f.write(ET.tostring(Node_HWInterface, 'utf-8'))
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
  FE0_0_0.ConfigureFE(FESettings)

  # Config Front-end Module
  #FEModule0_0 = FEModule()
  #FEModule0_0.AddFE(FE0_0_0)
  #FEModule0_0.ConfigureGlobal(globalSettings)

  # Config HyBrid
  HyBrid0_0 = HyBridModule()
  HyBrid0_0.AddFE(FE0_0_0)
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

  
