/*!
        \file                MonitorDQMPlotSEH.cc
        \brief               DQM class for DQM example -> use it as a templare
        \author              Fabio Ravera
        \date                25/7/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#include "MonitorDQM/MonitorDQMPlotSEH.h"
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

//========================================================================================================================
MonitorDQMPlotSEH::MonitorDQMPlotSEH() {}

//========================================================================================================================
MonitorDQMPlotSEH::~MonitorDQMPlotSEH() {}

//========================================================================================================================
void MonitorDQMPlotSEH::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR DQM YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorData ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    // for(const auto& registerName: detectorMonitorConfig.fMonitorElementList.at("LpGBT")) if(registerName.second) bookLpGBTPlots(theOutputFile, theDetectorStructure, registerName.first);
    for(const auto& registerName: detectorMonitorConfig.fMonitorElementList.at("PowerSupply"))
        if(registerName.second) bookPowerSupplyPlots(theOutputFile, theDetectorStructure, registerName.first);
    for(const auto& registerName: detectorMonitorConfig.fMonitorElementList.at("TestCard"))
        if(registerName.second) bookTestCardPlots(theOutputFile, theDetectorStructure, registerName.first);
}

// //========================================================================================================================
// void MonitorDQMPlotSEH::bookLpGBTPlots(TFile* theOutputFile, const DetectorContainer& theDetectorStructure, std::string registerName)
// {
//     std::cout << __PRETTY_FUNCTION__ << "Booking plot for register = " << registerName << std::endl;
//     // creating the histograms for all the chips:
//     // create the GraphContainer<TGraph> as you would create a TGraph (it implements some feature needed to avoid memory
//     // leaks in copying histograms like the move constructor)
//     GraphContainer<TGraph> theTGraphPedestalContainer(0);
//     theTGraphPedestalContainer.setNameTitle("LpGBT_DQM_" + registerName, "LpGBT_DQM_" + registerName);
//     theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeDisplay(1);
//     theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetNdivisions(503);
//     theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeFormat(TIME_FORMAT);
//     theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeOffset(0);
//     theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTitle("time");
//     theTGraphPedestalContainer.fTheGraph->GetYaxis()->SetTitle((registerName + " [V]").c_str());
//     theTGraphPedestalContainer.fTheGraph->SetMarkerStyle(20);
//     theTGraphPedestalContainer.fTheGraph->SetMarkerSize(0.4);
//     // create Histograms for all the chips, they will be automatically accosiated to the output file, no need to save
//     // them, change the name for every chip or set their directory
//     RootContainerFactory::bookOpticalGroupHistograms<GraphContainer<TGraph>>(theOutputFile, theDetectorStructure, fLpGBTRegisterMonitorPlotMap[registerName], theTGraphPedestalContainer);
// }
//========================================================================================================================
void MonitorDQMPlotSEH::bookPowerSupplyPlots(TFile* theOutputFile, const DetectorContainer& theDetectorStructure, std::string registerName)
{
    std::cout << __PRETTY_FUNCTION__ << "Booking plot for PowerSupply = " << registerName << std::endl;
    // creating the histograms for all the chips:
    // create the GraphContainer<TGraph> as you would create a TGraph (it implements some feature needed to avoid memory
    // leaks in copying histograms like the move constructor)
    GraphContainer<TGraph> theTGraphPedestalContainer(0);
    theTGraphPedestalContainer.setNameTitle("PowerSupply_DQM_" + registerName, "PowerSupply_DQM_" + registerName);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeDisplay(1);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetNdivisions(503);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeFormat(TIME_FORMAT);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeOffset(0);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTitle("time");
    theTGraphPedestalContainer.fTheGraph->GetYaxis()->SetTitle((registerName).c_str());
    theTGraphPedestalContainer.fTheGraph->SetMarkerStyle(20);
    theTGraphPedestalContainer.fTheGraph->SetMarkerSize(0.4);
    // create Histograms for all the chips, they will be automatically accosiated to the output file, no need to save
    // them, change the name for every chip or set their directory
    RootContainerFactory::bookDetectorHistograms<GraphContainer<TGraph>>(theOutputFile, theDetectorStructure, fPowerSupplyMonitorPlotMap[registerName], theTGraphPedestalContainer);
}
//========================================================================================================================
void MonitorDQMPlotSEH::bookTestCardPlots(TFile* theOutputFile, const DetectorContainer& theDetectorStructure, std::string registerName)
{
    std::cout << __PRETTY_FUNCTION__ << "Booking plot for TestCard = " << registerName << std::endl;
    // creating the histograms for all the chips:
    // create the GraphContainer<TGraph> as you would create a TGraph (it implements some feature needed to avoid memory
    // leaks in copying histograms like the move constructor)
    GraphContainer<TGraph> theTGraphPedestalContainer(0);
    theTGraphPedestalContainer.setNameTitle("TestCard_DQM_" + registerName, "TestCard_DQM_" + registerName);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeDisplay(1);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetNdivisions(503);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeFormat(TIME_FORMAT);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeOffset(0);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTitle("time");
    theTGraphPedestalContainer.fTheGraph->GetYaxis()->SetTitle((registerName).c_str());
    theTGraphPedestalContainer.fTheGraph->SetMarkerStyle(20);
    theTGraphPedestalContainer.fTheGraph->SetMarkerSize(0.4);
    // create Histograms for all the chips, they will be automatically accosiated to the output file, no need to save
    // them, change the name for every chip or set their directory
    RootContainerFactory::bookDetectorHistograms<GraphContainer<TGraph>>(theOutputFile, theDetectorStructure, fTestCardMonitorPlotMap[registerName], theTGraphPedestalContainer);
}

// //========================================================================================================================
// void MonitorDQMPlotSEH::fillLpGBTmonitorPlots(DetectorDataContainer& theThresholdContainer, const std::string& registerName)
// {
//     if(fLpGBTRegisterMonitorPlotMap.find(registerName) == fLpGBTRegisterMonitorPlotMap.end())
//     {
//         LOG(ERROR) << BOLDRED << "No plots for LpGBT register " << registerName << RESET;
//         LOG(ERROR) << BOLDRED << "Check that DQM and Monitor register names matches" << RESET;
//         std::string errorMessage = "No plots for LpGBT register " + registerName + " - Check that DQM and Monitor register names matches";
//         throw std::runtime_error(errorMessage);
//     }

//     for(auto board: theThresholdContainer) // for on boards - begin
//     {
//         size_t boardId = board->getId();
//         for(auto opticalGroup: *board) // for on opticalGroup - begin
//         {
//             if(!opticalGroup->hasSummary()) continue;
//             size_t  opticalGroupId = opticalGroup->getId();
//             TGraph* LpGBTDQMPlot      = fLpGBTRegisterMonitorPlotMap.at(registerName).at(boardId)->getObject(opticalGroupId)->getSummary<GraphContainer<TGraph>>().fTheGraph;
//             auto theValueAndTime = theThresholdContainer.getSummary<ValueAndTime<float>>();
//             LpGBTDQMPlot->SetPoint(LpGBTDQMPlot->GetN(), getTimeStampForRoot(theValueAndTime.fTime), theValueAndTime.fValue * CONVERSION_FACTOR);
//         } // for on opticalGroup - end
//     }     // for on boards - end
// }
//========================================================================================================================
void MonitorDQMPlotSEH::fillPowerSupplyPlots(DetectorDataContainer& theThresholdContainer, const std::string& registerName)
{
    if(fPowerSupplyMonitorPlotMap.find(registerName) == fPowerSupplyMonitorPlotMap.end())
    {
        LOG(ERROR) << BOLDRED << "No plots for PowerSupply " << registerName << RESET;
        LOG(ERROR) << BOLDRED << "Check that DQM and Monitor register names matches" << RESET;
        std::string errorMessage = "No plots for PowerSupply " + registerName + " - Check that DQM and Monitor register names matches";
        throw std::runtime_error(errorMessage);
    }

    TGraph* PowerSupplyDQMPlot = fPowerSupplyMonitorPlotMap.at(registerName).getSummary<GraphContainer<TGraph>>().fTheGraph;
    auto    theValueAndTime    = theThresholdContainer.getSummary<ValueAndTime<float>>();
    PowerSupplyDQMPlot->SetPoint(PowerSupplyDQMPlot->GetN(), getTimeStampForRoot(theValueAndTime.fTime), theValueAndTime.fValue);
}

//========================================================================================================================
void MonitorDQMPlotSEH::fillTestCardPlots(DetectorDataContainer& theThresholdContainer, const std::string& registerName)
{
    if(fTestCardMonitorPlotMap.find(registerName) == fTestCardMonitorPlotMap.end())
    {
        LOG(ERROR) << BOLDRED << "No plots for TestCard " << registerName << RESET;
        LOG(ERROR) << BOLDRED << "Check that DQM and Monitor register names matches" << RESET;
        std::string errorMessage = "No plots for TestCard " + registerName + " - Check that DQM and Monitor register names matches";
        throw std::runtime_error(errorMessage);
    }

    TGraph* TestCardDQMPlot = fTestCardMonitorPlotMap.at(registerName).getSummary<GraphContainer<TGraph>>().fTheGraph;
    auto    theValueAndTime = theThresholdContainer.getSummary<ValueAndTime<float>>();
    TestCardDQMPlot->SetPoint(TestCardDQMPlot->GetN(), getTimeStampForRoot(theValueAndTime.fTime), theValueAndTime.fValue);
}

//========================================================================================================================
void MonitorDQMPlotSEH::process() {}

//========================================================================================================================
void MonitorDQMPlotSEH::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool MonitorDQMPlotSEH::fill(std::string& inputStream)
{
    // FILL not implemented, it will probably never need the dual process
    return false;
}
