#include "MonitorUtils/SEHMonitor.h"
#include "HWDescription/Definition.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "HWInterface/D19clpGBTInterface.h"
#include "Utils/ContainerFactory.h"
#include "Utils/Utilities.h"
#include "Utils/ValueAndTime.h"
#include <boost/algorithm/string.hpp>

#ifdef __USE_ROOT__
#include "TFile.h"
#endif

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

SEHMonitor::SEHMonitor(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig) : DetectorMonitor(theSystemController, theDetectorMonitorConfig)
{
    // Add a new TCP Client to avoid conflicts in parallel process
    LOG(INFO) << BOLDYELLOW << "Trying to connect to the Power Supply Server..." << RESET;
    fPowerSupplyClient = new TCPClient("127.0.0.1", 7000);
    if(!fPowerSupplyClient->connect(1))
    {
        LOG(INFO) << BOLDYELLOW << "Cannot connect to the Power Supply Server, power supplies will need to be controlled manually" << RESET;
        delete fPowerSupplyClient;
        fPowerSupplyClient = nullptr;
    }
    else { LOG(INFO) << BOLDYELLOW << "Connected to the Power Supply Server!" << RESET; }
    // fPowerSupplyClient->setReceiveTimeout(1,0);
#ifdef __USE_ROOT__
    fMonitorPlotDQMSEH = new MonitorDQMPlotSEH();
    fMonitorPlotDQMSEH->book(fOutputFile, *fTheSystemController->fDetectorContainer, fDetectorMonitorConfig);
    fMonitorPlotDQM   = new MonitorDQMPlot2S();
    fMonitorDQMPlot2S = static_cast<MonitorDQMPlot2S*>(fMonitorPlotDQM);
    fMonitorDQMPlot2S->book(fOutputFile, *fTheSystemController->fDetectorContainer, fDetectorMonitorConfig);
#endif
}
// Maybe not ideal here (but needed to avoid memory leak)?? Could be moved to ~DetectorMonitor() if fPowerSupplyClient is also used for other devices?
SEHMonitor::~SEHMonitor()
{
    if(fPowerSupplyClient != nullptr)
    {
        delete fPowerSupplyClient;
        fPowerSupplyClient = nullptr;
    }
#ifdef __USE_ROOT__
    if(fMonitorPlotDQMSEH != nullptr)
    {
        delete fMonitorPlotDQMSEH;
        fMonitorPlotDQMSEH = nullptr;
    }
#endif
}

void SEHMonitor::runMonitor()
{
    for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("LpGBT"))
        if(registerName.second) runLpGBTRegisterMonitor(registerName.first);
    for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("PowerSupply"))
        if(registerName.second) runPowerSupplyMonitor(registerName.first);
    for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("TestCard"))
        if(registerName.second) runTestCardMonitor(registerName.first);
}

void SEHMonitor::runLpGBTRegisterMonitor(const std::string& registerName)
{
    DetectorDataContainer theLpGBTRegisterContainer;
    ContainerFactory::copyAndInitOpticalGroup<ValueAndTime<float>>(*fTheSystemController->fDetectorContainer, theLpGBTRegisterContainer);

    for(const auto& board: *fTheSystemController->fDetectorContainer)
    {
        if(board->getFirstObject()->flpGBT == nullptr)
        {
            for(const auto& opticalGroup: *board)
                theLpGBTRegisterContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<ValueAndTime<float>>() = ValueAndTime<float>(0, getTimeStampString());
            continue;
        }
        for(const auto& opticalGroup: *board)
        {
            uint16_t registerValue = static_cast<D19clpGBTInterface*>(fTheSystemController->flpGBTInterface)->ReadADC(opticalGroup->flpGBT, registerName);
            LOG(DEBUG) << BOLDMAGENTA << "LpGBT " << opticalGroup->getId() << " - " << registerName << " = " << registerValue << RESET;
            theLpGBTRegisterContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<ValueAndTime<float>>() = ValueAndTime<float>(registerValue, getTimeStampString());
        }
    }

#ifdef __USE_ROOT__
    fMonitorDQMPlot2S->fillLpGBTmonitorPlots(theLpGBTRegisterContainer, registerName);
#else
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("SEHMonitorLpGBTRegister");
        theContainerSerialization.streamByBoardContainer(fTheSystemController->fMonitorDQMStreamer, theLpGBTRegisterContainer, registerName);
    }
#endif
}

void SEHMonitor::runPowerSupplyMonitor(const std::string& registerName)
{
    std::vector<std::string> seglist;
    boost::split(seglist, registerName, boost::is_any_of("_"));
    float cValue;

    if((registerName.find("HV") != std::string::npos) & (registerName.find("Current") != std::string::npos))
    {
        std::string message = "GetCurrent,PowerSupplyId:" + seglist.at(0) + ",ChannelId:" + seglist.at(1) + "_" + seglist.at(2);
        LOG(INFO) << BOLDMAGENTA << message << RESET;
        std::string current = fPowerSupplyClient->sendAndReceivePacket(message);

        cValue = std::stof(current);
        LOG(INFO) << BOLDMAGENTA << registerName << " " << cValue << RESET;

        cValue *= 1e9;
    }
    else
    {
        std::string message = "GetVoltage,PowerSupplyId:" + seglist.at(0) + ",ChannelId:" + seglist.at(1) + "_" + seglist.at(2);
        LOG(INFO) << BOLDMAGENTA << message << RESET;
        std::string voltage = fPowerSupplyClient->sendAndReceivePacket(message);
        cValue              = std::stof(voltage);
        LOG(INFO) << BOLDMAGENTA << registerName << " " << cValue << RESET;
    }
    DetectorDataContainer thePowerSupplyContainer;
    ContainerFactory::copyAndInitDetector<ValueAndTime<float>>(*fTheSystemController->fDetectorContainer, thePowerSupplyContainer);
    thePowerSupplyContainer.getSummary<ValueAndTime<float>>() = ValueAndTime<float>(cValue, getTimeStampString());

#ifdef __USE_ROOT__
    fMonitorPlotDQMSEH->fillPowerSupplyPlots(thePowerSupplyContainer, registerName);
#else
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("SEHMonitorPowerSupply");
        theContainerSerialization.streamByBoardContainer(fTheSystemController->fMonitorDQMStreamer, thePowerSupplyContainer, registerName);
    }
#endif
}

void SEHMonitor::runTestCardMonitor(const std::string& registerName)
{
    LOG(INFO) << BOLDMAGENTA << "We pretend to be a measurement " << registerName << RESET;
    float cValue = 0;
#if defined(__TCUSB__) && defined(__USE_ROOT__)
    pTC_2SSEH->read_hvmon(pTC_2SSEH->HV_meas, cValue);
    LOG(INFO) << BOLDMAGENTA << cValue << " " << registerName << RESET;
#endif
    DetectorDataContainer theTestCardContainer;
    ContainerFactory::copyAndInitDetector<ValueAndTime<float>>(*fTheSystemController->fDetectorContainer, theTestCardContainer);
    theTestCardContainer.getSummary<ValueAndTime<float>>() = ValueAndTime<float>(cValue, getTimeStampString());

#ifdef __USE_ROOT__
    fMonitorPlotDQMSEH->fillTestCardPlots(theTestCardContainer, registerName);
#else
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("SEHMonitorTestCard");
        theContainerSerialization.streamByBoardContainer(fTheSystemController->fMonitorDQMStreamer, theTestCardContainer, registerName);
    }
#endif
}

void SEHMonitor::runInputCurrentMonitor(const std::string& registerName)
{
    LOG(INFO) << BOLDMAGENTA << "Running Input Current Monitor" << RESET;

    DetectorDataContainer theLpGBTRegisterContainer;
    ContainerFactory::copyAndInitOpticalGroup<ValueAndTime<float>>(*fTheSystemController->fDetectorContainer, theLpGBTRegisterContainer);

    for(const auto& board: *fTheSystemController->fDetectorContainer)
    {
        if(board->getFirstObject()->flpGBT == nullptr) continue;
        for(const auto& opticalGroup: *board)
        {
            uint16_t registerValue = (fTheSystemController->flpGBTInterface)->ReadADC(opticalGroup->flpGBT, "ADC1");
            LOG(INFO) << BOLDMAGENTA << "LpGBT " << opticalGroup->getId() << " - "
                      << "ADC1"
                      << " = " << registerValue << RESET;
            // theLpGBTRegisterContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<ValueAndTime<float>>() = ValueAndTime<float>(registerValue, getTimeStampString());
        }
    }
    LOG(INFO) << BOLDMAGENTA << "We pretend to be a measurement" << RESET;
}

std::string SEHMonitor::getVariableValue(const std::string& variable, const std::string& buffer)
{
    size_t begin = buffer.find(variable) + variable.size() + 1;
    size_t end   = buffer.find(',', begin);
    if(end == std::string::npos) end = buffer.size();
    return buffer.substr(begin, end - begin);
}
