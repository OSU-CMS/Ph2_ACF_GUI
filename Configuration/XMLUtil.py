import xml.etree.ElementTree as ET
from xml.dom import minidom
from GlobalSettings import *
from FESettings import *
from HWSettings import *

def prettify(elem):
    """Return a pretty-printed XML string for the Element.
    """
    rough_string = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")

def LoadXML(filename="CMSIT.xml"):
  tree = ET.parse(filename)
  root = tree.getroot()
  return root,tree

def ShowXMLTree(XMLroot, depth=0):
  depth += 1
  print "--"*(depth-1), "|", XMLroot.tag, XMLroot.attrib, XMLroot.text 
  for child in XMLroot:
    ShowXMLTree(child,depth)

def ModifyBeboard(XMLroot, BeboardModule):
  def __init__(self):
    print "Nothing Done"

class HWDescription():
  BeBoardList = []
  Settings = {}
  def __init__(self):
    print "Setting HWDescription"
  def AddBeBoard(self, BeBoardModule):
    self.BeBoardList.append(BeBoardModule)
  def AddSettings(self, Settings):
    self.Settings = Settings

class BeBoardModule():
  FEModuleList = []
  def __init__(self):
    self.Id = str(0)
    self.boardType = "RD53"
    self.eventType = "VR"
    self.id = "cmsinnertracker.crate0.slot0" 
    self.uri = "chtcp-2.0://localhost:10203?target=192.168.1.80:50001" 
    self.address_table = "file://../settings/address_tables/CMSIT_address_table.xml"
  def SetBeBoard(self, Id, boardType, eventType = "VR"):
    self.Id = str(Id)
    self.boardType = boardType
    self.eventType = eventType
  def AddFEModule(self, FEModule):
    self.FEModuleList.append(FEModule)
  def SetConnection(self, beboardid, uri, addess_table):
    self.id = beboardid
    self.uri = uri
    self.address_table = address_table

class FEModule():
  FEList = []
  globalSetting = {}
  def __init__(self):
    self.FeId="0" 
    self.FMCId="0" 
    self.ModuleId="0" 
    self.Status="1"
    self.File_Path = "./"
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

class FE():
  settingList = {}
  def __init__(self):
    self.Id="0" 
    self.Lane="0" 
    self.configfile="CMSIT_RD53.txt"
  def SetFE(self, Id, Lane, configfile):
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

  Node_Settings = ET.SubElement(Node_HWInterface, 'Settings')
  Node_Settings = SetNodeValue(Node_Settings, HWDescription.Settings)

  xmlstr = prettify(Node_HWInterface)
  with open(outputFile, "w") as f:
    f.write(str(xmlstr.decode('UTF-8')))
    #f.write(ET.tostring(Node_HWInterface, 'utf-8'))
    f.close()  
        

if __name__ == "__main__":
  #root,tree = LoadXML()
  #ShowXMLTree(root)

  # Config Front-end Chip
  FE0_0_0 = FE()
  FE0_0_0.ConfigureFE(FESettings)

  # Config Front-end Module
  FEModule0_0 = FEModule()
  FEModule0_0.AddFE(FE0_0_0)
  FEModule0_0.ConfigureGlobal(globalSettings)

  # Config BeBoardModule
  BeBoardModule0 = BeBoardModule()
  BeBoardModule0.AddFEModule(FEModule0_0)

  # Config HWDescription
  HWDescription = HWDescription()
  HWDescription.AddBeBoard(BeBoardModule0)
  HWDescription.AddSettings(HWSettings)

  GenerateHWDescriptionXML(HWDescription)

  
