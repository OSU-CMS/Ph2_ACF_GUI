#include "MonitorUtils/Monitor2S.h"
#include "HWDescription/Definition.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Utilities.h"
#include "Utils/ValueAndTime.h"

#ifdef __USE_ROOT__
#include "MonitorDQM/MonitorDQMPlot2S.h"
#include "TFile.h"
#endif

using namespace Ph2_HwInterface;

Monitor2S::Monitor2S(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig) : OTMonitor(theSystemController, theDetectorMonitorConfig)
{
#ifdef __USE_ROOT__
    fMonitorPlotDQM = new MonitorDQMPlot2S();
    static_cast<MonitorDQMPlot2S*>(fMonitorPlotDQM)->book(fOutputFile, *fTheSystemController->fDetectorContainer, fDetectorMonitorConfig);
#endif
}

void Monitor2S::runMonitor()
{
    for(const auto& monitorValueName: fDetectorMonitorConfig.fMonitorElementList.at("CBC"))
        if(monitorValueName.second) runMonitorCBC(monitorValueName.first);
    OTMonitor::runMonitor();
}

void Monitor2S::runMonitorCBC(const std::string& monitorValueName)
{
    auto theCBCRegisterContainer = getReadoutChipMonitorValues(monitorValueName, FrontEndType::CBC3);

#ifdef __USE_ROOT__
    static_cast<MonitorDQMPlot2S*>(fMonitorPlotDQM)->fillCBCRegisterPlots(theCBCRegisterContainer, monitorValueName);
#else
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("Monitor2SCBCRegister");
        theContainerSerialization.streamByBoardContainer(fTheSystemController->fMonitorDQMStreamer, theCBCRegisterContainer, monitorValueName);
    }
#endif
}

void Monitor2S::readChipMonitorValue(const std::string& monitorValueName, Ph2_HwDescription::ReadoutChip* theChip, DetectorDataContainer& theDataContainer)
{
    uint16_t registerValue = fTheSystemController->fReadoutChipInterface->ReadChipReg(theChip, monitorValueName); // just to read something
    LOG(DEBUG) << BOLDMAGENTA << "board " << theChip->getBeBoardId() << "opticalGroup " << theChip->getOpticalGroupId() << "hybrid " << theChip->getHybridId() << " - chip " << theChip->getId() << " "
               << monitorValueName << " = " << registerValue << RESET;
    ValueAndTime<float> theRegisterAndTime(registerValue, getTimeStampString());
    theDataContainer.getChip(theChip->getBeBoardId(), theChip->getOpticalGroupId(), theChip->getHybridId(), theChip->getId())->getSummary<ValueAndTime<float>>() = theRegisterAndTime;
}
