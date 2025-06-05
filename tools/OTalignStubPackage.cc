#include "tools/OTalignStubPackage.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/DataContainer.h"
#include <algorithm>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTalignStubPackage::fCalibrationDescription = "Find stub package delay to properly decode stubs in the FC7";

OTalignStubPackage::OTalignStubPackage() : Tool() {}

OTalignStubPackage::~OTalignStubPackage() {}

void OTalignStubPackage::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeBoardRegister("fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link0_link9");
    fRegisterHelper->freeBoardRegister("fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link10_link11");
    fRegisterHelper->freeBoardRegister("fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link0_link9");
    fRegisterHelper->freeBoardRegister("fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link10_link11");
    fIsKickoff = findValueInSettings<double>("isKickoff", 0) > 0;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTalignStubPackage.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTalignStubPackage::ConfigureCalibration() {}

void OTalignStubPackage::Running()
{
    LOG(INFO) << "Starting OTalignStubPackage measurement.";
    Initialise();
    size_t numberOtIterations    = 0;
    size_t maxNumberOfIterations = 1;
    while(numberOtIterations < maxNumberOfIterations)
    {
        if(AlignStubPackage()) break;
        ++numberOtIterations;
        LOG(WARNING) << WARNING_FORMAT << "Not all stub packages are correctly aligned" << RESET;
    }
    if(numberOtIterations >= maxNumberOfIterations) { LOG(ERROR) << ERROR_FORMAT << "Failed to align all stub packages" << RESET; }
    LOG(INFO) << "Done with OTalignStubPackage.";
    Reset();
}

void OTalignStubPackage::Stop(void)
{
    LOG(INFO) << "Stopping OTalignStubPackage measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTalignStubPackage.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTalignStubPackage stopped.";
}

void OTalignStubPackage::Pause() {}

void OTalignStubPackage::Resume() {}

void OTalignStubPackage::Reset() { fRegisterHelper->restoreSnapshot(); }

bool OTalignStubPackage::AlignStubPackage()
{
    bool allHybridsAligned = true;

    uint16_t numberOfEvents                                   = 10;
    uint16_t triggerFrequency                                 = 400;   // kHz
    uint16_t clockFrequency                                   = 40000; // kHz
    uint16_t cMaxBxCounter                                    = 3564;
    uint16_t numberOfClockCyclesAfterInitialReset             = 100; // safety margin to avoid roll over
    uint16_t numberOfClockCyclesBetweenTwoConsecutiveTriggers = clockFrequency / triggerFrequency;

    if(numberOfClockCyclesBetweenTwoConsecutiveTriggers != float(clockFrequency / float(triggerFrequency)))
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Error: numberOfClockCyclesBetweenTwoConsecutiveTriggers must be an integer! Aborting..." << std::endl;
        abort();
    }
    if(numberOfClockCyclesAfterInitialReset + numberOfEvents * numberOfClockCyclesBetweenTwoConsecutiveTriggers >= cMaxBxCounter)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Error: BxId roll over not handled by the procedure! Aborting" << std::endl;
        abort();
    }

    uint16_t numberOfEventsToSkip = 1; // Ignore first events, bug with resync in FW?

    std::vector<uint16_t> emptyBunchCrossingId(numberOfEvents, 0xFFFF);
    DetectorDataContainer theBunchCrossingIdContainer;
    ContainerFactory::copyAndInitHybrid<std::vector<uint16_t>>(*fDetectorContainer, theBunchCrossingIdContainer, emptyBunchCrossingId);

    std::vector<bool>     emptyBestPackageDelay(8, false);
    DetectorDataContainer theBestPackageDelayContainer;
    ContainerFactory::copyAndInitHybrid<std::vector<bool>>(*fDetectorContainer, theBestPackageDelayContainer, emptyBestPackageDelay);

    std::vector<uint16_t> emptyBunchCrossingIdDifference(numberOfEvents - 1 - numberOfEventsToSkip, 0x7FFF);
    DetectorDataContainer theBunchCrossingIdDifferenceContainer;
    ContainerFactory::copyAndInitHybrid<std::vector<uint16_t>>(*fDetectorContainer, theBunchCrossingIdDifferenceContainer, emptyBunchCrossingIdDifference);

    for(auto theBoard: *fDetectorContainer)
    {
        // get interface
        auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
        // make sure you're only sending one trigger at a time here
        LOG(INFO) << GREEN << "Trying to align CIC stub decoder in the back-end" << RESET;

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                // disable all FEs
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
            }
        }

        std::vector<std::pair<std::string, uint32_t>> initialRegisterVector;
        initialRegisterVector.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 3});
        initialRegisterVector.push_back({"fc7_daq_cnfg.fast_command_block.user_trigger_frequency", triggerFrequency});
        initialRegisterVector.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        initialRegisterVector.push_back({"fc7_daq_cnfg.fast_command_block.misc.initial_fast_reset_enable", 1}); // Ensure a fast reset is sent before reading events to avoid roll over
        initialRegisterVector.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0});
        initialRegisterVector.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", 0});
        initialRegisterVector.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
        fBeBoardInterface->WriteBoardMultReg(theBoard, initialRegisterVector);

        for(uint8_t thePackageDelay = 0; thePackageDelay < 8; thePackageDelay++)
        {
            uint32_t packageDelayValue = 0;
            for(size_t link = 0; link < 10; ++link) { packageDelayValue = packageDelayValue | (thePackageDelay << (3 * link)); }

            std::vector<std::pair<std::string, uint32_t>> packageDelayRegisterVector;
            packageDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link0_link9", packageDelayValue});
            packageDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link10_link11", packageDelayValue & 0x3F});
            packageDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link0_link9", packageDelayValue});
            packageDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link10_link11", packageDelayValue & 0x3F});
            fBeBoardInterface->WriteBoardMultReg(theBoard, packageDelayRegisterVector);

            // reset stub readout to load new setting
            cInterface->Bx0Alignment();

            // read events
            ReadNEvents(theBoard, numberOfEvents);
            const std::vector<Event*>& theEventVector = this->GetEvents();

            // retrieve bunch crossing id for all events
            for(size_t eventNumber = 0; eventNumber < numberOfEvents; ++eventNumber)
            {
                auto theEvent = theEventVector.at(eventNumber);
                for(auto theOpticalGroup: *theBoard)
                {
                    for(auto theHybrid: *theOpticalGroup)
                    {
                        auto& eventBxIdVector           = theBunchCrossingIdContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<uint16_t>>();
                        eventBxIdVector.at(eventNumber) = theEvent->BxId(theHybrid->getId());
                        if(eventNumber > numberOfEventsToSkip)
                        {
                            auto& bxIdDifferenceVector =
                                theBunchCrossingIdDifferenceContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<uint16_t>>();
                            bxIdDifferenceVector.at(eventNumber - 1 - numberOfEventsToSkip) = eventBxIdVector.at(eventNumber) - eventBxIdVector.at(eventNumber - 1);
                        }
                    }
                }
            }

            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    auto theBunchCrossingIdDifference =
                        theBunchCrossingIdDifferenceContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<uint16_t>>();
                    // std::cout << "Hybrid id = " << theHybrid->getId() << std::endl;
                    // for(auto bxIdDifference: theBunchCrossingIdDifference) std::cout << bxIdDifference << " ";
                    // std::cout << std::endl;
                }
            }

            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    auto theBunchCrossingIdDifference =
                        theBunchCrossingIdDifferenceContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<uint16_t>>();
                    // remove duplicate
                    std::sort(theBunchCrossingIdDifference.begin(), theBunchCrossingIdDifference.end());
                    theBunchCrossingIdDifference.erase(unique(theBunchCrossingIdDifference.begin(), theBunchCrossingIdDifference.end()), theBunchCrossingIdDifference.end());
                    bool isSameAndCorrectBx = theBunchCrossingIdDifference.size() == 1 && theBunchCrossingIdDifference.at(0) == numberOfClockCyclesBetweenTwoConsecutiveTriggers;
                    theBestPackageDelayContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<bool>>().at(thePackageDelay) = isSameAndCorrectBx;
                }
            }
        }

        std::map<uint16_t, uint32_t> bestPackageDelayLink0Link9   = {{0, 0}, {1, 0}};
        std::map<uint16_t, uint32_t> bestPackageDelayLink10Link11 = {{0, 0}, {1, 0}};

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto theBestPackageDelayVector = theBestPackageDelayContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<bool>>();
                auto numberOfBestPackageDelays = std::count(theBestPackageDelayVector.begin(), theBestPackageDelayVector.end(), true);
                if(numberOfBestPackageDelays != 1)
                {
                    LOG(ERROR) << ERROR_FORMAT << "ERROR for Board " << +theBoard->getId() << " OpticalGroup " << +theOpticalGroup->getId() << " Hybrid " << +theHybrid->getId()
                               << ": number of best package delay = " << numberOfBestPackageDelays << ", expected to be 1" << RESET;
                    allHybridsAligned = false;
                    continue;
                }
                auto bestPackageDelay = std::find_if(theBestPackageDelayVector.begin(), theBestPackageDelayVector.end(), [](bool value) { return value; }) -
                                        theBestPackageDelayVector.begin(); // find intex of the best phase

                if(theOpticalGroup->getId() < 10)
                    bestPackageDelayLink0Link9[theHybrid->getId() % 2] |= (bestPackageDelay << (theOpticalGroup->getId() % 10) * 3);
                else
                    bestPackageDelayLink10Link11[theHybrid->getId() % 2] |= (bestPackageDelay << (theOpticalGroup->getId() % 10) * 3);
            }
        }

        std::vector<std::pair<std::string, uint32_t>> finalDelayRegisterVector;
        finalDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link0_link9", bestPackageDelayLink0Link9[0]});
        finalDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link10_link11", bestPackageDelayLink10Link11[0]});
        finalDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link0_link9", bestPackageDelayLink0Link9[1]});
        finalDelayRegisterVector.push_back({"fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link10_link11", bestPackageDelayLink10Link11[1]});
        fBeBoardInterface->WriteBoardMultReg(theBoard, finalDelayRegisterVector);
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTalignStubPackage.fillBestStubPackageDelay(theBestPackageDelayContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theBestStubPackageDelayContainerSerialization("OTalignStubPackageBestStubPackageDelay");
        theBestStubPackageDelayContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBestPackageDelayContainer);
    }
#endif

    return allHybridsAligned;
}
