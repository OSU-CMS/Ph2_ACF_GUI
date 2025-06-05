/*!
  \file                  RD53PixelAlive.cc
  \brief                 Implementaion of PixelAlive scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53PixelAlive.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void PixelAlive::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    CalibBase::ConfigureCalibration();
    injType         = static_cast<RD53Shared::INJtype>(this->findValueInSettings<double>("INJtype"));
    nHITxCol        = this->findValueInSettings<double>("nHITxCol");
    doDataIntegrity = this->findValueInSettings<double>("DoDataIntegrity");
    doOnlyNGroups   = this->findValueInSettings<double>("DoOnlyNGroups");
    occPerPixel     = this->findValueInSettings<double>("OccPerPixel");
    unstuckPixels   = this->findValueInSettings<double>("UnstuckPixels");
    doDisplay       = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip    = this->findValueInSettings<double>("UpdateChipCfg");
    saveBinaryData  = this->findValueInSettings<double>("SaveBinaryData");
    frontEnd        = RD53Shared::firstChip->getFEtype(colStart, colStop);

    // ################################
    // # Custom channel group handler #
    // ################################
    const auto groupType = CalibBase::assignGroupType(injType);
    theChnGroupHandler =
        std::make_shared<RD53ChannelGroupHandler>(rowStart, rowStop, colStart, colStop, RD53Shared::firstChip->getNRows(), RD53Shared::firstChip->getNCols(), groupType, nHITxCol, doOnlyNGroups);
    this->setChannelGroupHandler(theChnGroupHandler);

    // ######################
    // # Set injection type #
    // ######################
    PixelAlive::SetInjectionType();

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += PixelAlive::getNumberIterations();
}

void PixelAlive::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[PixelAlive::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_PixelAlive.raw", 'w');
        this->initializeWriteFileHandler();
    }

    PixelAlive::run();
    PixelAlive::analyze();
    PixelAlive::draw();
    PixelAlive::sendData();
}

void PixelAlive::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancySerialization("PixelAliveOccupancy");
        theOccupancySerialization.streamByChipContainer(fDQMStreamer, *theOccContainer.get());

        ContainerSerialization theBCIDSerialization("PixelAliveBCID");
        theBCIDSerialization.streamByChipContainer(fDQMStreamer, theBCIDContainer);

        ContainerSerialization theTrgIDSerialization("PixelAliveTrgID");
        theTrgIDSerialization.streamByChipContainer(fDQMStreamer, theTrgIDContainer);
    }
}

void PixelAlive::Stop()
{
    LOG(INFO) << GREEN << "[PixelAlive::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void PixelAlive::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos = nullptr;

    LOG(INFO) << GREEN << "[PixelAlive::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    PixelAlive::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // ###################################################
    // # Initialize histograms and binary and root files #
    // ###################################################
    CalibBase::initializeFiles(histoFileName, "PixelAlive", histos, currentRun, saveBinaryData);
}

void PixelAlive::run()
{
    if(doDataIntegrity != 0)
    {
        RD53RunProgress::turnOFF();

        std::shared_ptr<DetectorDataContainer> localOccContainer = std::make_shared<DetectorDataContainer>();
        this->fDetectorDataContainer                             = localOccContainer.get();
        ContainerFactory::copyAndInitStructure<OccupancyAndPh, GenericDataVector>(*fDetectorContainer, *this->fDetectorDataContainer);

        std::shared_ptr<RD53ChannelGroupHandler> localChnGroupHandler = std::make_shared<RD53ChannelGroupHandler>(
            rowStart, rowStop, colStart, colStop, RD53Shared::firstChip->getNRows(), RD53Shared::firstChip->getNCols(), RD53GroupType::AllPixels, nHITxCol, doOnlyNGroups);
        this->setChannelGroupHandler(localChnGroupHandler);

        LOG(INFO) << GREEN << "[PixelAlive::run] Running detection of Core-Column data corruption" << RESET;

        for(const auto cBoard: *fDetectorContainer)
        {
            // ########################
            // # Start silent running #
            // ########################
            CalibBase::SilentRunning(true);

            // ############################
            // # Disable all core columns #
            // ############################
            for(const auto& regName: RD53Shared::firstChip->getFEtype(colStart, colStop)->CoreColRegs) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, regName, 0);

            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        const auto&                     coreColRegs          = static_cast<RD53*>(cChip)->getFEtype(colStart, colStop)->CoreColRegs;
                        size_t                          badPixelsCounterChip = 0;
                        std::map<std::string, uint16_t> regValueMap;

                        LOG(INFO) << GREEN << "Results for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                                  << +cChip->getId() << RESET << GREEN << "]" << RESET;

                        for(const auto& regName: coreColRegs)
                        {
                            const auto numberOfBits     = cChip->getNumberOfBits(regName);
                            const auto baseNumberOfBits = cChip->getNumberOfBits(coreColRegs.at(0));
                            regValueMap[regName]        = RD53Shared::setBits(numberOfBits);

                            for(auto i = 0u; i < numberOfBits; i++)
                            {
                                if((cChip->getRegMap()[regName].fDefValue & (1 << i)) == 0)
                                {
                                    regValueMap[regName] ^= 1 << i;
                                    continue;
                                }

                                // ###########################
                                // # Download new DAC values #
                                // ###########################
                                this->fReadoutChipInterface->WriteChipReg(cChip, regName, 1 << i, false);

                                // ################
                                // # Run analysis #
                                // ################
                                this->SetTestPulse(false);
                                this->fMaskChannelsFromOtherGroups = false;
                                this->measureData(10, 10);

                                // #####################
                                // # Compute next step #
                                // #####################
                                bool statusGood = true;
                                for(const auto& ev: RD53Event::decodedEvents)
                                    if(ev.eventStatus != RD53FWEvtEncoder::GOOD)
                                    {
                                        statusGood = false;
                                        break;
                                    }

                                size_t badPixelsCounterCoreCol = 0;
                                size_t testedPixels            = 0;
                                if(((doDataIntegrity == 2) || (doDataIntegrity == 3)) && ((statusGood == false) || (RD53Event::decodedEvents.size() == 0)))
                                {
                                    static_cast<RD53Interface*>(this->fReadoutChipInterface)->InitRD53Uplinks(cChip);
                                    this->fReadoutChipInterface->MaskAllChannels(cChip, true);

                                    const auto& ele     = std::find(coreColRegs.begin(), coreColRegs.end(), regName);
                                    const auto  coreCol = (ele - coreColRegs.begin()) * baseNumberOfBits + i;
                                    LOG(WARNING) << BOLDBLUE << "\t--> Found problematic Core-Column " << BOLDYELLOW << coreCol << BOLDBLUE << "(" << BOLDYELLOW
                                                 << RD53Shared::firstChip->getNCols() / RD53Constants::NROW_CORE << BOLDBLUE << ")" << RESET << GREEN
                                                 << " --> I'll try to nail down the problem at pixel level" << RESET;

                                    for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                                    {
                                        const auto colStart = coreCol * RD53Constants::NROW_CORE;
                                        for(auto col = colStart; col < colStart + RD53Constants::NROW_CORE; col++)
                                        {
                                            if(!cChip->getChipOriginalMask()->isChannelEnabled(row, col)) continue;

                                            // ################
                                            // # Run analysis #
                                            // ################
                                            CalibBase::setSinglePixel(cChip, row, col, true, true);
                                            this->SetTestPulse(false);
                                            this->fMaskChannelsFromOtherGroups = false;
                                            this->measureData(10, 10);
                                            CalibBase::setSinglePixel(cChip, row, col, false, false);
                                            testedPixels++;

                                            // #####################
                                            // # Compute next step #
                                            // #####################
                                            statusGood = true;
                                            for(const auto& ev: RD53Event::decodedEvents)
                                                if(ev.eventStatus != RD53FWEvtEncoder::GOOD)
                                                {
                                                    statusGood = false;
                                                    break;
                                                }

                                            if((statusGood == false) || (RD53Event::decodedEvents.size() == 0))
                                            {
                                                if(doDataIntegrity == 2)
                                                {
                                                    static_cast<RD53*>(cChip)->enableDefaultPixel(row, col, false);
                                                    badPixelsCounterCoreCol++;
                                                }
                                                else
                                                {
                                                    static_cast<RD53*>(cChip)->maskCoreDefault(row, col);
                                                    badPixelsCounterCoreCol += RD53Constants::NROW_CORE * RD53Constants::NROW_CORE;
                                                }
                                                static_cast<RD53Interface*>(this->fReadoutChipInterface)->InitRD53Uplinks(cChip);
                                            }

                                            if((testedPixels % NPIXELS_PRINTOUT) == 0)
                                                LOG(INFO) << BOLDBLUE << "\t--> Number of tested pixels: " << BOLDYELLOW << testedPixels << BOLDBLUE << "/" << BOLDYELLOW
                                                          << RD53Shared::firstChip->getNRows() * RD53Constants::NROW_CORE << RESET;
                                        }
                                    }

                                    badPixelsCounterChip += badPixelsCounterCoreCol;
                                    static_cast<RD53*>(cChip)->copyMaskFromDefault("en hb");
                                }

                                if(((doDataIntegrity == 1) && ((statusGood == false) || (RD53Event::decodedEvents.size() == 0))) ||
                                   (badPixelsCounterCoreCol == (RD53Shared::firstChip->getNRows() * RD53Constants::NROW_CORE)))
                                {
                                    regValueMap[regName] ^= 1 << i;
                                    static_cast<RD53Interface*>(this->fReadoutChipInterface)->InitRD53Uplinks(cChip);
                                }
                            }

                            // ###########################
                            // # Download new DAC values #
                            // ###########################
                            this->fReadoutChipInterface->WriteChipReg(cChip, regName, 0, false);
                        }

                        // ###########################
                        // # Download new DAC values #
                        // ###########################
                        for(const auto& regName: coreColRegs)
                        {
                            this->fReadoutChipInterface->WriteChipReg(cChip, regName, regValueMap[regName], false);
                            const uint16_t mask         = cChip->getRegMap()[regName].fDefValue;
                            const auto     numberOfBits = cChip->getNumberOfBits(regName);
                            const auto     value        = (std::bitset<RD53Constants::NBIT_MAXREG>(regValueMap[regName]) & std::bitset<RD53Constants::NBIT_MAXREG>(mask))
                                                   .to_string()
                                                   .erase(0, RD53Constants::NBIT_MAXREG - numberOfBits);
                            const bool problems = (regValueMap[regName] != mask);
                            LOG(INFO) << (problems ? BOLDRED : BOLDBLUE) << "\t--> " << BOLDYELLOW << regName << (problems ? BOLDRED : BOLDBLUE) << " = 0b" << BOLDYELLOW << value
                                      << (problems ? BOLDRED : BOLDBLUE) << " (0 = disabled)" << RESET;
                        }

                        if(((doDataIntegrity == 2) || (doDataIntegrity == 3)) && (badPixelsCounterChip != 0))
                            LOG(WARNING) << BOLDRED << "\t--> Found " << BOLDYELLOW << badPixelsCounterChip << BOLDRED << " bad pixel(s) in this chip --> masked" << RESET;
                        LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;
                    }

            // #######################
            // # Stop silent running #
            // #######################
            CalibBase::SilentRunning(false);
        }

        // ############################
        // # Reset to original values #
        // ############################
        this->setChannelGroupHandler(theChnGroupHandler);
        RD53RunProgress::turnON();
    }

    // #########################
    // # Run actual PixelAlive #
    // #########################
    CalibBase::SilentRunning(doSilentRunning);
    PixelAlive::runPixelAlive();
    CalibBase::SilentRunning(!doSilentRunning);
}

void PixelAlive::runPixelAlive()
{
    theOccContainer              = std::make_shared<DetectorDataContainer>();
    this->fDetectorDataContainer = theOccContainer.get();
    ContainerFactory::copyAndInitStructure<OccupancyAndPh, GenericDataVector>(*fDetectorContainer, *this->fDetectorDataContainer);

    const auto groupType = CalibBase::assignGroupType(injType);
    this->SetTestPulse(groupType != RD53GroupType::AllPixels);
    this->fMaskChannelsFromOtherGroups = (groupType != RD53GroupType::AllPixels);
    this->measureData(nEvents, nEvtsBurst);

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void PixelAlive::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    PixelAlive::fillHisto();
    histos->process();
    doSaveData = saveData;

    if(doDisplay == true) myApp->Run(true);
#endif
}

std::shared_ptr<DetectorDataContainer> PixelAlive::analyze()
{
    // ####################
    // # Clear containers #
    // ####################
    theBCIDContainer.reset();
    ContainerFactory::copyAndInitChip<std::vector<uint16_t>>(*fDetectorContainer, theBCIDContainer);
    CalibBase::fillVectorContainer<uint16_t>(theBCIDContainer, RD53Shared::firstChip->getMaxBCIDvalue() + 1, 0);

    theTrgIDContainer.reset();
    ContainerFactory::copyAndInitChip<std::vector<uint16_t>>(*fDetectorContainer, theTrgIDContainer);
    CalibBase::fillVectorContainer<uint16_t>(theTrgIDContainer, RD53Shared::firstChip->getMaxTRIGIDvalue() + 1, 0);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    size_t nMaskedPixelsPerCalib = 0;
                    if(injType == RD53Shared::INJtype::None)
                        theOccContainer->getObject(cBoard->getId())
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getObject(cChip->getId())
                            ->getSummary<GenericDataVector, OccupancyAndPh>()
                            .fOccupancy /= nTRIGxEvent;

                    LOG(INFO) << GREEN << "Average occupancy for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                              << +cChip->getId() << RESET << GREEN << "] is " << BOLDYELLOW
                              << theOccContainer->getObject(cBoard->getId())
                                     ->getObject(cOpticalGroup->getId())
                                     ->getObject(cHybrid->getId())
                                     ->getObject(cChip->getId())
                                     ->getSummary<GenericDataVector, OccupancyAndPh>()
                                     .fOccupancy
                              << RESET;

                    for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                        for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                            if(cChip->getChipOriginalMask()->isChannelEnabled(row, col) && this->getChannelGroupHandlerContainer()
                                                                                               ->getObject(cBoard->getId())
                                                                                               ->getObject(cOpticalGroup->getId())
                                                                                               ->getObject(cHybrid->getId())
                                                                                               ->getObject(cChip->getId())
                                                                                               ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                                                                               ->allChannelGroup()
                                                                                               ->isChannelEnabled(row, col))
                            {
                                if(injType == RD53Shared::INJtype::None)
                                    theOccContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<OccupancyAndPh>(row, col)
                                        .fOccupancy /= nTRIGxEvent;

                                float occupancy = theOccContainer->getObject(cBoard->getId())
                                                      ->getObject(cOpticalGroup->getId())
                                                      ->getObject(cHybrid->getId())
                                                      ->getObject(cChip->getId())
                                                      ->getChannel<OccupancyAndPh>(row, col)
                                                      .fOccupancy;
                                bool enable = (injType == RD53Shared::INJtype::None ? occupancy <= occPerPixel : occupancy >= occPerPixel);
                                if(unstuckPixels == false)
                                    static_cast<RD53*>(cChip)->enablePixel(row, col, enable);
                                else if(enable == false)
                                    static_cast<RD53*>(cChip)->setTDAC(row, col, 0);
                                if(enable == false)
                                {
                                    nMaskedPixelsPerCalib++;
                                    theOccContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<OccupancyAndPh>(row, col)
                                        .fStatus = RD53Shared::ISMASKED;
                                }
                            }
                            else
                                theOccContainer->getObject(cBoard->getId())
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getChannel<OccupancyAndPh>(row, col)
                                    .fStatus = RD53Shared::ISDISABLED;

                    if(unstuckPixels == false)
                    {
                        LOG(INFO) << BOLDBLUE << "\t--> Number of potentially " << BOLDYELLOW << "masked" << BOLDBLUE << " pixels in this iteration: " << BOLDYELLOW << nMaskedPixelsPerCalib << RESET;
                        LOG(INFO) << BOLDBLUE << "\t--> Total number of potentially masked pixels: " << BOLDYELLOW << static_cast<RD53*>(cChip)->getNbMaskedPixels() << RESET;
                    }
                    else
                        LOG(INFO) << BOLDBLUE << "\t--> Number of potentially " << BOLDYELLOW << "unstuck" << BOLDBLUE << " pixels in this iteration: " << BOLDYELLOW << nMaskedPixelsPerCalib << RESET;

                    // ######################################
                    // # Copy register values for streaming #
                    // ######################################

                    for(auto i = 1u; i < theOccContainer->getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<GenericDataVector, OccupancyAndPh>()
                                             .data1.size();
                        i++)
                    {
                        long int deltaBCID = theOccContainer->getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<GenericDataVector, OccupancyAndPh>()
                                                 .data1[i] -
                                             theOccContainer->getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<GenericDataVector, OccupancyAndPh>()
                                                 .data1.at(i - 1);
                        deltaBCID += (deltaBCID >= 0 ? 0 : RD53Shared::firstChip->getMaxBCIDvalue() + 1);
                        if(deltaBCID >= int(RD53Shared::firstChip->getMaxBCIDvalue()))
                            LOG(ERROR) << BOLDBLUE << "[PixelAlive::analyze] " << BOLDRED << "deltaBCID out of range: " << BOLDYELLOW << deltaBCID << RESET;
                        else
                            theBCIDContainer.getObject(cBoard->getId())
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<std::vector<uint16_t>>()
                                .at(deltaBCID)++;
                    }

                    for(auto i = 1u; i < theOccContainer->getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<GenericDataVector, OccupancyAndPh>()
                                             .data2.size();
                        i++)
                    {
                        long int deltaTrgID = theOccContainer->getObject(cBoard->getId())
                                                  ->getObject(cOpticalGroup->getId())
                                                  ->getObject(cHybrid->getId())
                                                  ->getObject(cChip->getId())
                                                  ->getSummary<GenericDataVector, OccupancyAndPh>()
                                                  .data2[i] -
                                              theOccContainer->getObject(cBoard->getId())
                                                  ->getObject(cOpticalGroup->getId())
                                                  ->getObject(cHybrid->getId())
                                                  ->getObject(cChip->getId())
                                                  ->getSummary<GenericDataVector, OccupancyAndPh>()
                                                  .data2.at(i - 1);
                        deltaTrgID += (deltaTrgID >= 0 ? 0 : RD53Shared::firstChip->getMaxTRIGIDvalue() + 1);
                        if(deltaTrgID > int(RD53Shared::firstChip->getMaxTRIGIDvalue()))
                            LOG(ERROR) << BOLDBLUE << "[PixelAlive::analyze] " << BOLDRED << "deltaTrgID out of range: " << BOLDYELLOW << deltaTrgID << RESET;
                        else
                            theTrgIDContainer.getObject(cBoard->getId())
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<std::vector<uint16_t>>()
                                .at(deltaTrgID)++;
                    }
                }

    return theOccContainer;
}

void PixelAlive::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fill(*theOccContainer.get());
    histos->fillBCID(theBCIDContainer);
    histos->fillTrgID(theTrgIDContainer);
#endif
}

void PixelAlive::SetInjectionType() { CalibBase::WriteBroadcastChipReg("DIGITAL_INJ_EN", ((injType == RD53Shared::INJtype::Digital) || (injType == RD53Shared::INJtype::SelfTrigger))); }
