#include "tools/PSPixelAlive.h"
#include "HWDescription/BeBoardRegItem.h"
#include "HWDescription/Cbc.h"
#include "HWInterface/D19cFWInterface.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/Occupancy.h"
#include "Utils/SSAChannelGroupHandler.h"
#include "Utils/ThresholdAndNoise.h"
#include "boost/format.hpp"
#include <math.h>

PSPixelAlive::PSPixelAlive() : Tool() {}

PSPixelAlive::~PSPixelAlive() { clearDataMembers(); }

void PSPixelAlive::cleanContainerVector() {}

void PSPixelAlive::clearDataMembers()
{
    return;
    if(fDisableStubLogic)
    {
        delete fStubLogicValue;
        delete fHIPCountValue;
    }
    cleanContainerVector();
}

// Does Setup before any data taking
void PSPixelAlive::Initialise()
{
    for(auto cBoard: *fDetectorContainer)
    {
        BeBoardRegMap cRegMap      = cBoard->getBeBoardRegMap();
        uint32_t      cTriggerFreq = cRegMap["fc7_daq_cnfg.fast_command_block.user_trigger_frequency"].fValue;

        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        cRegVec.clear();
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.user_trigger_frequency", cTriggerFreq});
        cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
        //        LOG(INFO) << BOLDYELLOW << "Noise measured on BeBoard#" << +cBoard->getId() << " with a trigger rate of " << cTriggerFreq << "kHz." << RESET;
    }
    fDisableStubLogic = false;

    this->enableTestPulse(true); // For Testing Purposes

    fWithSSA = false;
    fWithMPA = false;
    std::vector<FrontEndType> cAllFrontEndTypes;
    for(auto cBoard: *fDetectorContainer)
    {
        auto cFrontEndTypes = cBoard->connectedFrontEndTypes();
        fWithSSA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::SSA2) != cFrontEndTypes.end();
        fWithMPA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::MPA2) != cFrontEndTypes.end();
        for(auto cFrontEndType: cFrontEndTypes)
        {
            if(std::find(cAllFrontEndTypes.begin(), cAllFrontEndTypes.end(), cFrontEndType) == cAllFrontEndTypes.end()) cAllFrontEndTypes.push_back(cFrontEndType);
        }
    }
    if(fWithSSA && !fWithMPA) LOG(INFO) << BOLDBLUE << "PixelAlive with SSAs" << RESET;
    if(fWithMPA && !fWithSSA) LOG(INFO) << BOLDBLUE << "PixelAlive with MPAs" << RESET;
    if(fWithSSA && fWithMPA) LOG(INFO) << BOLDBLUE << "PixelAlive with SSAs+MPAs" << RESET;

    for(auto cFrontEndType: cAllFrontEndTypes)
    {
        if(cFrontEndType == FrontEndType::SSA2)
        {
            SSAChannelGroupHandler theChannelGroupHandler;
            theChannelGroupHandler.setChannelGroupParameters(1, 1, NSSACHANNELS); // 16*2*8
            setChannelGroupHandler(theChannelGroupHandler, cFrontEndType);
        }
        else if(cFrontEndType == FrontEndType::MPA2)
        {
            MPAChannelGroupHandler theChannelGroupHandler;
            theChannelGroupHandler.setChannelGroupParameters(1, NMPAROWS, NSSACHANNELS); // 16*2*8
            setChannelGroupHandler(theChannelGroupHandler, cFrontEndType);
        }
    }

    initializeRecycleBin();

    //    fAllChan = pAllChan;

    // Reads in options from xml file, has corresponding stuff in header
    fSkipMaskedChannels = findValueInSettings<double>("SkipMaskedChannels", 0); // after comma is default

    fPSPixelAliveMask   = findValueInSettings<double>("PixelAliveMask", 1); // Mask Pixels?
    fEventsPerPoint     = findValueInSettings<double>("PixelAliveNevents", 1000);
    fNEventsPerBurst    = (fEventsPerPoint >= fMaxNevents) ? fMaxNevents : fEventsPerPoint;
    fPulseAmplitude_MPA = findValueInSettings<double>("PixelAlivePulseAmplitude_MPA", 145);
    fPulseAmplitude_SSA = findValueInSettings<double>("PixelAlivePulseAmplitude_SSA", 132);
    fPulseThreshold_MPA = findValueInSettings<double>("PixelAliveThreshold_MPA", 225);
    fPulseThreshold_SSA = findValueInSettings<double>("PixelAliveThreshold_SSA", 111);
    LOG(INFO) << "Parsed settings:";
    LOG(INFO) << " Nevents = " << fEventsPerPoint;
    //    std::cout << __PRETTY_FUNCTION__ << " MPA PulseAmplitude " << +fPulseAmplitude_MPA << std::endl;
    std::cout << " MPA Injection Charge " << +fPulseAmplitude_MPA << std::endl;
    std::cout << " MPA Threshold " << +fPulseThreshold_MPA << std::endl;
    std::cout << " SSA Injection Charge " << +fPulseAmplitude_SSA << std::endl;
    std::cout << " SSA Threshold " << +fPulseThreshold_SSA << std::endl;
    //    LOG(INFO) << " MPA Injection Charge = " << fPulseAmplitude_MPA << " Threshold = " << fPulseThreshold_MPA;
    //    LOG(INFO) << " SSA Injection Charge = " << fPulseAmplitude_SSA << " Threshold = " << fPulseThreshold_SSA;

    // LOG(INFO) << " Fast Counter Readout [PS] " << +cEnableFastCounterReadout << RESET;

    // Saves original board config, so set back at the end
    ContainerFactory::copyAndInitBoard<BeBoardRegMap>(*fDetectorContainer, fBoardRegContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        auto&                cBoardRegNap = fBoardRegContainer.getObject(cBoard->getId())->getSummary<BeBoardRegMap>();
        const BeBoardRegMap& cOrigRegMap  = static_cast<const BeBoard*>(cBoard)->getBeBoardRegMap();
        cBoardRegNap.insert(cOrigRegMap.begin(), cOrigRegMap.end());
    }

    // for now.. force to use async mode here (does not track timing of hits), only used for testing
    bool cForcePSasync = true;
    // event types
    fEventTypes.clear();
    for(auto cBoard: *fDetectorContainer)
    {
        fEventTypes.push_back(cBoard->getEventType());
        if(!fWithSSA && !fWithMPA) continue;
        if(!cForcePSasync) continue;
        cBoard->setEventType(EventType::PSAS); // Sets up board to expect async input
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->InitializePSCounterFWInterface(cBoard);
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid) { fReadoutChipInterface->WriteChipReg(cChip, "AnalogueAsync", 0); } // Tells chip to expect async
            }
        }
    }

// Once I have the histogram code working add this back in
#ifdef __USE_ROOT__
    LOG(INFO) << BOLDYELLOW << "DQMHistogramPSPixelAlive Book" << RESET;
    fDQMHistogramPSPixelAlive.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

// Resets all registers saved in initialize, will need to look this over
void PSPixelAlive::Reset()
{
    LOG(INFO) << BOLDGREEN << "Resetting registers touched  by PixelAlive" << RESET;
    // set everything back to original values .. like I wasn't here
    for(auto cBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
        LOG(INFO) << BOLDBLUE << "Resetting all registers on back-end board " << +cBoard->getId() << RESET;
        auto&                                         cBeRegMap = fBoardRegContainer.getObject(cBoard->getId())->getSummary<BeBoardRegMap>();
        std::vector<std::pair<std::string, uint32_t>> cVecBeBoardRegs;
        cVecBeBoardRegs.clear();
        for(auto cReg: cBeRegMap) { cVecBeBoardRegs.push_back(make_pair(cReg.first, cReg.second.fValue)); }
        fBeBoardInterface->WriteBoardMultReg(theBoard, cVecBeBoardRegs);

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                LOG(INFO) << BOLDBLUE << "PixelAlive::Resetting all registers on readout chips connected to FEhybrid#" << +(cHybrid->getId()) << " back to their original values..." << RESET;

                for(auto cChip: *cHybrid)
                {
                    // Set this back to what it was
                    fReadoutChipInterface->WriteChipReg(cChip, "AnalogueAsync", 0);
                }
            }
        }
    }
    resetPointers();
}

// Print Out Results and runs functions
void PSPixelAlive::measurePixels()
{
    LOG(INFO) << BOLDBLUE << "Measure Pixel Occupancy" << RESET;
    measureOccupancy();
    // Commenting out plotting for now
    //    LOG(INFO) << BOLDBLUE << "Produce PixelAlive Plots" << RESET;
    //    producePSPixelAlivePlots();
    LOG(INFO) << BOLDBLUE << "Done" << RESET;
}

// Make own version of plotmaker
void PSPixelAlive::producePSPixelAlivePlots()
{
#ifdef __USE_ROOT__
    //    fDQMHistogramPixelAlive.fillPedestalAndNoisePlots(*fThresholdAndNoiseContainer);

#endif
}

// Make own version of channel masking
void PSPixelAlive::maskNoisyChannels(BoardDataContainer* board)
{
    /*
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    LOG(INFO) << BOLDYELLOW << chip->getId() << RESET;
                    auto chipDC = fDetectorContainer->at(board->getId())->at(opticalGroup->getId())->at(hybrid->getId())->at(chip->getId());
                    // auto cType = chipDC->getFrontEndType();

                    // uint32_t       NCH = NCHANNELS;
                    // if(cType == FrontEndType::CBC3)
                    //   NCH = NCHANNELS;
                    // else if(cType == FrontEndType::SSA2)
                    //  NCH = NSSACHANNELS;
                    // else if( cType == FrontEndType::MPA2)
                    //  NCH = NMPAROWS * NSSACHANNELS;

    //                float fMean=1.0;
    //                        if(cType == FrontEndType::MPA2)
    //                     fMean=2.7;
    //                        if(cType == FrontEndType::SSA2)
    //                     fMean=4.2;
                    auto     cOriginalMask = chipDC->getChipOriginalMask();
    //                uint32_t nMask         = 0;

                    for(uint16_t row = 0; row<roc->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col<roc->getNumberOfCols(); ++col)
                        {
        //                    float cPedestal = chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThreshold;
                            // float cNoise = chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fNoise;
                            // LOG(INFO) << BOLDYELLOW << "CHECK "<<iChannel <<", "<<chip->getChannel<ThresholdAndNoise>(row, col).fNoise<<" "<<fPedeNoiseLimit*fMean<<RESET;
                            // LOG(INFO) << BOLDYELLOW << "CHECK "<<iChannel <<", "<<std::fabs(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold -cPedestal)<<"
    "<<fPedeNoise_UntrimmedLimit<<RESET; if(fPedeNoiseMask and (chip->getChannel<ThresholdAndNoise>(row, col).fNoise > fPedeNoiseLimit))
                            {
                                nMask += 1;
                                LOG(INFO) << BOLDYELLOW << "Masking Channel: " << iChannel << " with a noise of " << chip->getChannel<ThresholdAndNoise>(row, col).fNoise << ", which is over the limit
    of "
                                        << fPedeNoiseLimit << RESET;
                                cOriginalMask->disableChannel(row, col); //Make version of this for masking pixels
                            }
                            if(fPedeNoise_MaskUntrimmed and std::fabs(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold - cPedestal) > fPedeNoise_UntrimmedLimit)
                            {
                                uint8_t thetrim = fReadoutChipInterface->ReadChipReg(static_cast<ReadoutChip*>(chipDC), "TrimDAC_P" + std::to_string(iChannel + 1));

                                nMask += 1;
                                LOG(INFO) << BOLDYELLOW << "Masking Channel:  " << iChannel << " with a pedestal difference of "
                                        << std::fabs(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold - cPedestal) << ", which is over the limit of " << fPedeNoise_UntrimmedLimit
                                        << " trimval: " << +thetrim << RESET;
                                cOriginalMask->disableChannel(row, col); //This is where the channel is disabled
                            }

                            // LOG(INFO) << BOLDYELLOW << "snorp SUMMARY TH "<<cPedestal <<RESET;
                            // LOG(INFO) << BOLDYELLOW << "snorp SUMMARY NOI "<<cNoise <<RESET;
                            // LOG(INFO) << BOLDYELLOW << "fPedeNoiseLimit "<<fPedeNoiseLimit<< " fPedeNoiseMask "<<fPedeNoiseMask <<RESET;
                            // LOG(INFO) << BOLDYELLOW << "Noise "<<iChannel<< ": "<<chip->getChannel<ThresholdAndNoise>(row, col).fNoise <<RESET;
                            // LOG(INFO) << BOLDYELLOW << "Thresh "<<iChannel<< ": "<<chip->getChannel<ThresholdAndNoise>(row, col).fThreshold  <<RESET;
                        }
                    }
                    // fReadoutChipInterface->maskChannelGroup(chipDC,cOriginalMask);
                    if(nMask > 0) LOG(INFO) << BOLDYELLOW << "PedeNoise masked " << nMask << " channels..." << RESET;
                    fReadoutChipInterface->ConfigureChipOriginalMask(chipDC);
                }

           }
        }
    */
}

void PSPixelAlive::writeObjects()
{
#ifdef __USE_ROOT__
    LOG(INFO) << BOLDYELLOW << "DQMHistogramPSPixelAlive Process" << RESET;
    fDQMHistogramPSPixelAlive.process();
#endif
}

// Modify for PixelAlive to run functions (Initialize, measurePixels, Reset)

void PSPixelAlive::Run()
{
    LOG(INFO) << "Starting Pixel Occupancy Measurement";
    Initialise();
    measurePixels();
    LOG(INFO) << "Done With Occupancy Measurement";
    Reset();
}

// Keep without changing

void PSPixelAlive::Stoper()
{
    LOG(INFO) << "Stopping Pixel Occupancy Measurement";
    Reset();
    writeObjects();
    dumpConfigFiles(); // Will Dump configuration (pixel masking is only thing changed) into results directory, copy to MPA files if we want to use same settings
    SaveResults();
    closeFileHandler();
    clearDataMembers();
    LOG(INFO) << "Pixel Occupancy Measurement Stopped.";
}

// These need to be here
void PSPixelAlive::Pause() {}

void PSPixelAlive::Resume() {}

// Measure Pixel Occupancy
void PSPixelAlive::measureOccupancy()
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(fWithSSA || fWithMPA)
        {
            // Allow for different SSA and MPA injection amplitudes
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto cType = cChip->getFrontEndType();

                        if(cType == FrontEndType::MPA2)
                        {
                            fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", fPulseAmplitude_MPA);
                            fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fPulseThreshold_MPA);
                        }
                        else
                        {
                            fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", fPulseAmplitude_SSA);
                            fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fPulseThreshold_SSA);
                        }
                    }
                }
            }
        }

        else
            setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "TestPulsePotNodeSel", fPulseAmplitude_MPA);
    }

    auto epsilon = findValueInSettings<double>("PixelEpsilon", 0.05); // Acceptable occupancy > 1 - epsilon

    // DetectorDataContainer* theOccupancyContainer = fRecycleBin.get(&ContainerFactory::copyAndInitStructure<Occupancy>, Occupancy());
    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);

    //    bool originalAllChannelFlag = this->fAllChan;
    //    this->setTestAllChannels(true);

    this->measureData(fEventsPerPoint, fNEventsPerBurst); // Sends pulses and measures occupancy

#ifdef __USE_ROOT__
    LOG(INFO) << BOLDMAGENTA << "Making DQMHistogramPSPixelAlive Plots" << RESET;
    fDQMHistogramPSPixelAlive.fillOccupancyPlots(theOccupancyContainer);
#else

    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("PSPixelAliveValidation");
        theContainerSerialization.streamByHybridContainer(fDQMStreamer, theOccupancyContainer);
    }
#endif
    LOG(INFO) << "measuring with " << fEventsPerPoint << " events and " << fNEventsPerBurst << " per burst";
    LOG(INFO) << BOLDMAGENTA << "PSPixelAlive Epsilon: " << epsilon << RESET;

    uint32_t nMask_MPA = 0; // number of masked pixels (MPA)
    uint32_t nMask_SSA = 0; // number of masked pixels (SSA)

    for(auto cBoard: *fDetectorContainer)
    {
        //	auto cBoardIdx = cBoard->getId();
        for(auto cOpticalGroup: *cBoard)
        {
            //            auto cOpticalGroupIdx = cOpticalGroup->getId();
            for(auto cHybrid: *cOpticalGroup)
            {
                //		auto cHybridIdx = cHybrid->getId();
                for(auto cChip: *cHybrid)
                { // Add one more loop to loop through each channel within each chip

                    LOG(INFO) << BLUE << "Pixel Alive Testing: Board(" << cBoard->getId() << ") Optical Group(" << cOpticalGroup->getId() << ") Hybrid(" << cHybrid->getId() << ") Chip("
                              << cChip->getId() << ")" << RESET;
                    fReadoutChipInterface->WriteChipReg(cChip, "AnalogueAsync", 0);
                    //                    auto cChipIdx       = cChip->getId();
                    auto cType = cChip->getFrontEndType();

                    auto chipDC        = fDetectorContainer->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId());
                    auto cOriginalMask = chipDC->getChipOriginalMask();

                    for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                        {
                            //			LOG (INFO) << RED << "Ch " << iChan << RESET ;
                            float occupancy = theOccupancyContainer.getObject(cBoard->getId())
                                                  ->getObject(cOpticalGroup->getId())
                                                  ->getObject(cHybrid->getId())
                                                  ->getObject(cChip->getId())
                                                  ->getChannel<Occupancy>(row, col)
                                                  .fOccupancy;

                            if(occupancy > (1.0 - epsilon))
                            {
                                //   LOG(INFO) << BLUE << "Occupancy Acceptable" << RESET;
                            }
                            else
                            {
                                //   LOG(INFO) << RED << "Occupancy Unaccptable" << RESET;
                                LOG(INFO) << RED << "Masking Channel(" << row << ", " << col << ") Occupancy = " << occupancy << RESET;
                                if(fPSPixelAliveMask)
                                {
                                    cOriginalMask->disableChannel(row, col); // Mask failing pixel
                                    if(cType == FrontEndType::SSA2)
                                        nMask_SSA++;
                                    else if(cType == FrontEndType::MPA2)
                                        nMask_MPA++;
                                }
                            }
                            if(occupancy >= 1.01) { LOG(INFO) << RED << "Occupancy Above 1.0" << RESET; }
                        }
                    }
                }
            }
        }
    }
    if(fPSPixelAliveMask)
    {
        LOG(INFO) << BOLDRED << "Masked " << nMask_SSA << " SSA Channels" << RESET;
        LOG(INFO) << BOLDRED << "Masked " << nMask_MPA << " MPA Channels" << RESET;
    }
}
