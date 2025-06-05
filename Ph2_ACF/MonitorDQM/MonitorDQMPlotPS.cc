/*!
        \file                MonitorDQMPlotPS.cc
        \brief               DQM class for DQM example -> use it as a templare
        \author              Fabio Ravera
        \date                25/7/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#include "MonitorDQM/MonitorDQMPlotPS.h"
#include "HWDescription/ReadoutChip.h"
#include "Parser/DetectorMonitorConfig.h"
#include "RootUtils/GraphContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/ValueAndTime.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
MonitorDQMPlotPS::MonitorDQMPlotPS() {}

//========================================================================================================================
MonitorDQMPlotPS::~MonitorDQMPlotPS() {}

//========================================================================================================================
void MonitorDQMPlotPS::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig)
{
    MonitorDQMPlotOT::book(theOutputFile, theDetectorStructure, detectorMonitorConfig);

    auto        MPAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string theMPAqueryFunctionString = "MPAqueryFunction";
    theDetectorStructure.addReadoutChipQueryFunction(MPAqueryFunction, theMPAqueryFunctionString);
    for(const auto& registerName: detectorMonitorConfig.fMonitorElementList.at("MPA2"))
        if(registerName.second) bookMPA2Plots(theOutputFile, registerName.first);
    theDetectorStructure.removeReadoutChipQueryFunction(theMPAqueryFunctionString);

    auto        SSAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string theSSAqueryFunctionString = "SSAqueryFunction";
    theDetectorStructure.addReadoutChipQueryFunction(SSAqueryFunction, theSSAqueryFunctionString);
    for(const auto& registerName: detectorMonitorConfig.fMonitorElementList.at("SSA2"))
        if(registerName.second) bookSSA2Plots(theOutputFile, registerName.first);
    theDetectorStructure.removeReadoutChipQueryFunction(theSSAqueryFunctionString);
}

//========================================================================================================================
void MonitorDQMPlotPS::bookSSA2Plots(TFile* theOutputFile, std::string registerName)
{
    std::string yAxisUnits = "V";
    if(registerName == "temp") yAxisUnits = "#circC";
    auto theTGraphHistogramContainer = producePlotTemplate(registerName, "SSA", yAxisUnits);
    RootContainerFactory::bookChipHistograms<GraphContainer<TGraph>>(theOutputFile, *fDetectorContainer, fSSA2RegisterMonitorPlotMap[registerName], theTGraphHistogramContainer);
}

//========================================================================================================================
void MonitorDQMPlotPS::bookMPA2Plots(TFile* theOutputFile, std::string registerName)
{
    std::string yAxisUnits = "V";
    if(registerName == "temp") yAxisUnits = "#circC";
    auto theTGraphHistogramContainer = producePlotTemplate(registerName, "MPA", yAxisUnits);
    RootContainerFactory::bookChipHistograms<GraphContainer<TGraph>>(theOutputFile, *fDetectorContainer, fMPA2RegisterMonitorPlotMap[registerName], theTGraphHistogramContainer);
}

//========================================================================================================================
void MonitorDQMPlotPS::fillSSA2RegisterPlots(DetectorDataContainer& theInputContainer, const std::string& registerName)
{
    fillReadoutChipPlots(theInputContainer, registerName, fSSA2RegisterMonitorPlotMap, FrontEndType::SSA2);
}

//========================================================================================================================
void MonitorDQMPlotPS::fillMPA2RegisterPlots(DetectorDataContainer& theInputContainer, const std::string& registerName)
{
    fillReadoutChipPlots(theInputContainer, registerName, fMPA2RegisterMonitorPlotMap, FrontEndType::MPA2);
}

//========================================================================================================================
void MonitorDQMPlotPS::process() { MonitorDQMPlotOT::process(); }

//========================================================================================================================
void MonitorDQMPlotPS::reset(void) { MonitorDQMPlotOT::reset(); }

//========================================================================================================================
bool MonitorDQMPlotPS::fill(std::string& inputStream)
{
    if(MonitorDQMPlotOT::fill(inputStream)) return true;

    ContainerSerialization theMPA2RegisterSerialization("PSMonitorMPA2Register");
    ContainerSerialization theSSA2RegisterSerialization("PSMonitorSSA2Register");
    ContainerSerialization theLpGBTRegisterSerialization("PSMonitorLpGBTRegister");

    if(theMPA2RegisterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PSMonitor MPA2Register!!!!!\n";
        std::string           registerName;
        DetectorDataContainer fDetectorData =
            theMPA2RegisterSerialization.deserializeBoardContainer<EmptyContainer, ValueAndTime<float>, EmptyContainer, EmptyContainer, EmptyContainer>(fDetectorContainer, registerName);
        fillMPA2RegisterPlots(fDetectorData, registerName);
        return true;
    }
    if(theSSA2RegisterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PSMonitor MPA2Register!!!!!\n";
        std::string           registerName;
        DetectorDataContainer fDetectorData =
            theSSA2RegisterSerialization.deserializeBoardContainer<EmptyContainer, ValueAndTime<float>, EmptyContainer, EmptyContainer, EmptyContainer>(fDetectorContainer, registerName);
        fillSSA2RegisterPlots(fDetectorData, registerName);
        return true;
    }
    if(theLpGBTRegisterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PSMonitor LpGBTRegister!!!!!\n";
        std::string           registerName;
        DetectorDataContainer fDetectorData =
            theLpGBTRegisterSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, ValueAndTime<float>, EmptyContainer>(fDetectorContainer, registerName);
        fillLpGBTmonitorPlots(fDetectorData, registerName);
        return true;
    }
    return false;
}
