/*!
        \file                MonitorDQMPlot2S.cc
        \brief               DQM class for DQM example -> use it as a templare
        \author              Fabio Ravera
        \date                25/7/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#include "MonitorDQM/MonitorDQMPlot2S.h"
#include "HWDescription/Definition.h"
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
MonitorDQMPlot2S::MonitorDQMPlot2S() {}

//========================================================================================================================
MonitorDQMPlot2S::~MonitorDQMPlot2S() {}

//========================================================================================================================
void MonitorDQMPlot2S::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig)
{
    MonitorDQMPlotOT::book(theOutputFile, theDetectorStructure, detectorMonitorConfig);

    for(const auto& registerName: detectorMonitorConfig.fMonitorElementList.at("CBC"))
        if(registerName.second) bookCBCPlots(theOutputFile, registerName.first);
}

//========================================================================================================================
void MonitorDQMPlot2S::bookCBCPlots(TFile* theOutputFile, std::string registerName)
{
    auto theTGraphHistogramContainer = producePlotTemplate(registerName, "CBC");
    RootContainerFactory::bookChipHistograms<GraphContainer<TGraph>>(theOutputFile, *fDetectorContainer, fCBCRegisterMonitorPlotMap[registerName], theTGraphHistogramContainer);
}

//========================================================================================================================
void MonitorDQMPlot2S::fillCBCRegisterPlots(DetectorDataContainer& theInputContainer, const std::string& registerName)
{
    fillReadoutChipPlots(theInputContainer, registerName, fCBCRegisterMonitorPlotMap, FrontEndType::CBC3);
}

//========================================================================================================================
void MonitorDQMPlot2S::process() { MonitorDQMPlotOT::process(); }

//========================================================================================================================
void MonitorDQMPlot2S::reset(void) { MonitorDQMPlotOT::reset(); }

//========================================================================================================================
bool MonitorDQMPlot2S::fill(std::string& inputStream)
{
    if(MonitorDQMPlotOT::fill(inputStream)) return true;

    ContainerSerialization theCBCRegisterSerialization("Monitor2SCBCRegister");

    if(theCBCRegisterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Monitor2S CBCRegister!!!!!\n";
        std::string           registerName;
        DetectorDataContainer fDetectorData =
            theCBCRegisterSerialization.deserializeBoardContainer<EmptyContainer, ValueAndTime<float>, EmptyContainer, EmptyContainer, EmptyContainer>(fDetectorContainer, registerName);
        fillCBCRegisterPlots(fDetectorData, registerName);
        return true;
    }
    return false;
}
