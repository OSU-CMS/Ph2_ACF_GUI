#include "KIRA.h"

#include "HWDescription/BeBoardRegItem.h"
#include "Utils/ContainerFactory.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Occupancy.h"

KIRA::KIRA() : OTTool() { fTargetIntensity = 0; }
KIRA::~KIRA() {}

// State machine control functions
void KIRA::Running()
{
    auto cKiraPort = fSettingsMap.find("KIRA_Port");
    auto cKiraId   = fSettingsMap.find("KIRA_ID");

    if(cKiraPort != std::end(fSettingsMap) && cKiraId != std::end(fSettingsMap))
    {
        uint16_t    port = static_cast<uint16_t>(boost::any_cast<double>(cKiraPort->second));
        std::string id   = boost::any_cast<std::string>(cKiraId->second);
        LOG(INFO) << "KIRA Port: " << port << " KIRA ID: " << id << RESET;

        Initialise(port, id);
        determineLatency();
        performKIRATest();
    }
    else { LOG(INFO) << "KIRA settings not set, abort" << RESET; }
}

void KIRA::Initialise(int pKiraPort, std::string pKiraId)
{
    Prepare();
    SetName("KIRATest");

    fKiraClient = new TCPClient("127.0.0.1", pKiraPort);
    fKiraId     = pKiraId;
    LOG(INFO) << BOLDYELLOW << "Trying to connect to the KIRA Server..." << RESET;
    if(!fKiraClient->connect(1))
    {
        LOG(ERROR) << BOLDRED << "Cannot connect to the KIRA Server, KIRA will need to be controlled manually" << RESET;
        delete fKiraClient;
        fKiraClient = nullptr;
    }
    else { LOG(INFO) << BOLDYELLOW << "Connected to the KIRA Server!" << RESET; }

    // Switch all LEDs off
    for(int i = 0; i < 8; i++)
    {
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(i) + ",Light:off");
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(i) + ",Light:off");
    }
    LOG(INFO) << BOLDBLUE << "All KIRA LEDs switched off" << RESET;
    fKiraClient->sendAndReceivePacket("KIRATriggerFrequency,ArduinoId:" + fKiraId + ",TriggerFrequency:40000");
    fKiraClient->sendAndReceivePacket("KIRATrigger,ArduinoId:" + fKiraId + ",Trigger:on");
    fKiraClient->sendAndReceivePacket("KIRAPulseLength,ArduinoId:" + fKiraId + ",PulseLength:50");

    LOG(INFO) << BOLDBLUE << "KIRA trigger enabled and set to 40 kHz, 50ns pulse length" << RESET;

    fRecycleBin.setDetectorContainer(fDetectorContainer);

#ifdef __USE_ROOT__
    fDQMHistogrammer.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    PrepareForExternal();
}

void KIRA::PrepareForExternal()
{
    LOG(INFO) << BOLDBLUE << "Prepare for external triggers" << RESET;
    this->enableTestPulse(false);
    for(auto cBoard: *fDetectorContainer)
    {
        uint8_t                                       cTriggerSource = 5;
        std::vector<std::string>                      cFcmdRegs{"trigger_source", "triggers_to_accept"};
        std::vector<uint32_t>                         cFcmdRegVals{cTriggerSource, 0}; // fNevents};
        std::vector<uint32_t>                         cFcmdRegOrigVals(cFcmdRegs.size(), 0);
        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        cRegVec.clear();
        for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
        {
            std::string cRegName    = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
            cFcmdRegOrigVals[cIndx] = fBeBoardInterface->ReadBoardReg(cBoard, cRegName);
            cRegVec.push_back({cRegName, cFcmdRegVals[cIndx]});
        }
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0});
        cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        // enable DIO5
        LOG(INFO) << "\tTrigger source: 5" << RESET;
        LOG(INFO) << "\tEnable DIO5 Block" << RESET;
        LOG(INFO) << "\tDisable output on channel 2" << RESET;
        LOG(INFO) << "\tEnable termination on channel 2" << RESET;
        LOG(INFO) << "\tSet threshold on channel to 0" << RESET;

        cRegVec.push_back({"fc7_daq_cnfg.readout_block.global.data_handshake_enable", 0x0});
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.dio5_en", 0x1});
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.ch2.out_enable", 0});
        // SET TERMINATION TO ZERO, BEACUSE THE PULSE HEIGHT OF THE ARDUINO IS RATHER SMALL!!!
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.ch2.term_enable", 0});
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.ch2.threshold", 0});

        fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled", 0);

        // stop triggers
        fBeBoardInterface->Stop(cBoard);
        // send a ReSync
        fBeBoardInterface->ChipReSync(cBoard);
    }
}
void KIRA::determineLatency()
{
    auto cBoard = fDetectorContainer->getFirstObject();
    // initialize latency scan range
    uint16_t cLatencyStart     = findValueInSettings<double>("StartLatency", 85);
    uint16_t cLatencyRange     = findValueInSettings<double>("LatencyRange", 15);
    uint16_t cLatencyLED       = findValueInSettings<double>("KiraLatencyLed", 4);
    uint16_t cLatencyIntensity = findValueInSettings<double>("KiraLatencyIntensity", 30000);
    uint16_t cLatencySensor    = findValueInSettings<double>("KiraLatencySensor", 0);
    bool     botSensor         = (cLatencySensor == 0);
    uint16_t cHitMaximum       = 0;
    uint16_t cLatencySetting   = 0;

    LOG(INFO) << "Determine correct latency setting for KIRA operation using top LED " << +cLatencyLED << " with intensity " << cLatencyIntensity << RESET;
    if(botSensor)
    {
        fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLatencyLED) + ",Intensity:" + std::to_string(cLatencyIntensity));
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLatencyLED) + ",Light:on");
    }
    else
    {
        fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLatencyLED) + ",Intensity:" + std::to_string(cLatencyIntensity));
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLatencyLED) + ",Light:on");
    }

    for(uint16_t cLat = cLatencyStart; cLat < cLatencyStart + cLatencyRange; cLat++)
    {
        LOG(INFO) << BOLDBLUE << "Set Latency to " << cLat << RESET;
        setSameDacBeBoard(cBoard, "TriggerLatency", cLat);
        fBeBoardInterface->ChipReSync(cBoard);
        ReadNEvents(cBoard, fNevents);
        // ContinuousReadout();

        size_t                     cTriggerMult = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
        const std::vector<Event*>& cEvents      = this->GetEvents();
        // loop over triggers in the burst
        for(size_t cTriggerId = 0; cTriggerId < cTriggerMult + 1; cTriggerId++)
        {
            // prepare container to hold hit information per chip
            DetectorDataContainer cHitContainer;
            ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, cHitContainer);
            // zero hit container
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        cHitContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = 0;
                    } // chip
                } // hybrid
            } // optical group

            // start at the beginning + trigger id in burst
            auto cEventIter  = cEvents.begin() + cTriggerId;
            fNReadbackEvents = cEvents.size();
            do {
                if(cEventIter >= cEvents.end()) break;
                for(auto cOpticalGroup: *cBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;
                            // skip all chips that are not directly illuminated by the LED
                            if(cHybrid->getId() % 2 == 0 && cChip->getId() != 7 - cLatencyLED) continue;
                            if(cHybrid->getId() % 2 == 1 && cChip->getId() != cLatencyLED) continue;

                            auto cHits = (*cEventIter)->GetHits(cHybrid->getId(), cChip->getId());
                            if(cHits.size() != 0) LOG(DEBUG) << BOLDBLUE << "Event#" << (*cEventIter)->GetEventCount() << "Chip#" << +cChip->getId() % 8 << " " << +cHits.size() << " hits." << RESET;
                            for(auto cHit: cHits)
                            {
                                // monitor only specified sensor channels
                                if(cHit.second % 2 == cLatencySensor)
                                {
                                    cHitContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() += 1;
                                }
                            }
                        } // chip vector
                    } // hybrid vector
                } // optical group vector
                cEventIter += (1 + cTriggerMult);
            } while(cEventIter < cEvents.end());
#ifdef __USE_ROOT__
            fDQMHistogrammer.fillLatencyPlots(cLat + cTriggerId, cTriggerId, cHitContainer, fNReadbackEvents);
#endif

            // Find earliest maximum latency bin (= largest latency setting with maximum entries)
            for(auto cOpticalGroup: *cBoard)
            {
                double cTmpHitsSum = 0;
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;
                        if(cHybrid->getId() % 2 == 0 && cChip->getId() != 7 - cLatencyLED) continue;
                        if(cHybrid->getId() % 2 == 1 && cChip->getId() != cLatencyLED) continue;
                        double cTmpHits = cHitContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() /
                                          (1.0 * fNReadbackEvents);
                        cTmpHitsSum += cTmpHits;

                        if(cTmpHitsSum >= cHitMaximum)
                        {
                            cHitMaximum     = cTmpHitsSum;
                            cLatencySetting = cLat + cTriggerId;
                        }
                    }
                }
            }
        }
    }

    LOG(INFO) << BOLDRED << "Found optimal latency to be " << cLatencySetting << RESET;
    LOG(INFO) << BOLDRED << "Set Latency to " << cLatencySetting << RESET;
    setSameDacBeBoard(cBoard, "TriggerLatency", cLatencySetting);
    fBeBoardInterface->ChipReSync(cBoard);
    // Switch LED off again
    if(botSensor)
    {
        fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLatencyLED) + ",Intensity:0");
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLatencyLED) + ",Value:off");
    }
    else
    {
        fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLatencyLED) + ",Intensity:0");
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLatencyLED) + ",Value:off");
    }
}

void KIRA::performKIRATest()
{
    LOG(INFO) << BOLDRED << "Starting KIRA Test" << RESET;
    auto     cBoard     = fDetectorContainer->getFirstObject();
    uint16_t cIntensity = findValueInSettings<double>("KiraIntensity", 30000);
    if(fTargetIntensity != 0) cIntensity = fTargetIntensity;

    for(uint16_t cLED = 0; cLED < 8; cLED++)
    {
        LOG(INFO) << BOLDYELLOW << "Test with bottom LED " << cLED << " - Intensity set to " << cIntensity << RESET;
        // Start with bottom LED
        fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Intensity:" + std::to_string(cIntensity));
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Light:on");

        // ContinuousReadout();
        ReadNEvents(cBoard, fNevents);
        const std::vector<Event*>& cEvents       = this->GetEvents();
        DetectorDataContainer      cHitContainer = analyseEvents(cBoard, cEvents, 0, cLED);
#ifdef __USE_ROOT__
        fDQMHistogrammer.fillBottomSensorPlots(cHitContainer, fNReadbackEvents, cLED);
#endif
        // Switch off LED
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Light:off");

        LOG(INFO) << BOLDYELLOW << "Test with top LED " << cLED << " - Intensity set to " << cIntensity << RESET;
        // Start top LED
        fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Intensity:" + std::to_string(cIntensity));
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Light:on");

        fBeBoardInterface->ChipReSync(cBoard);
        // ContinuousReadout();
        ReadNEvents(cBoard, fNevents);
        const std::vector<Event*>& cEvents2       = this->GetEvents();
        DetectorDataContainer      cHitContainer2 = analyseEvents(cBoard, cEvents2, 1, cLED);
#ifdef __USE_ROOT__
        fDQMHistogrammer.fillTopSensorPlots(cHitContainer2, fNReadbackEvents, cLED);
#endif
        // Switch off LED
        fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Light:off");
    }
}

void KIRA::calibrateIntensity()
{
    LOG(INFO) << BOLDRED << "Performing LED intensity calibration " << RESET;
    auto     cBoard          = fDetectorContainer->getFirstObject();
    uint32_t cIntensityStart = findValueInSettings<double>("KiraCalibrationIntensityStart", 29000);
    uint32_t cIntensityStop  = findValueInSettings<double>("KiraCalibrationIntensityStop", 31000);
    uint16_t cIntensityStep  = findValueInSettings<double>("KiraCalibrationIntensityStep", 1000);
    fTargetIntensity         = 0;
    uint16_t cNrChipCovered  = 0;
    for(uint16_t cLED = 0; cLED < 8; cLED++)
    {
        LOG(INFO) << BOLDYELLOW << "Calibrating Bottom LED " << cLED << RESET;
        for(uint32_t cIntensity = cIntensityStart; cIntensity <= cIntensityStop; cIntensity += cIntensityStep)
        {
            LOG(INFO) << BOLDYELLOW << "Setting LED Intensity to " << cIntensity << RESET;
            fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Intensity:" + std::to_string(cIntensity));
            fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Light:on");
            // ContinuousReadout();
            ReadNEvents(cBoard, fNevents);
            const std::vector<Event*>& cEvents             = this->GetEvents();
            DetectorDataContainer      cHitContainerBottom = analyseEvents(cBoard, cEvents, 0, cLED, false);
            // Check if chip on both hybrids is already fully illuminated
            bool cChipCovered = check_channel_illumination(cBoard, cHitContainerBottom, cLED);
#ifdef __USE_ROOT__
            fDQMHistogrammer.fillSensorPlotsCalibration(cHitContainerBottom, fNReadbackEvents, cLED, cIntensity, 0);
#endif

            fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Intensity:0");
            fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Bot" + std::to_string(cLED) + ",Light:off");
            if(cChipCovered)
            {
                if(cIntensity > fTargetIntensity) fTargetIntensity = cIntensity;
                cNrChipCovered += 1;
                break;
            }
        }
        LOG(INFO) << BOLDYELLOW << "Calibrating Top LED " << cLED << RESET;
        for(uint32_t cIntensity = cIntensityStart; cIntensity <= cIntensityStop; cIntensity += cIntensityStep)
        {
            LOG(INFO) << BOLDYELLOW << "Setting LED Intensity to " << cIntensity << RESET;
            fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Intensity:0");
            fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Light:on");
            // ContinuousReadout();
            ReadNEvents(cBoard, fNevents);
            const std::vector<Event*>& cEvents          = this->GetEvents();
            DetectorDataContainer      cHitContainerTop = analyseEvents(cBoard, cEvents, 1, cLED, false);
            // Check if chip on both hybrids is already fully illuminated
            bool cChipCovered = check_channel_illumination(cBoard, cHitContainerTop, cLED);
#ifdef __USE_ROOT__
            fDQMHistogrammer.fillSensorPlotsCalibration(cHitContainerTop, fNReadbackEvents, cLED, cIntensity, 1);
#endif

            fKiraClient->sendAndReceivePacket("KIRAIntensity,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Intensity:0");
            fKiraClient->sendAndReceivePacket("KIRALight,ArduinoId:" + fKiraId + ",LED:Top" + std::to_string(cLED) + ",Light:off");
            if(cChipCovered)
            {
                if(cIntensity > fTargetIntensity) fTargetIntensity = cIntensity;
                cNrChipCovered += 1;
                break;
            }
        }
    }
    if(fTargetIntensity != 0 && cNrChipCovered == 16) { LOG(INFO) << BOLDBLUE << "Found new target intensity to illuminate all chips: " << fTargetIntensity << RESET; }
    else { fTargetIntensity = 0; }
}

bool KIRA::check_channel_illumination(BeBoard* pBoard, DetectorDataContainer& pContainer, uint16_t pLED)
{
    bool cChipCovered = false;
    for(auto cOpticalGroup: *pBoard)
    {
        uint32_t cSum = 0;
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;
                // skip all chips that are not directly illuminated by the LED
                if(cHybrid->getId() % 2 == 0 && cChip->getId() != 7 - pLED) continue;
                if(cHybrid->getId() % 2 == 1 && cChip->getId() != pLED) continue;
                // count occupancy in all channels
                auto cContainer =
                    pContainer.getObject(pBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<GenericDataArray<float, VECSIZE>>();
                for(uint16_t cIndx = 0; cIndx < 127; cIndx++) { cSum += cContainer.at(cIndx); }
            }
        }
        if(cSum / (254. * fNReadbackEvents) == 1) { cChipCovered = true; }
    }
    return cChipCovered;
}

DetectorDataContainer KIRA::analyseEvents(BeBoard* pBoard, const std::vector<Event*>& pEvents, uint16_t pSensor, uint16_t pLED, bool pSkipChips)
{
    // prepare container to hold hit information per chip
    DetectorDataContainer cHitContainer;
    ContainerFactory::copyAndInitChip<GenericDataArray<float, VECSIZE>>(*fDetectorContainer, cHitContainer);

    // start at the beginning + trigger id in burst
    auto cEventIter  = pEvents.begin();
    fNReadbackEvents = pEvents.size();
    do {
        if(cEventIter >= pEvents.end()) break;
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;
                    // skip all chips that are not directly illuminated by the LED
                    if(pSkipChips)
                    {
                        if(cHybrid->getId() % 2 == 0 && cChip->getId() != 7 - pLED) continue;
                        if(cHybrid->getId() % 2 == 1 && cChip->getId() != pLED) continue;
                    }

                    auto cHits = (*cEventIter)->GetHits(cHybrid->getId(), cChip->getId());
                    if(cHits.size() != 0) LOG(DEBUG) << BOLDBLUE << "Event#" << (*cEventIter)->GetEventCount() << "Chip#" << +cChip->getId() % 8 << " " << +cHits.size() << " hits." << RESET;
                    for(auto cHit: cHits)
                    {
                        LOG(DEBUG) << "Hit: " << cHit.second << RESET;
                        // Fill hits in Data Container for bottom sensor
                        if(cHit.second % 2 == pSensor)
                        {
                            cHitContainer.getObject(pBoard->getId())
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<GenericDataArray<float, VECSIZE>>()
                                .at(int(cHit.second / 2)) += 1;
                        }
                    }
                } // chip vector
            } // hybrid vector
        } // optical group vector
        cEventIter += 1;
    } while(cEventIter < pEvents.end());
    return cHitContainer;
}

void KIRA::Stop() {}

void KIRA::Pause() {}

void KIRA::Resume() {}
