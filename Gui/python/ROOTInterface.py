import ROOT

import logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

ROOT.gROOT.SetBatch(ROOT.kTRUE)

class Node():
	def __init__(self, keyname, obj, className):
		self.KeyName = keyname
		self.ClassName = className
		self.Obj = obj
		self.Daughters = []


	def appendDaugther(self,node):
		self.Daughters.append(node)

	def getKeyName(self):
		return self.KeyName

	def getClassName(self):
		return self.ClassName

	def getObject(self):
		return self.Obj
	
	def getDaugthers(self):
		return self.Daughters


def GetSubDirectory(obj, node):
	ListOfSubNodes = obj.GetListOfKeys()

	for key in ListOfSubNodes:
		obj = key.ReadObj()
		keyName = str(key.GetName())
		className = key.GetClassName()
		subNode = Node(keyName,obj,className)
		node.appendDaugther(subNode)
		if className == "TDirectoryFile":
			GetSubDirectory(obj,subNode)

def GetDirectory(inputFile):
	ROOT_Nodes = []

	try:   
		openFile = ROOT.TFile.Open(inputFile,"READ")
	except IOError:
		print("File: {0} not opened".format(inputFile))

	ListOfRoots = openFile.GetListOfKeys()

	for key in ListOfRoots:
		obj = key.ReadObj()
		keyName = key.GetName()
		className = key.GetClassName()
		node = Node(keyName,obj,className)
		ROOT_Nodes.append(node)
		if key.GetClassName() == "TDirectoryFile":
			GetSubDirectory(obj,node)
	
	return  ROOT_Nodes

def DirectoryVLR(node,depth):
	nodeName = "-"*depth + node.getKeyName()
	print(nodeName+";"+node.getClassName())
	if node.getClassName() == "TCanvas":
		obj = node.getObject()
		obj.Draw()
		obj.SaveAs(node.getKeyName()+".jpg")
	for node in node.getDaugthers():
		DirectoryVLR(node,depth+1)
	
def showDirectory(nodes):
	for node in nodes:
		keyName = node.getKeyName()
		DirectoryVLR(node,0)

def TCanvas2JPG(outputDir, canvas):
	outputFile = outputDir+"/display.jpg"
	try:
		canvas.Draw()
		canvas.Print(outputFile)
		#canvas.Close()
		logger.info(outputFile + " is saved")
	except:
		logger.warning("Failed to save "+ outputFile)
	return outputFile

if __name__ == "__main__":
	nodes =  GetDirectory("/Users/czkaiweb/Research/data/Run0060_ThrMinimization.root")
	showDirectory(nodes)
			

	
	

