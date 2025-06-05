#include "PhaseScan.h"

#include "../HWDescription/Cbc.h"
#include "../Utils/CBCChannelGroupHandler.h"
#include "../Utils/ContainerFactory.h"
#include "../Utils/GenericDataArray.h"
#include "../Utils/MPAChannelGroupHandler.h"
#include "../Utils/Occupancy.h"
#include "../Utils/SSAChannelGroupHandler.h"

PhaseScan::PhaseScan() : Tool() {}

PhaseScan::~PhaseScan() {}

void PhaseScan::Initialize()
{
    // check sparsification
    for(auto cBoard: *fDetectorContainer)
    {
        bool cSparsified = (fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable") == 1);
        cBoard->setSparsification(cSparsified);
    }

    ReadoutChip* cFirstReadoutChip = static_cast<ReadoutChip*>(fDetectorContainer->getFirstObject()->getFirstObject()->getFirstObject()->getFirstObject());
    bool         cWithCBC          = (cFirstReadoutChip->getFrontEndType() == FrontEndType::CBC3);
    bool         cWithPSv2         = (cFirstReadoutChip->getFrontEndType() == FrontEndType::SSA2 || cFirstReadoutChip->getFrontEndType() == FrontEndType::MPA2);

    if(cWithCBC)
    {
        CBCChannelGroupHandler theChannelGroupHandler;
        theChannelGroupHandler.setChannelGroupParameters(16, 1, 2); // 16*2*8
        setChannelGroupHandler(theChannelGroupHandler);
    }
    else if(cWithPSv2)
    {
        MPAChannelGroupHandler theChannelGroupHandlerMPA;
        theChannelGroupHandlerMPA.setChannelGroupParameters(1, NMPAROWS, NSSACHANNELS); // 16*2*8
        setChannelGroupHandler(theChannelGroupHandlerMPA, FrontEndType::MPA2);

        SSAChannelGroupHandler theChannelGroupHandlerSSA;
        theChannelGroupHandlerSSA.setChannelGroupParameters(1, 1, NSSACHANNELS); // 16*2*8
        setChannelGroupHandler(theChannelGroupHandlerSSA, FrontEndType::SSA2);
    }

    initializeRecycleBin();

    fPhaseStartLatency = findValueInSettings<double>("PhaseStartLatency", 1);
    fPhaseLatencyRange = findValueInSettings<double>("PhaseLatencyRange", 1);
    fStartPhase        = findValueInSettings<double>("StartPhase", 0);
    fPhaseRange        = findValueInSettings<double>("PhaseRange", 7);
    std::cout << "Going to read " << fNevents << " events" << std::endl;

#ifdef __USE_ROOT__
    fDQMHistogramPhaseScan.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    LOG(INFO) << "Histograms and Settings initialised.";
}

void PhaseScan::ScanPhase()
{
    uint32_t cDeltaLat = fPhaseStartLatency;
    do {
        uint32_t cPhaseLat = fStartPhase;
        do {
            for(auto cBoard: *fDetectorContainer)
            {
                for(auto cOpticalGroup: *cBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            if(cChip->getFrontEndType() == FrontEndType::SSA2)
                                fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cDeltaLat + 1);
                            else
                                fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cDeltaLat);
                            fReadoutChipInterface->WriteChipReg(cChip, "PhaseShift", cPhaseLat);
                        }
                    }
                }
                fBeBoardInterface->ChipReSync(cBoard);
                this->ReadNEvents(cBoard, fNevents);
                const std::vector<Event*>& cEvents      = this->GetEvents();
                size_t                     cTriggerMult = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
                uint32_t                   NPclus       = 0;
                uint32_t                   NSclus       = 0;

                for(size_t cTriggerId = 0; cTriggerId < cTriggerMult + 1; cTriggerId++)
                {
                    DetectorDataContainer cHitContainer;
                    ContainerFactory::copyAndInitChip<GenericDataArray<uint16_t, VECSIZE>>(*fDetectorContainer, cHitContainer);

                    // std::cout<<"cTriggerId "<<+cTriggerId<<std::endl;
                    auto cEventIter = cEvents.begin() + cTriggerId;
                    do {
                        uint8_t cTDCVal = (*cEventIter)->GetTDC();
                        for(auto cOpticalGroup: *cBoard)
                        {
                            for(auto cHybrid: *cOpticalGroup)
                            {
                                for(auto cChip: *cHybrid)
                                {
                                    auto cPclstrs = static_cast<D19cCic2Event*>((*cEventIter))->GetPixelClusters(cHybrid->getId(), cChip->getId());
                                    auto cSclstrs = static_cast<D19cCic2Event*>((*cEventIter))->GetStripClusters(cHybrid->getId(), cChip->getId());

                                    // cTotalHitsS0 += cPclstrs.size();
                                    // cTotalHitsS1 += cSclstrs.size();
                                    if(cChip->getFrontEndType() == FrontEndType::MPA2)
                                    {
                                        for(auto& cPclstr: cPclstrs)
                                        {
                                            for(uint8_t cId = 0; cId < (cPclstr.fWidth); cId++)
                                            {
                                                cHitContainer.getObject(cBoard->getId())
                                                    ->getObject(cOpticalGroup->getId())
                                                    ->getObject(cHybrid->getId())
                                                    ->getObject(cChip->getId())
                                                    ->getSummary<GenericDataArray<uint16_t, VECSIZE>>()
                                                    .at(cTDCVal) += 1;
                                                NPclus += 1;
                                            }
                                        }
                                    }
                                    if(cChip->getFrontEndType() == FrontEndType::SSA2)
                                    {
                                        for(auto& cSclstr: cSclstrs)
                                        {
                                            for(uint8_t cId = 0; cId < (cSclstr.fWidth); cId++)
                                            {
                                                cHitContainer.getObject(cBoard->getId())
                                                    ->getObject(cOpticalGroup->getId())
                                                    ->getObject(cHybrid->getId())
                                                    ->getObject(cChip->getId())
                                                    ->getSummary<GenericDataArray<uint16_t, VECSIZE>>()
                                                    .at(cTDCVal) += 1;
                                                NSclus += 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        cEventIter += (1 + cTriggerMult);
                    } while(cEventIter < cEvents.end());
#ifdef __USE_ROOT__
                    fDQMHistogramPhaseScan.fillPhasePlots(cDeltaLat, cPhaseLat, cHitContainer);
#endif
                    ;
                    if(NPclus + NSclus > 0)
                    {
                        LOG(INFO) << "Latency: " << cDeltaLat << " SamplingPhase: " << cPhaseLat << RESET;
                        LOG(INFO) << "Found NPclus: " << NPclus << " NSclus: " << NSclus << RESET;
                    }
                }
            }
            cPhaseLat += 1;
        } while(cPhaseLat < (fStartPhase + fPhaseRange));
        cDeltaLat += 1;

    } while(cDeltaLat < (fPhaseStartLatency + fPhaseLatencyRange));
}

void PhaseScan::writeObjects()
{
#ifdef __USE_ROOT__
    fDQMHistogramPhaseScan.process();
#endif
}

void PhaseScan::Running()
{
    LOG(INFO) << "Starting Phase Scan";
    Initialize();
    PhaseScan();
    LOG(INFO) << "Done with Phase Scan";
}

void PhaseScan::Stop()
{
    LOG(INFO) << "Stopping Phase Scan.";
    writeObjects();
    dumpConfigFiles();
    closeFileHandler();
    LOG(INFO) << "Phase Scan stopped.";
}

void PhaseScan::Pause() {}

void PhaseScan::Resume() {}
