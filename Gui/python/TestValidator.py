import ROOT

ROOT.gROOT.SetBatch(ROOT.kTRUE)

import os
import re
from collections import defaultdict
from Gui.python.ROOTInterface import (
    GetDirectory,
    TCanvas2SVG,
)
from Gui.GUIutils.settings import ModuleLaneMap
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map
from felis import Felis


def ResultGrader(inputDir, testName, runNumber, moduleType):    
    #rubric_name = "rubricV2.py"
    try:
        module_name = inputDir.split("Module")[1].split('_')[0]
        ROOT_file_path = "{0}/Run{1}_{2}.root".format(
            inputDir, runNumber, testName.split('_')[0].rstrip("Scan")
        )
        chip_canvases = [
            f"Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_{i}" for i in ModuleLaneMap[moduleType].values()
        ]
        relevant_files = [os.fsdecode(file) for file in os.listdir(inputDir)]
        
        my_felis = Felis(inputDir, False)
        sanity, message = my_felis.set_result(
            ROOT_file_path,
            chip_canvases,
            relevant_files,
            module_name,
            testName,
            Test_to_Ph2ACF_Map[testName],
        )
        
        return sanity
    except Exception as err:
        print("Failed to get the score: {}".format(repr(err)))
        return False


def GetCanvasVAL(node, canvasList, ModuleMap):
    if node.getDaugthers() != []:
        for Node in node.getDaugthers():
            if Node.getClassName() == "TCanvas":
                CanvasName = Node.getKeyName()
                FWConfigList = CanvasName.split("_")
                BeboardID = re.sub(r"[A-Z]\(|\)", "", FWConfigList[1])
                OGID = re.sub(r"[A-Z]\(|\)", "", FWConfigList[2])
                HyBridID = re.sub(r"[A-Z]\(|\)", "", FWConfigList[3])
                ChipID = re.sub(r"[A-Z]\(|\)", "", FWConfigList[5])
                ModulePath = "{0}_{1}_{2}".format(BeboardID, OGID, HyBridID)
                if ModulePath in ModuleMap.keys():
                    ModuleName = ModuleMap[ModulePath]
                else:
                    continue
                # Module_ID_Sec = CanvasName.split("_")[2] ##To be CHECKED!!!!!
                # if ("H" in Module_ID_Sec):
                # 	Module_ID = Module_ID_Sec.lstrip("H(").rstrip(")")
                # elif ("M" in Module_ID_Sec):
                # 	Module_ID = Module_ID_Sec.lstrip("M(").rstrip(")")
                # elif ("O" in Module_ID_Sec):
                # 	Module_ID = Module_ID_Sec.lstrip("O(").rstrip(")")
                # else:
                # 	Module_ID = -1
                # Chip_ID = CanvasName.split("_")[5].lstrip("O(").rstrip(")")
                if ModuleName not in canvasList.keys():
                    canvasList[ModuleName] = {}
                    canvasList[ModuleName][CanvasName] = Node.getObject()
                else:
                    canvasList[ModuleName][CanvasName] = Node.getObject()
            canvasList = GetCanvasVAL(Node, canvasList, ModuleMap)
        return canvasList
    else:
        return canvasList


def GradePixelAlive(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            PlotName = CanvasName.split("_")[4]
            if PlotName in ["Occ1D", "PixelAlive", "ToT1D", "ToT2D"]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Occ1D" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                AvgOccu = CanvasHist.GetMean()
                factorPerModule[key][Chip_ID]["AvgOccu"] = AvgOccu
            if "PixelAlive" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist2D = CanvasObj.GetPrimitive(CanvasName)
                RowRange = [0, 191]
                nRowRange = RowRange[1] - RowRange[0] + 1
                ColRange = [128, 263]
                nColRange = ColRange[1] - ColRange[0] + 1
                Hist2D_Scanned = ROOT.TH2F(
                    "Hist2D_Scanned",
                    "Hist2D_Scanned",
                    nColRange,
                    -0.5,
                    nColRange - 0.5,
                    nRowRange,
                    -0.5,
                    nRowRange - 0.5,
                )
                Hist2D_Ref = ROOT.TH2F(
                    "Hist2D_Ref",
                    "Hist2D_Ref",
                    nColRange,
                    -0.5,
                    nColRange - 0.5,
                    nRowRange,
                    -0.5,
                    nRowRange - 0.5,
                )

                Eff_threshold = 0.90
                for row in range(RowRange[0] + 1, RowRange[1] + 1):
                    for col in range(ColRange[0] + 1, ColRange[1] + 1):
                        Hist2D_Scanned.SetBinContent(
                            col - ColRange[0],
                            row - RowRange[0],
                            0.0
                            if CanvasHist2D.GetBinContent(col, row) > Eff_threshold
                            else 1.0,
                        )

                for row in range(RowRange[0] + 1, RowRange[1] + 1):
                    for col in range(ColRange[0] + 1, ColRange[1] + 1):
                        Hist2D_Ref.SetBinContent(
                            col - ColRange[0],
                            row - RowRange[0],
                            1.0
                            if max(
                                Hist2D_Scanned.GetBinContent(
                                    (col + 1) % nColRange, row
                                ),
                                Hist2D_Scanned.GetBinContent(
                                    (col - 1) % nColRange, row
                                ),
                                Hist2D_Scanned.GetBinContent(
                                    col, (row + 1) % nRowRange
                                ),
                                Hist2D_Scanned.GetBinContent(
                                    col, (row - 1) % nRowRange
                                ),
                            )
                            > Eff_threshold
                            else 0.0,
                        )

                # nLowEffBins = Hist2D_Scanned.Integral()
                nLowEffBinsNonIso = Hist2D_Ref.Integral()
                factorPerModule[key][Chip_ID]["numLowEffNonIsoBin"] = nLowEffBinsNonIso

    nLowEffBinsThreshold = 200
    ChipThreshold = 0.0

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            factorLowEff = (
                1
                - factorPerModule[Module_ID][Chip_ID]["numLowEffNonIsoBin"]
                / nLowEffBinsThreshold
            )
            score = (
                factorPerModule[Module_ID][Chip_ID]["AvgOccu"] * 0
                if factorLowEff < 0.0
                else factorLowEff
            )
            GradePerChip[Chip_ID] = score
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}

        # passModule[Module_ID] = {0 : True}  #FIXME temporary fix to prevent failing grade
    return grade, passModule, figureList


def GradeNoiseScan(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            PlotName = CanvasName.split("_")[4]
            if PlotName in ["Occ1D", "PixelAlive", "ToT1D", "ToT2D"]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Occ1D" in CanvasName:
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                AvgOccu = CanvasHist.GetMean()
                factorPerModule[key][Chip_ID]["AvgOccu"] = AvgOccu

    ChipThreshold = 0.00  # 0.99

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            score = 1 - factorPerModule[Module_ID][Chip_ID]["AvgOccu"]
            GradePerChip[Chip_ID] = score
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def GradeSCurveScan(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)

    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}
            PlotName = CanvasName.split("_")[4]

            if PlotName in [
                "SCurves",
                "Threshold1D",
                "Noise1D",
                "Threshold2D",
                "Noise2D",
                "ToT2D",
            ]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Threshold1D" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                StdDev1D = CanvasHist.GetStdDev()
                factorPerModule[key][Chip_ID]["StdDev1D"] = StdDev1D

    StdThreshold = 15.0
    ChipThreshold = -0.1

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            # score =  1 - factorPerModule[Module_ID][Chip_ID]["StdDev1D"]/StdThreshold
            score = 1.0
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def GradeGainScan(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            PlotName = CanvasName.split("_")[4]

            if PlotName in [
                "Gain",
                "Intercept1D",
                "Slope1D",
                "InterceptLowQ1D",
                "SlopeLowQ1D",
                "Chi2DoF1D",
                "Intercept2D",
                "Slope2D",
                "InterceptLowQ2D",
                "SlopeLowQ2D",
                "Chi2DoF2D",
            ]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Threshold1D" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                StdDev1D = CanvasHist.GetStdDev()
                factorPerModule[key][Chip_ID]["StdDev1D"] = StdDev1D

            ## To Be Corrected
            if True:
                factorPerModule[key][Chip_ID]["fakeScore"] = 1.0

    ChipThreshold = -0.5

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            score = factorPerModule[Module_ID][Chip_ID]["fakeScore"]
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def GradeInjectionDelay(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)

    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}
            PlotName = CanvasName.split("_")[4]

            outputFileName = "{}_{}".format(key, CanvasName)
            CanvasObj = CanvasPerModule[CanvasName]
            outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
            figureList[key].append(outputFile)

            factorPerModule[key][Chip_ID]["injectionscore"] = 1.0

    ChipThreshold = -0.1

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            score = factorPerModule[Module_ID][Chip_ID]["injectionscore"]
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def GradeGainOptimization(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            PlotName = CanvasName.split("_")[4]
            if PlotName in [
                "Gain",
                "Intercept1D",
                "Slope1D",
                "InterceptLowQ1D",
                "SlopeLowQ1D",
                "Chi2DoF1D",
                "Intercept2D",
                "Slope2D",
                "InterceptLowQ2D",
                "SlopeLowQ2D",
                "Chi2DoF2D",
                "KrumCurr",
            ]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Threshold1D" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                StdDev1D = CanvasHist.GetStdDev()
                factorPerModule[key][Chip_ID]["StdDev1D"] = StdDev1D

            if True:
                factorPerModule[key][Chip_ID]["fakeScore"] = 1.0

    ChipThreshold = -0.5

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            score = factorPerModule[Module_ID][Chip_ID]["fakeScore"]
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def GradeThresholdAdjustment(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            PlotName = CanvasName.split("_")[4]
            if PlotName in ["Threshold", "Occ1D", "PixelAlive", "ToT1D", "ToT2D"]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Threshold1D" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                StdDev1D = CanvasHist.GetStdDev()
                factorPerModule[key][Chip_ID]["StdDev1D"] = StdDev1D

            if True:
                factorPerModule[key][Chip_ID]["fakeScore"] = 1.0

    ChipThreshold = -0.5

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            # score =  factorPerModule[Module_ID][Chip_ID]["fakeScore"]
            score = 1.0
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def GradeThresholdEqualization(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            PlotName = CanvasName.split("_")[4]
            if PlotName in [
                "ThrEqualization",
                "TDAC",
                "Occ1D",
                "PixelAlive",
                "ToT1D",
                "ToT2D",
            ]:
                outputFileName = "{}_{}".format(key, CanvasName)
                CanvasObj = CanvasPerModule[CanvasName]
                outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
                figureList[key].append(outputFile)

            if "Threshold1D" in CanvasName:
                CanvasObj = CanvasPerModule[CanvasName]
                CanvasHist = CanvasObj.GetPrimitive(CanvasName)
                StdDev1D = CanvasHist.GetStdDev()
                factorPerModule[key][Chip_ID]["StdDev1D"] = StdDev1D

            if True:
                factorPerModule[key][Chip_ID]["fakeScore"] = 1.0

    ChipThreshold = -0.5

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            # score =  factorPerModule[Module_ID][Chip_ID]["fakeScore"]
            score = 1.0
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


def FakeGrade(canvasList):
    grade = {}
    passModule = {}
    factorPerModule = {}
    figureList = defaultdict(lambda: [])
    # Saving histogram for future check
    tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
    if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
        try:
            os.mkdir(tmpDir)
            logger.info("Creating " + tmpDir)
        except:
            logger.warning("Failed to create " + tmpDir)
    if len(canvasList.keys()) == 0:
        return grade
    for key in canvasList.keys():
        CanvasPerModule = canvasList[key]
        factorPerModule[key] = {}
        for CanvasName in CanvasPerModule.keys():
            Chip_ID = CanvasName.split("_")[5].lstrip("Chip(").rstrip(")")
            if Chip_ID not in factorPerModule[key].keys():
                factorPerModule[key][Chip_ID] = {}

            outputFileName = "{}_{}".format(key, CanvasName)
            CanvasObj = CanvasPerModule[CanvasName]
            outputFile = TCanvas2SVG(tmpDir, CanvasObj, outputFileName)
            figureList[key].append(outputFile)

            if True:
                factorPerModule[key][Chip_ID]["fakeScore"] = 1.0

    ChipThreshold = 0.5

    for Module_ID in factorPerModule.keys():
        GradePerChip = {}
        passChip = {}
        for Chip_ID in factorPerModule[Module_ID].keys():
            score = factorPerModule[Module_ID][Chip_ID]["fakeScore"]
            GradePerChip[Chip_ID] = score if score > 0.0 else 0.0
            passChip[Chip_ID] = score > ChipThreshold
        if GradePerChip != {}:
            grade[Module_ID] = GradePerChip
            passModule[Module_ID] = passChip
        else:
            grade[Module_ID] = {0: -1}
            passModule[Module_ID] = {0: False}
    return grade, passModule, figureList


if __name__ == "__main__":
    ResultGrader("/Users/czkaiweb/Research/data", "PixelAlive", "000047")
    ResultGrader("/Users/czkaiweb/Research/data", "NoiseScan", "000026")
    ResultGrader("/Users/czkaiweb/Research/data", "Physics", "000026")
