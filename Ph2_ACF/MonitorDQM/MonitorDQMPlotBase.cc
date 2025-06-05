#include "MonitorDQM/MonitorDQMPlotBase.h"
#include "RootUtils/GraphContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"

#include <TAxis.h>
#include <TDatime.h>
#include <TGraph.h>

uint32_t MonitorDQMPlotBase::getTimeStampForRoot(std::string rawTime)
{
    int yy, mm, dd, hh, mi, ss;
    if(sscanf(rawTime.c_str(), "%d-%d-%d %d:%d:%d", &yy, &mm, &dd, &hh, &mi, &ss) != 6) { std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] bad time format: " << rawTime << std::endl; }

    TDatime rootTime(rawTime.c_str());
    return rootTime.Convert();
}

void MonitorDQMPlotBase::bookImplementer(TFile*                   theOutputFile,
                                         const DetectorContainer& theDetectorStructure,
                                         DetectorDataContainer&   dataContainer,
                                         GraphContainer<TGraph>&  graphContainer,
                                         const std::string&       type,
                                         const char*              XTitle,
                                         const char*              YTitle)
{
    graphContainer.fTheGraph->GetXaxis()->SetTimeDisplay(1);
    graphContainer.fTheGraph->GetXaxis()->SetNdivisions(503);
    graphContainer.fTheGraph->GetXaxis()->SetTimeFormat(TIME_FORMAT);
    graphContainer.fTheGraph->GetXaxis()->SetTimeOffset(0);
    if(XTitle != nullptr) graphContainer.fTheGraph->GetXaxis()->SetTitle(XTitle);
    if(YTitle != nullptr)
    {
        graphContainer.setNameTitle("DQM_" + std::string{YTitle}, "DQM_" + std::string{YTitle});
        graphContainer.fTheGraph->GetYaxis()->SetTitle(YTitle);
    }
    graphContainer.fTheGraph->SetMarkerStyle(20);
    graphContainer.fTheGraph->SetMarkerSize(0.8);

    if(type == "chip")
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, dataContainer, graphContainer);
    else if(type == "opto")
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, dataContainer, graphContainer);
}