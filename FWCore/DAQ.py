import importlib
import sys
import subprocess
from Configuration.XMLUtil import *

class DAQ():
  Procedure = []
  def __init__(self,name):
    self.name = name 
  def ShowProcedure(self):
    for step in self.Procedure:
      print step.type
      print step.kargs
  def Run(self):
    for step in self.Procedure:
      if "ResetFlag" in step.kargs and step.kargs["ResetFlag"] == True:
        command = "CMSITminiDAQ -r"
        process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
        process.wait()
      if "HWDescription" in step.kargs:
        HWDescription = step.kargs["HWDescription"]
      else:
        HWDescription = "CMSIT.xml"
      command = "CMSITminiDAQ -f %s -c %s" %(HWDescription, test)
      process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
      process.wait()
    

class Step():
  def __init__(self,type_,*arg,**kargs):
    self.type  = type_
    self.arg   = arg 
    self.kargs = kargs

def Process(*arg,**kargs):
  procedure = []
  for step in arg:
      procedure.append(step)
  return procedure


def SetConfiguration(name,path):
  config = importlib.import_module(path)
  description = eval("config.%s"%(name))
  return description

def ConfigureBoard(BoardDescription, outputFile):
  GenerateHWDescriptionXML(BoardDescription,outputFile)
  return outputFile
    
  
