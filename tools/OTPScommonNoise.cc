#include "tools/OTPScommonNoise.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/SSAChannelGroupHandler.h"
#include "tools/OTMeasureOccupancy.h"
#include <boost/math/distributions/normal.hpp>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTPScommonNoise::fCalibrationDescription = "Measure common noise in PS modules";

OTPScommonNoise::OTPScommonNoise() : Tool() {}

OTPScommonNoise::~OTPScommonNoise() {}

void OTPScommonNoise::Initialise(void)
{
    fNumberOfEvents = findValueInSettings<double>("OTPScommonNoise_NumberOfEvents", 100);
    fListOfSigma    = convertStringToFloatList(findValueInSettings<std::string>("OTPScommonNoise_ListOfSigma", "0, 3"));
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTPScommonNoise.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTPScommonNoise::ConfigureCalibration() {}

void OTPScommonNoise::SetThresholds(float numberOfSigma)
{
    float theStripSigma;
    float thePixelSigma;
    if(numberOfSigma == 0)
    {
        // For PS modules the CIC is in sparsified mode. Therefore we cannot have more 128 clusters per hybrid on the strips and pixels sensor.
        // Therefore we must set a threshold that allows around 64 clusters per pixels and strips.
        uint16_t theMaximumChannelNumber  = MAXCICCLUSTERS / 2;
        float    theStripAllowedOccupancy = float(theMaximumChannelNumber) / (NSSACHANNELS * NCHIPS_OT);
        float    thePixelAllowedOccupancy = float(theMaximumChannelNumber) / (NSSACHANNELS * NMPAROWS * NCHIPS_OT);

        LOG(INFO) << BOLDYELLOW << "theStripAllowedOccupancy: " << theStripAllowedOccupancy << " thePixelAllowedOccupancy: " << thePixelAllowedOccupancy << RESET;

        // Now we calculate how many sigmas away from the pedestal we should be to have that occupancy
        boost::math::normal gaus(0, 1); // we consider a standard gaussian
        theStripSigma = quantile(complement(gaus, theStripAllowedOccupancy));
        thePixelSigma = quantile(complement(gaus, thePixelAllowedOccupancy));
    }
    else
    {
        theStripSigma = numberOfSigma;
        thePixelSigma = numberOfSigma;
    }

    LOG(INFO) << BOLDYELLOW << "theStripSigma: " << theStripSigma << " thePixelSigma: " << thePixelSigma << RESET;

    // now we set the thresold  to pedestal + noise*sigma
    for(auto pBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    float theSigma = (cChip->getFrontEndType() == FrontEndType::SSA2) ? theStripSigma : thePixelSigma;
                    LOG(INFO) << BOLDYELLOW << " chip " << +cChip->getId() << " cChip->getAveragePedestal(): " << cChip->getAveragePedestal() << " cChip->getAverageNoise: " << cChip->getAverageNoise()
                              << RESET;
                    float theThreshold = cChip->getAveragePedestal() + cChip->getAverageNoise() * theSigma;
                    LOG(INFO) << BOLDYELLOW << " chip " << +cChip->getId() << " theThreshold: " << theThreshold << " rounded " << std::round(theThreshold) << RESET;
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", std::round(theThreshold));
                }
            }
        }
    }
}
void OTPScommonNoise::TakeData(float numberOfSigma)
{
    for(auto theBoard: *fDetectorContainer) { fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", 1); }

    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";

    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";

    // DetectorDataContainer the2DHitContainer;
    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);
    setSameDac("ReadoutMode", 0x0); // normal readout mode
    setSameDac("control_2", 0x0F);  // maximize cluster cut

    DetectorDataContainer theStripHitContainer;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>(*fDetectorContainer, theStripHitContainer);
    DetectorDataContainer theStripHybridHitContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>(*fDetectorContainer, theStripHybridHitContainer);
    DetectorDataContainer theStripModuleHitContainer;
    ContainerFactory::copyAndInitOpticalGroup<GenericDataArray<uint32_t, MAXCICCHANNELS * 2 + 1>>(*fDetectorContainer, theStripModuleHitContainer);
    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);

    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);
    setSameDac("Control_1", 0x0);         // set Readout mode to normal
    setSameDac("PixelControl_ALL", 0x1D); // disable Hip cut, cluster cut to the maximum, mode select level

    DetectorDataContainer thePixelHitContainer;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>(*fDetectorContainer, thePixelHitContainer);
    DetectorDataContainer thePixelHybridHitContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>(*fDetectorContainer, thePixelHybridHitContainer);
    DetectorDataContainer thePixelModuleHitContainer;
    ContainerFactory::copyAndInitOpticalGroup<GenericDataArray<uint32_t, MAXCICCHANNELS * 2 + 1>>(*fDetectorContainer, thePixelModuleHitContainer);

    // Correlation between SSA and MPA pairs, the container is created only for the MPAs and saved per hybrid
    DetectorDataContainer the2DSSAMPACorrelationContainer;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint32_t, MAXCICCHANNELS + 1, MAXCICCHANNELS + 1>>(*fDetectorContainer, the2DSSAMPACorrelationContainer);
    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);

    // Correlation between strip and pixels on 1 hybrid
    DetectorDataContainer the2DStripPixelHybridCorrelationContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint32_t, MAXCICCHANNELS + 1, MAXCICCHANNELS + 1>>(*fDetectorContainer, the2DStripPixelHybridCorrelationContainer);

    DetectorDataContainer the2DStripPixelModuleCorrelationContainer;
    ContainerFactory::copyAndInitOpticalGroup<GenericDataArray<uint32_t, MAXCICCHANNELS * 2 + 1, MAXCICCHANNELS * 2 + 1>>(*fDetectorContainer, the2DStripPixelModuleCorrelationContainer);

    // Prepare SSA and MPA for measurement
    OTMeasureOccupancy measureOccupancy;
    measureOccupancy.Inherit(this);
    measureOccupancy.fSSAtestPulseValue = 0;
    measureOccupancy.fMPAtestPulseValue = 0;
    measureOccupancy.prepareOccupancyMeasurementPS();

    for(auto theBoard: *fDetectorContainer)
    {
        uint32_t theEventCounter = fNumberOfEvents;
        while(theEventCounter != 0)
        {
            uint32_t cNEventToRead = theEventCounter;
            theEventCounter -= cNEventToRead;
            ReadNEvents(theBoard, cNEventToRead);
            const std::vector<Event*>& events = GetEvents();
            setNReadbackEvents(events.size());
            LOG(INFO) << BOLDYELLOW << "Reading out " << events.size() << " events, " << theEventCounter << " events remaining." << RESET;

            for(auto cOpticalGroup: *theBoard)
            {
                for(auto& cEvent: events)
                {
                    LOG(DEBUG) << BOLDYELLOW << " Event number " << cEvent->GetEventCount() << RESET;
                    if(theEventCounter > fNumberOfEvents) continue;

                    uint32_t cStripModuleHits = 0;
                    uint32_t cPixelModuleHits = 0;

                    for(auto cHybrid: *cOpticalGroup)
                    {
                        uint32_t cStripHybridHits = 0;
                        uint32_t cPixelHybridHits = 0;

                        for(auto cChip: *cHybrid)
                        {
                            // uint32_t chipOffset_module = (cHybrid->getId() * HYBRID_CHANNELS_OT) + (cChip->getId() * NCHANNELS);
                            auto hit_vec = cEvent->GetHits(cHybrid->getId(), cChip->getId());

                            uint32_t cEventHits = hit_vec.size();                            //
                            LOG(DEBUG) << BOLDBLUE << " cEventHits " << cEventHits << RESET; //                              = cEventHitsEven + cEventHitsOdd;
                            // cChipCorrelationMap[cHybrid->getId()][cChip->getId()] = cEventHits;

                            if(cChip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                theStripHitContainer.getObject(theBoard->getId())
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>()
                                    .at(cEventHits) += 1;

                                cStripHybridHits += cEventHits;
                            }
                            if(cChip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                thePixelHitContainer.getObject(theBoard->getId())
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>()
                                    .at(cEventHits) += 1;

                                cPixelHybridHits += cEventHits;

                                auto     strip_hit       = cEvent->GetHits(cHybrid->getId(), cChip->getId() % 8);
                                uint32_t cStripEventHits = strip_hit.size();
                                the2DSSAMPACorrelationContainer.getObject(theBoard->getId())
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS + 1, MAXCICCHANNELS + 1>>()
                                    .at(cStripEventHits)
                                    .at(cEventHits) += 1;
                            }

                            LOG(DEBUG) << BOLDBLUE << "cStripHybridHits: " << cStripHybridHits << " cPixelHybridHits: " << cPixelHybridHits << RESET;

                        } // chip loop

                        theStripHybridHitContainer.getObject(theBoard->getId())
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>()
                            .at(cStripHybridHits) += 1;

                        thePixelHybridHitContainer.getObject(theBoard->getId())
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS + 1>>()
                            .at(cPixelHybridHits) += 1;

                        // save per hybrid
                        cStripModuleHits += cStripHybridHits;
                        cPixelModuleHits += cPixelHybridHits;

                        the2DStripPixelHybridCorrelationContainer.getObject(theBoard->getId())
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS + 1, MAXCICCHANNELS + 1>>()
                            .at(cStripHybridHits)
                            .at(cPixelHybridHits) += 1;
                    }

                    theStripModuleHitContainer.getObject(theBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS * 2 + 1>>().at(cStripModuleHits) +=
                        1;

                    thePixelModuleHitContainer.getObject(theBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS * 2 + 1>>().at(cPixelModuleHits) +=
                        1;

                    the2DStripPixelModuleCorrelationContainer.getObject(theBoard->getId())
                        ->getObject(cOpticalGroup->getId())
                        ->getSummary<GenericDataArray<uint32_t, MAXCICCHANNELS * 2 + 1, MAXCICCHANNELS * 2 + 1>>()
                        .at(cStripModuleHits)
                        .at(cPixelModuleHits) += 1;

                } // end events loop
            } // end module loop
        } // end acquisition loop
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTPScommonNoise.fillChipHitPlots(theStripHitContainer, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillChipHitPlots(thePixelHitContainer, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillHybridHitPlots(theStripHybridHitContainer, true, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillHybridHitPlots(thePixelHybridHitContainer, false, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillModuleHitPlots(theStripModuleHitContainer, true, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillModuleHitPlots(thePixelModuleHitContainer, false, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillSSAtoMPACorrelationPlots(the2DSSAMPACorrelationContainer, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillStripPixelHybridCorrelationPlots(the2DStripPixelHybridCorrelationContainer, numberOfSigma);
    fDQMHistogramOTPScommonNoise.fillStripPixelModuleCorrelationPlots(the2DStripPixelModuleCorrelationContainer, numberOfSigma);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theStripChipHitContainerSerialization("OTPScommonNoiseStripChipHit");
        theStripChipHitContainerSerialization.streamByChipContainer(fDQMStreamer, theStripHitContainer, numberOfSigma);
        ContainerSerialization thePixelChipHitContainerSerialization("OTPScommonNoisePixelChipHit");
        thePixelChipHitContainerSerialization.streamByChipContainer(fDQMStreamer, thePixelHitContainer, numberOfSigma);

        bool                   isSSA = true;
        ContainerSerialization theStripHybridHitContainerSerialization("OTPScommonNoiseStripHybridHit");
        theStripHybridHitContainerSerialization.streamByHybridContainer(fDQMStreamer, theStripHybridHitContainer, isSSA, numberOfSigma);
        ContainerSerialization theStripModuleHitContainerSerialization("OTPScommonNoiseStripModuleHit");
        theStripModuleHitContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theStripModuleHitContainer, isSSA, numberOfSigma);

        isSSA = false;
        ContainerSerialization thePixelHybridHitContainerSerialization("OTPScommonNoisePixelHybridHit");
        thePixelHybridHitContainerSerialization.streamByHybridContainer(fDQMStreamer, thePixelHybridHitContainer, isSSA, numberOfSigma);
        ContainerSerialization thePixelModuleHitContainerSerialization("OTPScommonNoisePixelModuleHit");
        thePixelModuleHitContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, thePixelModuleHitContainer, isSSA, numberOfSigma);

        ContainerSerialization theSSAMPACorrelationContainerSerialization("OTPScommonNoiseSSAMPACorrelation");
        theSSAMPACorrelationContainerSerialization.streamByHybridContainer(fDQMStreamer, the2DSSAMPACorrelationContainer, numberOfSigma);
        ContainerSerialization theStripPixelHybridContainerSerialization("OTPScommonNoiseStripPixelHybridCorrelation");
        theStripPixelHybridContainerSerialization.streamByHybridContainer(fDQMStreamer, the2DStripPixelHybridCorrelationContainer, numberOfSigma);
        ContainerSerialization theStripPixelModuleContainerSerialization("OTPScommonNoiseStripPixelModuleCorrelation");
        theStripPixelModuleContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, the2DStripPixelModuleCorrelationContainer, numberOfSigma);
    }
#endif
}

void OTPScommonNoise::Running()
{
    if(fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S)
    {
        LOG(ERROR) << ERROR_FORMAT << " Running a PS calibration on a 2S module! " << RESET;
        return;
    }
    LOG(INFO) << BOLDMAGENTA << "Starting OTPScommonNoise measurement." << RESET;
    Initialise();
    for(auto numberOfSigma: fListOfSigma)
    {
        SetThresholds(numberOfSigma);
        TakeData(numberOfSigma);
    }
    LOG(INFO) << BOLDMAGENTA << "Done with OTPScommonNoise." << RESET;
    Reset();
}

void OTPScommonNoise::Stop(void)
{
    LOG(INFO) << "Stopping OTPScommonNoise measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTPScommonNoise.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTPScommonNoise stopped.";
}

void OTPScommonNoise::Pause() {}

void OTPScommonNoise::Resume() {}

void OTPScommonNoise::Reset() { fRegisterHelper->restoreSnapshot(); }
