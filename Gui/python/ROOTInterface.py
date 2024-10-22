import ROOT
import time
from Gui.python.logging_config import logger

ROOT.gROOT.SetBatch(ROOT.kTRUE)
ROOT.gStyle.SetPalette(57)


class Node:
    def __init__(self, keyname, obj, className):
        self.KeyName = keyname
        self.ClassName = className
        self.Obj = obj
        self.Daughters = []

    def appendDaugther(self, node):
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
        subNode = Node(keyName, obj, className)
        node.appendDaugther(subNode)
        if className == "TDirectoryFile":
            GetSubDirectory(obj, subNode)


def GetDirectory(inputFile):
    ROOT_Nodes = []

    try:
        openFile = ROOT.TFile.Open(inputFile, "READ")
    except IOError:
        print("File: {0} not opened".format(inputFile))

    ListOfRoots = openFile.GetListOfKeys()

    for key in ListOfRoots:
        obj = key.ReadObj()
        keyName = key.GetName()
        className = key.GetClassName()
        node = Node(keyName, obj, className)
        ROOT_Nodes.append(node)
        if key.GetClassName() == "TDirectoryFile":
            GetSubDirectory(obj, node)

    return ROOT_Nodes


def DirectoryVLR(node, depth):
    nodeName = "-" * depth + node.getKeyName()
    print(nodeName + ";" + node.getClassName())
    if node.getClassName() == "TCanvas":
        obj = node.getObject()
        obj.SetBatch(ROOT.kTRUE)
        obj.Draw()
        obj.SaveAs(node.getKeyName() + ".jpg")
    for node in node.getDaugthers():
        DirectoryVLR(node, depth + 1)


def showDirectory(nodes):
    for node in nodes:
        keyName = node.getKeyName()
        DirectoryVLR(node, 0)


def TCanvas2JPG(outputDir, canvas, name=None):
    ROOT.gStyle.SetPalette(57)
    if not name:
        seconds = time.time()
        outputFile = outputDir + "/display{}.jpg".format(seconds)
    else:
        outputFile = outputDir + "/{}.jpg".format(name)
    try:
        canvas.SetBatch(ROOT.kTRUE)
        canvas.Draw()
        canvas.Print(outputFile)
        # canvas.Close()
        logger.info(outputFile + " is saved")
    except:
        logger.warning("Failed to save " + outputFile)
    return outputFile


def TCanvas2SVG(outputDir, canvas, name=None):
    ROOT.gStyle.SetPalette(57)
    if not name:
        seconds = time.time()
        outputFile = outputDir + "/display{}.svg".format(seconds)
    else:
        outputFile = outputDir + "/{}.svg".format(name)
    try:
        canvas.SetBatch(ROOT.kTRUE)
        canvas.Draw()
        if "SCurve" in name:
            canvas.SetLogz()
        if "PixelAlive" in name:
            ROOT.gStyle.SetOptStat(0) #no statistics box
        else:
            ROOT.gStyle.SetOptStat(1111) #default statistics box
        canvas.Print(outputFile)
        # canvas.Close()
        logger.info(outputFile + " is saved")
    except Exception as e:
        logger.warning("Failed to save " + outputFile + "\nReason: " + str(e))
    return outputFile


def GetBinary(fileName):
    binaryData = ROOT.TFile(fileName)
    return binaryData


#@precondition: commands must be a list of strings representing valid root shell commands
def executeCommandSequence(commands: list):
    for command in commands:
        ROOT.gROOT.ProcessLine(command)


if __name__ == "__main__":
    nodes = GetDirectory("/Users/czkaiweb/Research/data/Run0060_ThrMinimization.root")
    showDirectory(nodes)
