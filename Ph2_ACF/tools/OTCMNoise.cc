#include "OTCMNoise.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cTriggerInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include <math.h>

std::string OTCMNoise::fCalibrationDescription = "Measure common noise in 2S modules";
// PUBLIC METHODS
OTCMNoise::OTCMNoise() : Tool() {}

OTCMNoise::~OTCMNoise() {}

void OTCMNoise::Initialize()
{
    fRegisterHelper->takeSnapshot();

    parseSettings();

#ifdef __USE_ROOT__
    fDQMHistogramOTCMNoise.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    LOG(INFO) << "Histograms and Settings initialised.";
}

void OTCMNoise::SetThresholds(int manualVcth, float nSigma)
{
    // Set Vcth to pedestal, or overload with manual setting
    ThresholdVisitor cVisitor(fReadoutChipInterface, 0);

    LOG(INFO) << "OT_MODULE_TEST:: Setting threshold on each chip" << RESET;
    for(auto pBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                if(manualVcth != 0)
                {
                    LOG(INFO) << BOLDGREEN << "Setting Manual Vcth to " << manualVcth << RESET;
                    cVisitor.setThreshold(manualVcth);
                    static_cast<OuterTrackerHybrid*>(cHybrid)->accept(cVisitor);
                }
                else
                {
                    LOG(INFO) << BOLDCYAN << "Running with threshold at the pedestal + " << nSigma << " sigma." << RESET;
                    for(auto theChip: *cHybrid) { fReadoutChipInterface->WriteChipReg(theChip, "Threshold", round(theChip->getAveragePedestal() - nSigma * theChip->getAverageNoise())); };
                }

                for(auto cChip: *cHybrid)
                {
                    LOG(INFO) << BOLDGREEN << "Disabling stub reconstruction" << RESET;
                    static_cast<CbcInterface*>(fReadoutChipInterface)->enableHipSuppression(cChip, false, true, 0);

                    // if (cChip->getId()!=4){
                    //     LOG(INFO)<<BOLDRED<<"Setting threshold to 0 on CBC# "<<cChip->getId()<<RESET;
                    //     fReadoutChipInterface->WriteChipReg(cChip, "Threshold", 0);
                    // }
                }
            }
        }
    }
}

void OTCMNoise::TakeData(float fThreshold)
{
    if(fManualVcth != 0)
        LOG(INFO) << BOLDGREEN << "Taking data with Manual Vcth to " << fManualVcth << RESET;
    else
        LOG(INFO) << BOLDGREEN << "Taking data with threshold at the pedestal + " << fThreshold << " sigma." << RESET;

    ThresholdVisitor cVisitor(fReadoutChipInterface);
    this->accept(cVisitor);
    fVcth = cVisitor.getThreshold();
    LOG(INFO) << "Checking threshold on latest CBC that was touched...: " << fVcth;
    // Data is split between odd and even strips...
    DetectorDataContainer theChipHitContainer;
    DetectorDataContainer theHybridHitContainer;
    DetectorDataContainer theModuleHitContainer;

    DetectorDataContainer the2DHitContainer;
    DetectorDataContainer the2DChipHitContainer;

    // channel, chip, hybrid, optical group, board, detector
    // can have 0 or 255 hits, need NCHANNELS+1 (inclusive)
    ContainerFactory::copyAndInitStructure<EmptyContainer, GenericDataArray<uint32_t, 3 * (NCHANNELS + 1)>, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(*fDetectorContainer,
                                                                                                                                                                            theChipHitContainer);

    ContainerFactory::copyAndInitStructure<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT + 1)>, EmptyContainer, EmptyContainer, EmptyContainer>(
        *fDetectorContainer, theHybridHitContainer);

    ContainerFactory::copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT * 2 + 1)>, EmptyContainer, EmptyContainer>(
        *fDetectorContainer, theModuleHitContainer);

    // 2D arrays for module-level and hybrid-level correlation
    if(f2DHistograms)
        ContainerFactory::
            copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>, EmptyContainer, EmptyContainer>(
                *fDetectorContainer, the2DHitContainer);

    // 2D arrays for chip-level correlation
    if(f2DHistogramsLight)
        ContainerFactory::copyAndInitStructure<EmptyContainer, GenericDataArray<uint32_t, NCHANNELS, NCHANNELS>, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(*fDetectorContainer,
                                                                                                                                                                                 the2DChipHitContainer);

    // Creating the correlation plots... Maybe a lot of RAM being used?
    DetectorDataContainer the2DSensorModuleCorrelationContainer;
    DetectorDataContainer the2DSensorHybridCorrelationContainer;
    DetectorDataContainer the2DSensorChipCorrelationContainer;
    DetectorDataContainer the2DHybridCorrelationContainer;
    DetectorDataContainer the2DChipCorrelationContainer;

    ContainerFactory::copyAndInitOpticalGroup<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1, NCHANNELS * NCHIPS_OT + 1>>(*fDetectorContainer, the2DHybridCorrelationContainer);

    ContainerFactory::copyAndInitChip<GenericDataArray<uint32_t, NCHANNELS + 1, NCHANNELS * NCHIPS_OT + 1>>(*fDetectorContainer, the2DChipCorrelationContainer);

    ContainerFactory::copyAndInitStructure<EmptyContainer,
                                           EmptyContainer,
                                           EmptyContainer,
                                           GenericDataArray<uint32_t, (NCHANNELS * NCHIPS_OT * 2) / 2 + 1, (NCHANNELS * NCHIPS_OT * 2) / 2 + 1>,
                                           EmptyContainer,
                                           EmptyContainer>(*fDetectorContainer, the2DSensorModuleCorrelationContainer);

    ContainerFactory::copyAndInitStructure<EmptyContainer,
                                           EmptyContainer,
                                           GenericDataArray<uint32_t, (NCHANNELS * NCHIPS_OT / 2 + 1), (NCHANNELS * NCHIPS_OT / 2 + 1)>,
                                           EmptyContainer,
                                           EmptyContainer,
                                           EmptyContainer>(*fDetectorContainer, the2DSensorHybridCorrelationContainer);

    ContainerFactory::copyAndInitStructure<EmptyContainer, GenericDataArray<uint32_t, (NCHANNELS / 2 + 1), (NCHANNELS / 2 + 1)>, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(
        *fDetectorContainer, the2DSensorChipCorrelationContainer);
    for(auto cBoard: theChipHitContainer)
    {
        // BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
        BeBoard* theBoard = static_cast<BeBoard*>(fDetectorContainer->getObject(cBoard->getId()));

        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getTriggerInterface()->Start(true);
        uint32_t cN = fNevents;
        while(cN != 0)
        {
            uint32_t cNEventToRead = cN;
            if(cNEventToRead > 1000) cNEventToRead = 1000;
            cN -= cNEventToRead;
            ReadNEvents(theBoard, cNEventToRead);
            const std::vector<Event*>& events = GetEvents();
            setNReadbackEvents(events.size());
            LOG(INFO) << "Reading out " << events.size() << "events, " << cN << " events remaining.";

            for(auto cOpticalGroup: *cBoard)
            {
                for(auto& cEvent: events)
                {
                    if(cN > fNevents) continue;

                    uint32_t cModuleHits     = 0;
                    uint32_t cModuleHitsEven = 0;
                    uint32_t cModuleHitsOdd  = 0;

                    std::vector<uint32_t>             hit_channels;
                    std::map<int, std::map<int, int>> cChipCorrelationMap;
                    std::map<int, int>                cHybridCorrelationMap;
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        uint32_t cHybridHits     = 0;
                        uint32_t cHybridHitsEven = 0;
                        uint32_t cHybridHitsOdd  = 0;
                        for(auto cChip: *cHybrid)
                        {
                            uint32_t chipOffset_module = (cHybrid->getId() * NCHANNELS * NCHIPS_OT) + (cChip->getId() * NCHANNELS);
                            auto     hit_vec           = cEvent->GetHits(cHybrid->getId(), cChip->getId());
                            uint32_t cEventHitsEven    = 0;
                            uint32_t cEventHitsOdd     = 0;
                            for(auto hit: hit_vec)
                            {
                                if(hit.second % 2)
                                    cEventHitsEven++;
                                else
                                    cEventHitsOdd++;
                            }
                            uint32_t cEventHits                                   = cEventHitsEven + cEventHitsOdd;
                            cChipCorrelationMap[cHybrid->getId()][cChip->getId()] = cEventHits;

                            auto theChipHitContainerValues = &(theChipHitContainer.getObject(cBoard->getId())
                                                                   ->getObject(cOpticalGroup->getId())
                                                                   ->getObject(cHybrid->getId())
                                                                   ->getObject(cChip->getId())
                                                                   ->getSummary<GenericDataArray<uint32_t, 3 * (NCHANNELS + 1)>>());

                            (*theChipHitContainerValues).at(cEventHitsEven)++;
                            (*theChipHitContainerValues).at((NCHANNELS + 1) + cEventHitsOdd)++;
                            (*theChipHitContainerValues).at(2 * (NCHANNELS + 1) + cEventHits)++;

                            cHybridHits += cEventHits;
                            cHybridHitsEven += cEventHitsEven;
                            cHybridHitsOdd += cEventHitsOdd;
                            cHybridCorrelationMap[cHybrid->getId()] = cHybridHits;

                            the2DSensorChipCorrelationContainer.getObject(cBoard->getId())
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<GenericDataArray<uint32_t, (NCHANNELS / 2 + 1), (NCHANNELS / 2 + 1)>>()
                                .at(cEventHitsEven)
                                .at(cEventHitsOdd) += 1;

                            // for 2d correlation, save channels with hits per chip
                            if(f2DHistograms)
                            {
                                for(auto hit: hit_vec) { hit_channels.push_back(hit.second + chipOffset_module); }
                            }

                            // for 2d correlation only at the individual chip level, we can fill directly the Container
                            if(f2DHistogramsLight)
                            {
                                for(auto hit: hit_vec)
                                {
                                    for(auto hit2: hit_vec)
                                    {
                                        the2DChipHitContainer.getChip(cBoard->getId(), cOpticalGroup->getId(), cHybrid->getId(), cChip->getId())
                                            ->getSummary<GenericDataArray<uint32_t, NCHANNELS, NCHANNELS>>()
                                            .at(hit.second)
                                            .at(hit2.second) += 1;
                                    }
                                }
                            }
                        }
                        // save per hybrid
                        cModuleHits += cHybridHits;
                        cModuleHitsEven += cHybridHitsEven;
                        cModuleHitsOdd += cHybridHitsOdd;
                        // Re-looping on chips...
                        for(auto cChip: *cHybrid)
                        {
                            the2DChipCorrelationContainer.getChip(cBoard->getId(), cOpticalGroup->getId(), cHybrid->getId(), cChip->getId())
                                ->getSummary<GenericDataArray<uint32_t, NCHANNELS + 1, NCHANNELS * NCHIPS_OT + 1>>()
                                .at(cChipCorrelationMap.at(cHybrid->getId()).at(cChip->getId()))
                                .at(cHybridHits) += 1;
                        }

                        the2DSensorHybridCorrelationContainer.getObject(cBoard->getId())
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getSummary<GenericDataArray<uint32_t, (NCHANNELS * NCHIPS_OT / 2 + 1), (NCHANNELS * NCHIPS_OT / 2 + 1)>>()
                            .at(cHybridHitsEven)
                            .at(cHybridHitsOdd) += 1;

                        auto theHybridContainerValues = &(theHybridHitContainer.getObject(cBoard->getId())
                                                              ->getObject(cOpticalGroup->getId())
                                                              ->getObject(cHybrid->getId())
                                                              ->getSummary<GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT + 1)>>());
                        (*theHybridContainerValues).at(cHybridHitsEven)++;
                        (*theHybridContainerValues).at((NCHANNELS * NCHIPS_OT + 1) + cHybridHitsOdd)++;
                        (*theHybridContainerValues).at(2 * (NCHANNELS * NCHIPS_OT + 1) + cHybridHits)++;
                    }

                    auto theModuleContainerValues =
                        &(theModuleHitContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT * 2 + 1)>>());

                    (*theModuleContainerValues).at(cModuleHitsEven)++;
                    (*theModuleContainerValues).at((NCHANNELS * NCHIPS_OT * 2 + 1) + cModuleHitsOdd)++;
                    (*theModuleContainerValues).at(2 * (NCHANNELS * NCHIPS_OT * 2 + 1) + cModuleHits)++;

                    the2DSensorModuleCorrelationContainer.getObject(cBoard->getId())
                        ->getObject(cOpticalGroup->getId())
                        ->getSummary<GenericDataArray<uint32_t, ((NCHANNELS * NCHIPS_OT * 2) / 2 + 1), ((NCHANNELS * NCHIPS_OT * 2) / 2 + 1)>>()
                        .at(cModuleHitsEven)
                        .at(cModuleHitsOdd) += 1;

                    the2DHybridCorrelationContainer.getObject(cBoard->getId())
                        ->getObject(cOpticalGroup->getId())
                        ->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1, NCHANNELS * NCHIPS_OT + 1>>()
                        .at(cHybridCorrelationMap.begin()->second)
                        .at(cHybridCorrelationMap.rbegin()->second) += 1;

                    if(f2DHistograms)
                    {
                        // per module correlation also tells us per hybrid correlation
                        for(size_t iCh1 = 0; iCh1 < hit_channels.size(); iCh1++)
                        {
                            for(size_t iCh2 = 0; iCh2 < hit_channels.size(); iCh2++)
                            {
                                the2DHitContainer.getObject(cBoard->getId())
                                    ->getObject(cOpticalGroup->getId())
                                    ->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>()
                                    .at(hit_channels.at(iCh1))
                                    .at(hit_channels.at(iCh2)) += 1;
                            }
                        }
                    }

                } // end events loop

            } // end module loop
        } // end acquisition loop
    }
#ifdef __USE_ROOT__
    fDQMHistogramOTCMNoise.fillChipHitPlots(theChipHitContainer, false, fThreshold);
    fDQMHistogramOTCMNoise.fillHybridHitPlots(theHybridHitContainer, fThreshold);
    fDQMHistogramOTCMNoise.fillModuleHitPlots(theModuleHitContainer, fThreshold);

    fDQMHistogramOTCMNoise.fillHybridCorrelationPlots(the2DHybridCorrelationContainer, fThreshold);
    fDQMHistogramOTCMNoise.fillChipCorrelationPlots(the2DChipCorrelationContainer, fThreshold);
    fDQMHistogramOTCMNoise.fillSensorChipCorrelationPlots(the2DSensorChipCorrelationContainer, fThreshold);
    fDQMHistogramOTCMNoise.fillSensorHybridCorrelationPlots(the2DSensorHybridCorrelationContainer, fThreshold);
    fDQMHistogramOTCMNoise.fillSensorModuleCorrelationPlots(the2DSensorModuleCorrelationContainer, fThreshold);
    if(f2DHistograms) fDQMHistogramOTCMNoise.fill2DHitPlots(the2DHitContainer, fThreshold);
    if(f2DHistogramsLight) fDQMHistogramOTCMNoise.fill2DHitLightPlots(the2DChipHitContainer, fThreshold);

#else
    if(fDQMStreamerEnabled)
    {
        std::map<std::string, DetectorDataContainer*> cStreamableMap;
        cStreamableMap["OTCMNoiseChipHitStream"]                   = &theChipHitContainer;
        cStreamableMap["OTCMNoiseHybridHitStream"]                 = &theHybridHitContainer;
        cStreamableMap["OTCMNoiseModuleHitStream"]                 = &theModuleHitContainer;
        cStreamableMap["OTCMNoise2DHybridCorrelationStream"]       = &the2DHybridCorrelationContainer;
        cStreamableMap["OTCMNoise2DChipCorrelationStream"]         = &the2DChipCorrelationContainer;
        cStreamableMap["OTCMNoise2DSensorModuleCorrelationStream"] = &the2DSensorModuleCorrelationContainer;
        cStreamableMap["OTCMNoise2DSensorHybridCorrelationStream"] = &the2DSensorHybridCorrelationContainer;
        cStreamableMap["OTCMNoise2DSensorChipCorrelationStream"]   = &the2DSensorChipCorrelationContainer;

        for(auto cStreamable: cStreamableMap)
        {
            // LOG(INFO) << "Streaming " << cStreamable.first << RESET;
            ContainerSerialization theHitSerializationSum(cStreamable.first);
            theHitSerializationSum.streamByOpticalGroupContainer(fDQMStreamer, *(cStreamable.second), fThreshold);
        }

        if(f2DHistograms)
        {
            // LOG(INFO) << "Streaming OTCMNoise2DHitStream" << RESET;
            ContainerSerialization the2DHitSerialization("OTCMNoise2DHitStream");
            the2DHitSerialization.streamByOpticalGroupContainer(fDQMStreamer, the2DHitContainer, fThreshold);
        }

        if(f2DHistogramsLight)
        {
            // LOG(INFO) << "Streaming OTCMNoise2DHitLightStream" << RESET;
            ContainerSerialization the2DLightHitSerialization("OTCMNoise2DHitLightStream");
            the2DLightHitSerialization.streamByChipContainer(fDQMStreamer, the2DChipHitContainer, fThreshold);
        }
    }
#endif
}

void OTCMNoise::parseSettings()
{
    // now read the settings from the map
    fNevents           = findValueInSettings<double>("CMNoise_Nevents", 100);
    f2DHistograms      = findValueInSettings<double>("CMNoise_2DHistograms", 0);
    f2DHistogramsLight = findValueInSettings<double>("CMNoise_2DHistogramsLight", 0);
    fManualVcth        = findValueInSettings<double>("CMNoise_manualVcth", 0);
    fListOfThresholds  = convertStringToFloatList(findValueInSettings<std::string>("CMNoise_nSigmas", "0"));

    LOG(INFO) << "Parsed the following settings:";
    LOG(INFO) << "	Running " << fNevents;
    LOG(INFO) << "	2D Histograms? " << f2DHistograms;
    LOG(INFO) << "	2D Histograms Light? " << f2DHistogramsLight;
    LOG(INFO) << "	Manual Vcth " << fManualVcth;
    for(auto& cThreshold: fListOfThresholds) { LOG(INFO) << "	Threshold (in Sigmas) " << cThreshold; }
}

void OTCMNoise::writeObjects() {}

void OTCMNoise::ConfigureCalibration() {}

void OTCMNoise::Running()
{
    if(fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS)
    {
        LOG(ERROR) << ERROR_FORMAT << " Running a PS calibration on a PS module! " << RESET;
        return;
    }
    LOG(INFO) << "Starting CM noise measurement";
    Initialize();
    // Set the Vcth manually
    if(fManualVcth != 0)
    {
        SetThresholds(fManualVcth, 0);
        TakeData(fManualVcth);
    }
    else
    {
        // Loop over the thresholds
        for(auto& cThreshold: fListOfThresholds)
        {
            SetThresholds(0, cThreshold);
            TakeData(cThreshold);
        }
    }

    Reset();
    LOG(INFO) << "Done with CM noise";
}

void OTCMNoise::Stop()
{
    LOG(INFO) << "Stopping CM noise measurement";
    writeObjects();
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "CM Noise measurement stopped.";
}

void OTCMNoise::Pause() {}

void OTCMNoise::Resume() {}

void OTCMNoise::Reset() { fRegisterHelper->restoreSnapshot(); }
