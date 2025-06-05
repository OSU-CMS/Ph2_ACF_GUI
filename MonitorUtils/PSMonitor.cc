#include "MonitorUtils/PSMonitor.h"
#include "HWDescription/Definition.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "HWInterface/D19clpGBTInterface.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Utilities.h"
#include "Utils/ValueAndTime.h"

#ifdef __USE_ROOT__
#include "MonitorDQM/MonitorDQMPlotPS.h"
#include "TFile.h"
#endif

using namespace Ph2_HwInterface;

PSMonitor::PSMonitor(Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig) : OTMonitor(theSystemController, theDetectorMonitorConfig)
{
#ifdef __USE_ROOT__
    fMonitorPlotDQM = new MonitorDQMPlotPS();
    static_cast<MonitorDQMPlotPS*>(fMonitorPlotDQM)->book(fOutputFile, *fTheSystemController->fDetectorContainer, fDetectorMonitorConfig);
#endif
}

void PSMonitor::runMonitor()
{
    for(const auto& monitorValueName: fDetectorMonitorConfig.fMonitorElementList.at("SSA2"))
        if(monitorValueName.second) runMonitorSSA(monitorValueName.first);
    for(const auto& monitorValueName: fDetectorMonitorConfig.fMonitorElementList.at("MPA2"))
        if(monitorValueName.second) runMonitorMPA(monitorValueName.first);
    OTMonitor::runMonitor();
}

void PSMonitor::runMonitorSSA(const std::string& monitorValueName)
{
    auto theSSA2RegisterContainer = getReadoutChipMonitorValues(monitorValueName, FrontEndType::SSA2);

#ifdef __USE_ROOT__
    static_cast<MonitorDQMPlotPS*>(fMonitorPlotDQM)->fillSSA2RegisterPlots(theSSA2RegisterContainer, monitorValueName);
#else
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("PSMonitorSSA2Register");
        theContainerSerialization.streamByBoardContainer(fTheSystemController->fMonitorDQMStreamer, theSSA2RegisterContainer, monitorValueName);
    }
#endif
}

void PSMonitor::runMonitorMPA(const std::string& monitorValueName)
{
    auto theMPA2RegisterContainer = getReadoutChipMonitorValues(monitorValueName, FrontEndType::MPA2);

#ifdef __USE_ROOT__
    static_cast<MonitorDQMPlotPS*>(fMonitorPlotDQM)->fillMPA2RegisterPlots(theMPA2RegisterContainer, monitorValueName);
#else
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("PSMonitorMPA2Register");
        theContainerSerialization.streamByBoardContainer(fTheSystemController->fMonitorDQMStreamer, theMPA2RegisterContainer, monitorValueName);
    }
#endif
}

void PSMonitor::readChipMonitorValue(const std::string& monitorValueName, Ph2_HwDescription::ReadoutChip* theChip, DetectorDataContainer& theDataContainer)
{
    auto  thePSInterface = static_cast<PSInterface*>(fTheSystemController->fReadoutChipInterface);
    float monitorValue   = 0;
    if(monitorValueName == "temp") { monitorValue = thePSInterface->measureTemperature(theChip); }
    else { monitorValue = thePSInterface->readADCVoltage(theChip, monitorValueName); }

    ValueAndTime<float> theRegisterAndTime(monitorValue, getTimeStampString());
    theDataContainer.getChip(theChip->getBeBoardId(), theChip->getOpticalGroupId(), theChip->getHybridId(), theChip->getId())->getSummary<ValueAndTime<float>>() = theRegisterAndTime;
}
