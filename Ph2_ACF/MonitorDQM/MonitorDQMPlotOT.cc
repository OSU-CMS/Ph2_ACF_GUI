#include "MonitorDQM/MonitorDQMPlotOT.h"
#include "Parser/DetectorMonitorConfig.h"
#include "RootUtils/GraphContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "TAxis.h"
#include "TFile.h"
#include "TGraph.h"
#include "Utils/Container.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/ValueAndTime.h"

//========================================================================================================================
MonitorDQMPlotOT::MonitorDQMPlotOT() {}

//========================================================================================================================
MonitorDQMPlotOT::~MonitorDQMPlotOT() {}

//========================================================================================================================
void MonitorDQMPlotOT::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR DQM YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorData ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    for(const auto& monitorValueName: detectorMonitorConfig.fMonitorElementList.at("LpGBT"))
        if(monitorValueName.second) bookLpGBTPlots(theOutputFile, monitorValueName.first);
}

//========================================================================================================================
void MonitorDQMPlotOT::bookLpGBTPlots(TFile* theOutputFile, const std::string& monitorValueName)
{
    std::string yAxisUnits = "V";
    if(monitorValueName == "LpGBTtemp" || monitorValueName == "SensorTemp" || monitorValueName == "BPOL12Vtemp" || monitorValueName == "BPOL2V5temp") yAxisUnits = "#circC";
    if(monitorValueName == "VTRxLeakageCurr") yAxisUnits = "mA";
    auto theTGraphHistogramContainer = producePlotTemplate(monitorValueName, "LpGBT", yAxisUnits);
    RootContainerFactory::bookOpticalGroupHistograms<GraphContainer<TGraph>>(theOutputFile, *fDetectorContainer, fLpGBTRegisterMonitorPlotMap[monitorValueName], theTGraphHistogramContainer);
}

//========================================================================================================================
void MonitorDQMPlotOT::fillLpGBTmonitorPlots(DetectorDataContainer& theInputContainer, const std::string& monitorValueName)
{
    if(fLpGBTRegisterMonitorPlotMap.find(monitorValueName) == fLpGBTRegisterMonitorPlotMap.end())
    {
        LOG(ERROR) << BOLDRED << "No plots for LpGBT register " << monitorValueName << RESET;
        LOG(ERROR) << BOLDRED << "Check that DQM and Monitor register names matches" << RESET;
        std::string errorMessage = "No plots for LpGBT register " + monitorValueName + " - Check that DQM and Monitor register names matches";
        throw std::runtime_error(errorMessage);
    }

    for(auto board: theInputContainer) // for on boards - begin
    {
        size_t boardId = board->getId();
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            if(!opticalGroup->hasSummary()) continue;
            size_t  opticalGroupId  = opticalGroup->getId();
            TGraph* LpGBTDQMPlot    = fLpGBTRegisterMonitorPlotMap.at(monitorValueName).getObject(boardId)->getObject(opticalGroupId)->getSummary<GraphContainer<TGraph>>().fTheGraph;
            auto    theValueAndTime = opticalGroup->getSummary<ValueAndTime<float>>();
            LpGBTDQMPlot->SetPoint(LpGBTDQMPlot->GetN(), getTimeStampForRoot(theValueAndTime.fTime), theValueAndTime.fValue);
        } // for on opticalGroup - end
    } // for on boards - end
}

//========================================================================================================================
void MonitorDQMPlotOT::process() {}

//========================================================================================================================
void MonitorDQMPlotOT::reset(void) {}

//========================================================================================================================
bool MonitorDQMPlotOT::fill(std::string& inputStream)
{
    ContainerSerialization theLpGBTRegisterSerialization("MonitorOTLpGBTMonitor");

    if(theLpGBTRegisterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched MonitorOT LpGBTRegister!!!!!\n";
        std::string           monitorValueName;
        DetectorDataContainer fDetectorData =
            theLpGBTRegisterSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, ValueAndTime<float>, EmptyContainer>(fDetectorContainer, monitorValueName);
        fillLpGBTmonitorPlots(fDetectorData, monitorValueName);
        return true;
    }
    return false;
}

//========================================================================================================================
GraphContainer<TGraph> MonitorDQMPlotOT::producePlotTemplate(const std::string& monitorValueName, const std::string& chipName, std::string yAxisUnits)
{
    GraphContainer<TGraph> theTGraphPedestalContainer(0);
    theTGraphPedestalContainer.setNameTitle(chipName + "_DQM_" + monitorValueName, chipName + "_DQM_" + monitorValueName);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeDisplay(1);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetNdivisions(503);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeFormat(TIME_FORMAT);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTimeOffset(0);
    theTGraphPedestalContainer.fTheGraph->GetXaxis()->SetTitle("Local Time");
    theTGraphPedestalContainer.fTheGraph->GetYaxis()->SetTitle((monitorValueName + (yAxisUnits.length() > 0 ? (" [" + yAxisUnits) + "]" : "")).c_str());
    theTGraphPedestalContainer.fTheGraph->SetMarkerStyle(20);
    theTGraphPedestalContainer.fTheGraph->SetMarkerSize(0.4);

    return theTGraphPedestalContainer;
}

//========================================================================================================================
void MonitorDQMPlotOT::fillReadoutChipPlots(DetectorDataContainer&                              theInputContainer,
                                            const std::string&                                  monitorValueName,
                                            const std::map<std::string, DetectorDataContainer>& theValueMonitorPlotMap,
                                            FrontEndType                                        theFrontEndType)
{
    if(theValueMonitorPlotMap.find(monitorValueName) == theValueMonitorPlotMap.end())
    {
        LOG(ERROR) << BOLDRED << "No plots for chip value " << monitorValueName << RESET;
        LOG(ERROR) << BOLDRED << "Check that DQM and Monitor value names matches" << RESET;
        std::string errorMessage = "No plots for chip value " + monitorValueName + " - Check that DQM and Monitor value names matches";
        throw std::runtime_error(errorMessage);
    }

    for(auto board: theInputContainer) // for on boards - begin
    {
        size_t boardId = board->getId();
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            size_t opticalGroupId = opticalGroup->getId();
            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                size_t hybridId = hybrid->getId();
                for(auto chip: *hybrid) // for on chip - begin
                {
                    size_t chipId       = chip->getId();
                    auto theReadoutChip = static_cast<const Ph2_HwDescription::ReadoutChip*>(fDetectorContainer->getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId));
                    if(theReadoutChip->getFrontEndType() != theFrontEndType) continue;
                    // Retreive the corresponging chip histogram:
                    TGraph* chipDQMPlot = theValueMonitorPlotMap.at(monitorValueName)
                                              .getObject(boardId)
                                              ->getObject(opticalGroupId)
                                              ->getObject(hybridId)
                                              ->getObject(chipId)
                                              ->getSummary<GraphContainer<TGraph>>()
                                              .fTheGraph;

                    // Check if the chip data are there (it is needed in the case of the SoC when data may be sent chip
                    // by chip and not in one shot)
                    if(!chip->hasSummary()) continue;
                    auto theValueAndTime = chip->getSummary<ValueAndTime<float>>();
                    chipDQMPlot->SetPoint(chipDQMPlot->GetN(), getTimeStampForRoot(theValueAndTime.fTime), theValueAndTime.fValue); // for on channel - end
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on boards - end
}
