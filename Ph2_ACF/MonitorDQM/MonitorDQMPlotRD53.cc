/*!
  \file                  MonitorDQMPlotRD53.cc
  \brief                 Implementaion of DQM monitoring process
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "MonitorDQM/MonitorDQMPlotRD53.h"
#include "Parser/DetectorMonitorConfig.h"
#include "TGraph.h"

void MonitorDQMPlotRD53::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& fDetectorMonitorConfig)
{
    fDetectorContainer = &theDetectorStructure;

    for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("RD53"))
        if(registerName.second)
        {
            auto graphContainer = GraphContainer<TGraph>(0);
            bookImplementer(theOutputFile, theDetectorStructure, fRegisterMonitorPlotMap[registerName.first], graphContainer, "chip", "Time", registerName.first.c_str());
        }

    for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("LpGBT"))
        if(registerName.second)
        {
            auto graphContainer = GraphContainer<TGraph>(0);
            bookImplementer(theOutputFile, theDetectorStructure, fRegisterMonitorPlotMap[registerName.first], graphContainer, "opto", "Time", registerName.first.c_str());
        }
}

bool MonitorDQMPlotRD53::fill(std::string& inputStream)
{
    ContainerSerialization theChipContainerSerialization("ITMonitorChipRegister");
    ContainerSerialization theOptoContainerSerialization("ITMonitorOptoRegister");

    if(theChipContainerSerialization.attachDeserializer(inputStream))
    {
        LOG(INFO) << GREEN << "Matched IT chip register" << RESET;
        std::string           registerName;
        DetectorDataContainer fDetectorData = theChipContainerSerialization.deserializeChipContainer<EmptyContainer, ValueAndTime<float>>(fDetectorContainer, registerName);
        MonitorDQMPlotRD53::fillChipPlots(fDetectorData, registerName);
        return true;
    }
    if(theOptoContainerSerialization.attachDeserializer(inputStream))
    {
        LOG(INFO) << GREEN << "Matched IT opto register" << RESET;
        std::string           registerName;
        DetectorDataContainer fDetectorData =
            theOptoContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, ValueAndTime<float>>(fDetectorContainer, registerName);
        MonitorDQMPlotRD53::fillOptoPlots(fDetectorData, registerName);
        return true;
    }

    return false;
}

void MonitorDQMPlotRD53::fillChipPlots(DetectorDataContainer& DataContainer, const std::string& registerName)
{
    if(fRegisterMonitorPlotMap.find(registerName) == fRegisterMonitorPlotMap.end())
    {
        std::string errorMessage = "No booked plots for IT register: ";
        LOG(ERROR) << BOLDRED << errorMessage << BOLDYELLOW << registerName << RESET;
        throw std::runtime_error(errorMessage + registerName);
    }

    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    TGraph* chipDQMPlot = fRegisterMonitorPlotMap[registerName]
                                              .getObject(cBoard->getId())
                                              ->getObject(cOpticalGroup->getId())
                                              ->getObject(cHybrid->getId())
                                              ->getObject(cChip->getId())
                                              ->getSummary<GraphContainer<TGraph>>()
                                              .fTheGraph;

                    if(cChip->hasSummary() == false) continue;
                    chipDQMPlot->SetPoint(chipDQMPlot->GetN(), this->getTimeStampForRoot(cChip->getSummary<ValueAndTime<float>>().fTime), cChip->getSummary<ValueAndTime<float>>().fValue);
                }
}

void MonitorDQMPlotRD53::fillOptoPlots(DetectorDataContainer& DataContainer, const std::string& registerName)
{
    if(fRegisterMonitorPlotMap.find(registerName) == fRegisterMonitorPlotMap.end())
    {
        std::string errorMessage = "No booked plots for IT register: ";
        LOG(ERROR) << BOLDRED << errorMessage << BOLDYELLOW << registerName << RESET;
        throw std::runtime_error(errorMessage + registerName);
    }

    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            TGraph* optoDQMPlot = fRegisterMonitorPlotMap[registerName].getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<GraphContainer<TGraph>>().fTheGraph;

            if(cOpticalGroup->hasSummary() == false) continue;
            optoDQMPlot->SetPoint(optoDQMPlot->GetN(), this->getTimeStampForRoot(cOpticalGroup->getSummary<ValueAndTime<float>>().fTime), cOpticalGroup->getSummary<ValueAndTime<float>>().fValue);
        }
}
