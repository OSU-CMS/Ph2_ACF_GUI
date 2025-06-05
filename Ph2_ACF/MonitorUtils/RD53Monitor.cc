/*!
  \file                  RD53Monitor.cc
  \brief                 Implementaion of monitoring process
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "MonitorUtils/RD53Monitor.h"

RD53Monitor::RD53Monitor(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig)
    : DetectorMonitor(theSystemController, theDetectorMonitorConfig)
{
#ifdef __USE_ROOT__
    fMonitorPlotDQM = new MonitorDQMPlotRD53();
    fMonitorDQM     = static_cast<MonitorDQMPlotRD53*>(fMonitorPlotDQM);
    fMonitorDQM->book(fOutputFile, *fTheSystemController->fDetectorContainer, fDetectorMonitorConfig);
#endif
}

void RD53Monitor::runMonitor()
{
    if(fDetectorMonitorConfig.getNumberOfMonitoredRegisters() == 0) return;

    for(const auto cBoard: *fTheSystemController->fDetectorContainer)
    {
        std::vector<std::string> listOfRegisters;
        for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("RD53"))
            if(registerName.second) listOfRegisters.push_back(registerName.first);
        fTheSystemController->ReadSystemMonitor(cBoard, listOfRegisters, fDetectorMonitorConfig.fSilentRunning);

        for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("RD53"))
            if(registerName.second) runRD53RegisterMonitor(registerName.first);

        for(const auto& registerName: fDetectorMonitorConfig.fMonitorElementList.at("LpGBT"))
            if(registerName.second) runLpGBTRegisterMonitor(registerName.first);
    }
}

void RD53Monitor::runRD53RegisterMonitor(const std::string& registerName)
{
    DetectorDataContainer theRegisterContainer;
    ContainerFactory::copyAndInitChip<ValueAndTime<float>>(*fTheSystemController->fDetectorContainer, theRegisterContainer);

    for(const auto cBoard: *fTheSystemController->fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float registerValue;
                    if(fDetectorMonitorConfig.fSilentRunning == false)
                        LOG(INFO) << GREEN << "Reading monitored data for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                                  << cHybrid->getId() << "/" << +cChip->getId() << RESET << GREEN << "]" << RESET;
                    auto* readoutChipInterface = fTheSystemController->fReadoutChipInterface;

                    bool tmp;
                    if(static_cast<Ph2_HwInterface::RD53Interface*>(readoutChipInterface)->getADCobservable(registerName, tmp, fDetectorMonitorConfig.fSilentRunning) != -1)
                        // #######################
                        // # Monitor environment #
                        // #######################
                        registerValue =
                            fTheSystemController->fBeBoardInterface->ReadChipMonitor(fTheSystemController->fReadoutChipInterface, cChip, registerName, fDetectorMonitorConfig.fSilentRunning);
                    else
                    {
                        // #####################
                        // # Monitor registers #
                        // #####################
                        try
                        {
                            registerValue = readoutChipInterface->ReadChipReg(cChip, registerName);
                            if(fDetectorMonitorConfig.fSilentRunning == false)
                                LOG(INFO) << BOLDBLUE << "\t--> " << BOLDYELLOW << registerName << BOLDBLUE << " = 0x" << BOLDYELLOW << std::setprecision(0) << std::hex << registerValue << std::dec
                                          << RESET;
                        }
                        catch(const std::exception& e)
                        {
                            LOG(ERROR) << BOLDRED << e.what() << RESET;
                            registerValue = -1;
                        }
                    }

                    theRegisterContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<ValueAndTime<float>>() =
                        ValueAndTime<float>(registerValue, getTimeStampString());
                }

#ifdef __USE_ROOT__
    fMonitorDQM->fillChipPlots(theRegisterContainer, registerName);
#endif

    RD53Monitor::sendData(theRegisterContainer, registerName, "chip");
}

void RD53Monitor::runLpGBTRegisterMonitor(const std::string& registerName)
{
    DetectorDataContainer theRegisterContainer;
    ContainerFactory::copyAndInitOpticalGroup<ValueAndTime<float>>(*fTheSystemController->fDetectorContainer, theRegisterContainer);

    for(const auto cBoard: *fTheSystemController->fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            if(cOpticalGroup->flpGBT == nullptr)
            {
                LOG(INFO) << BOLDRED << "[RD53Monitor::runLpGBTRegisterMonitor] No LpGBT chip found for [board/opticalGroup = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId()
                          << BOLDRED << "]" << RESET;
                continue;
            }

            float registerValue;
            if(fDetectorMonitorConfig.fSilentRunning == false)
                LOG(INFO) << GREEN << "Reading monitored data for [board/opticalGroup = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << RESET << GREEN << "]" << RESET;
            auto* lpGBTInterface = fTheSystemController->flpGBTInterface;

            if(lpGBTInterface->fADCInputMap.find(registerName) != lpGBTInterface->fADCInputMap.end())
                // #######################
                // # Monitor environment #
                // #######################
                registerValue = lpGBTInterface->ReadChipMonitor(cOpticalGroup, registerName, fDetectorMonitorConfig.fSilentRunning);
            else
            {
                // #####################
                // # Monitor registers #
                // #####################
                try
                {
                    registerValue = lpGBTInterface->ReadChipReg(cOpticalGroup->flpGBT, registerName);
                    if(fDetectorMonitorConfig.fSilentRunning == false)
                        LOG(INFO) << BOLDBLUE << "\t--> " << BOLDYELLOW << registerName << BOLDBLUE << " = 0x" << BOLDYELLOW << std::setprecision(0) << std::hex << registerValue << std::dec << RESET;
                }
                catch(const std::exception& e)
                {
                    LOG(ERROR) << BOLDRED << e.what() << RESET;
                    registerValue = -1;
                }
            }

            theRegisterContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<ValueAndTime<float>>() = ValueAndTime<float>(registerValue, getTimeStampString());
        }

#ifdef __USE_ROOT__
    fMonitorDQM->fillOptoPlots(theRegisterContainer, registerName);
#endif

    RD53Monitor::sendData(theRegisterContainer, registerName, "opto");
}

void RD53Monitor::sendData(DetectorDataContainer& DataContainer, const std::string& registerName, const std::string& type)
{
    if(fTheSystemController->fMonitorDQMStreamerEnabled)
    {
        if(type == "chip")
        {
            ContainerSerialization theContainerSerialization("ITMonitorChipRegister");
            theContainerSerialization.streamByChipContainer(fTheSystemController->fMonitorDQMStreamer, DataContainer, registerName);
        }
        else if(type == "opto")
        {
            ContainerSerialization theContainerSerialization("ITMonitorOptoRegister");
            theContainerSerialization.streamByOpticalGroupContainer(fTheSystemController->fMonitorDQMStreamer, DataContainer, registerName);
        }
    }
}
