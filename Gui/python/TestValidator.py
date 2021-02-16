import ROOT
ROOT.gROOT.SetBatch(ROOT.kTRUE)

import os
from  Gui.python.ROOTInterface import *

def ResultGrader(inputDir, testName, runNumber):
	Grade = {}
	PassModule = {}
	ExpectedModuleList = [ module.lstrip("Module") for module in inputDir.split('_') if "Module" in module]
	if testName == "pixelalive":
		CanvasList = {}
		PixelAliveFileName = "{0}/Run{1}_PixelAlive.root".format(inputDir,runNumber)
		if os.path.isfile(PixelAliveFileName):
			Nodes = GetDirectory(PixelAliveFileName)
			for Node in Nodes:
				CanvasList = GetCanvasVAL(Node, CanvasList)
			Grade, PassModule = GradePixelAlive(CanvasList)
			if set(Grade.keys()) != set(ExpectedModuleList):
				logger.warning("Retrived modules from ROOT file doesn't match with folder name")
		else:
			for module in ExpectedModuleList:
				Grade[module] = {0 : -1.0}
				PassModule[module] = {0: False}


	elif testName == "noise":
		CanvasList = {}
		NoiseScanFileName = "{0}/Run{1}_NoiseScan.root".format(inputDir,runNumber)
		if os.path.isfile(NoiseScanFileName):
			Nodes = GetDirectory(NoiseScanFileName)
			for Node in Nodes:
				CanvasList = GetCanvasVAL(Node, CanvasList)
			Grade, PassModule = GradeNoiseScan(CanvasList)
			if set(Grade.keys()) != set(ExpectedModuleList):
				logger.warning("Retrived modules from ROOT file doesn't match with folder name")
		else:
			for module in ExpectedModuleList:
				Grade[module] = {0 : -1.0}
				PassModule[module] = {0: False}

	elif testName == "scurve":
		CanvasList = {}
		SCurveFileName = "{0}/Run{1}_SCurve.root".format(inputDir,runNumber)
		if os.path.isfile(SCurveFileName):
			Nodes = GetDirectory(SCurveFileName)
			for Node in Nodes:
				CanvasList = GetCanvasVAL(Node, CanvasList)
			Grade, PassModule = GradeSCurve(CanvasList)
			if set(Grade.keys()) != set(ExpectedModuleList):
				logger.warning("Retrived modules from ROOT file doesn't match with folder name")
		else:
			for module in ExpectedModuleList:
				Grade[module] = {0 : -1.0}
				PassModule[module] = {0: False}
				
	else:
		for module in ExpectedModuleList:
			Grade[module] = {0: -1.0}
			PassModule[module] = {0: False}

	for key in Grade.keys():
		gradeFile = open("{}/Grade_Module{}.txt".format(inputDir,key),"w")
		gradeFile.write("Test:       {}\n".format(testName))
		gradeFile.write("runNumber:  {}\n".format(runNumber))
		gradeFile.write("Grade:      {}\n".format(min(Grade[key].values())))
		gradeFile.close()

	return Grade, PassModule

def GetCanvasVAL(node,canvasList):
	if node.getDaugthers() != []:
			for Node in node.getDaugthers():
				if Node.getClassName() ==  "TCanvas":
					CanvasName = Node.getKeyName()
					Module_ID = CanvasName.split("_")[3].lstrip("O(").rstrip(")")
					#Chip_ID = CanvasName.split("_")[5].lstrip("O(").rstrip(")")
					if Module_ID not in canvasList.keys():
						canvasList[Module_ID] = {}
						canvasList[Module_ID][CanvasName] = Node.getObject()
					else:
						canvasList[Module_ID][CanvasName] = Node.getObject()
				canvasList = GetCanvasVAL(Node, canvasList)
			return canvasList
	else:
		return canvasList

def GradePixelAlive(canvasList):
	grade = {}
	passModule = {}
	factorPerModule = {}
	if len(canvasList.keys())==0:
		return grade
	for key in canvasList.keys():
		CanvasPerModule = canvasList[key]
		factorPerModule[key] = {}
		for CanvasName in CanvasPerModule.keys():
			Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
			if Chip_ID not in factorPerModule[key].keys():
				factorPerModule[key][Chip_ID] = {}
			if "Occ1D" in CanvasName:
				CanvasObj = CanvasPerModule[CanvasName]
				CanvasHist = CanvasObj.GetPrimitive(CanvasName)
				AvgOccu = CanvasHist.GetMean()
				factorPerModule[key][Chip_ID]["AvgOccu"] = AvgOccu
			if "PixelAlive" in CanvasName:
				CanvasObj = CanvasPerModule[CanvasName]
				CanvasHist2D = CanvasObj.GetPrimitive(CanvasName)
				RowRange = [0,191]
				nRowRange = RowRange[1]-RowRange[0]+1
				ColRange = [128,263]
				nColRange = ColRange[1]-ColRange[0]+1
				Hist2D_Scanned = ROOT.TH2F("Hist2D_Scanned","Hist2D_Scanned", \
					nColRange, -0.5, nColRange-0.5,	\
					nRowRange, -0.5, nRowRange-0.5)
				Hist2D_Ref = ROOT.TH2F("Hist2D_Ref","Hist2D_Ref", \
					nColRange, -0.5, nColRange-0.5,	\
					nRowRange, -0.5, nRowRange-0.5)

				Eff_threshold = 0.90
				for row in range(RowRange[0]+1,RowRange[1]+1):
					for col in range(ColRange[0]+1,ColRange[1]+1):
						Hist2D_Scanned.SetBinContent(col-ColRange[0],row-RowRange[0],  0.0 if CanvasHist2D.GetBinContent(col,row) > Eff_threshold else 1.0)

				for row in range(RowRange[0]+1,RowRange[1]+1):
					for col in range(ColRange[0]+1,ColRange[1]+1):	
						Hist2D_Ref.SetBinContent(col-ColRange[0],row-RowRange[0],	\
						1.0 if max( \
							Hist2D_Scanned.GetBinContent((col+1)%nColRange,row), \
							Hist2D_Scanned.GetBinContent((col-1)%nColRange,row), \
							Hist2D_Scanned.GetBinContent(col,(row+1)%nRowRange), \
							Hist2D_Scanned.GetBinContent(col,(row-1)%nRowRange)) \
							> Eff_threshold else 0.0)

				#nLowEffBins = Hist2D_Scanned.Integral()
				nLowEffBinsNonIso = Hist2D_Ref.Integral()
				factorPerModule[key][Chip_ID]["numLowEffNonIsoBin"] = nLowEffBinsNonIso

	nLowEffBinsThreshold = 200
	ChipThreshold = 0.5

	for Module_ID in factorPerModule.keys():
		GradePerChip = {}
		passChip = {}
		for Chip_ID in factorPerModule[Module_ID].keys():
			factorLowEff = (1-factorPerModule[Module_ID][Chip_ID]["numLowEffNonIsoBin"]/nLowEffBinsThreshold)
			score =  factorPerModule[Module_ID][Chip_ID]["AvgOccu"] * 0 if factorLowEff < 0.0 else factorLowEff
			GradePerChip[Chip_ID] = score
			passChip[Chip_ID] = score > ChipThreshold 
		if GradePerChip != {}:
			grade[Module_ID] = GradePerChip
			passModule[Module_ID] = passChip
		else:
			grade[Module_ID] = {0 : -1}
			passModule[Module_ID] = {0 : False}
	return grade, passModule

def GradeNoiseScan(canvasList):
	grade = {}
	passModule = {}
	factorPerModule = {}
	if len(canvasList.keys())==0:
		return grade
	for key in canvasList.keys():
		CanvasPerModule = canvasList[key]
		factorPerModule[key] = {}
		for CanvasName in CanvasPerModule.keys():
			Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
			if Chip_ID not in factorPerModule[key].keys():
				factorPerModule[key][Chip_ID] = {}
			
			if "Occ1D" in CanvasName:
				CanvasObj = CanvasPerModule[CanvasName]
				CanvasHist = CanvasObj.GetPrimitive(CanvasName)
				AvgOccu = CanvasHist.GetMean()
				factorPerModule[key][Chip_ID]["AvgOccu"] = AvgOccu 

	ChipThreshold = 0.99

	for Module_ID in factorPerModule.keys():
		GradePerChip = {}
		passChip = {}
		for Chip_ID in factorPerModule[Module_ID].keys():
			score =  1 - factorPerModule[Module_ID][Chip_ID]["AvgOccu"] 
			GradePerChip[Chip_ID] = score 
			passChip[Chip_ID] = score > ChipThreshold 
		if GradePerChip != {}:
			grade[Module_ID] = GradePerChip
			passModule[Module_ID] = passChip
		else:
			grade[Module_ID] = {0 : -1}
			passModule[Module_ID] = {0 : False}
	return grade, passModule
	
def GradeSCurve(canvasList):
	grade = {}
	passModule = {}
	factorPerModule = {}
	if len(canvasList.keys())==0:
		return grade
	for key in canvasList.keys():
		CanvasPerModule = canvasList[key]
		factorPerModule[key] = {}
		for CanvasName in CanvasPerModule.keys():
			Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
			if Chip_ID not in factorPerModule[key].keys():
				factorPerModule[key][Chip_ID] = {}
			
			if "Threshold1D" in CanvasName:
				CanvasObj = CanvasPerModule[CanvasName]
				CanvasHist = CanvasObj.GetPrimitive(CanvasName)
				StdDev1D = CanvasHist.GetStdDev()
				factorPerModule[key][Chip_ID]["StdDev1D"] = StdDev1D

	StdThreshold = 15.0
	ChipThreshold = 0.5

	for Module_ID in factorPerModule.keys():
		GradePerChip = {}
		passChip = {}
		for Chip_ID in factorPerModule[Module_ID].keys():
			score =  1 - factorPerModule[Module_ID][Chip_ID]["StdDev1D"]/StdThreshold
			GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
			passChip[Chip_ID] = score > ChipThreshold
		if GradePerChip != {}:
			grade[Module_ID] = GradePerChip
			passModule[Module_ID] = passChip
		else:
			grade[Module_ID] = {0 : -1}
			passModule[Module_ID] = {0 : False}
	return grade, passModule

if __name__ == "__main__":
	ResultGrader("/Users/czkaiweb/Research/data","pixelalive","000047")