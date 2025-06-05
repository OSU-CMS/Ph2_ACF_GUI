/*!
  \file                  RD53ThrAdjustment.cc
  \brief                 Implementaion of threshold adjustment
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53ThrAdjustment.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void ThrAdjustment::ConfigureCalibration()
{
    // ##############################
    // # Initialize sub-calibration #
    // ##############################
    PixelAlive::ConfigureCalibration();
    PixelAlive::doDisplay    = false;
    PixelAlive::doUpdateChip = false;
    RD53RunProgress::total() -= PixelAlive::getNumberIterations();

    // #######################
    // # Retrieve parameters #
    // #######################
    targetThreshold = this->findValueInSettings<double>("TargetThr");
    startValue      = this->findValueInSettings<double>("ThrStart");
    stopValue       = this->findValueInSettings<double>("ThrStop");
    doDisplay       = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip    = this->findValueInSettings<double>("UpdateChipCfg");

    PixelAlive::colStart = std::max(PixelAlive::colStart, frontEnd->colStart);
    PixelAlive::colStop  = std::min(PixelAlive::colStop, frontEnd->colStop);
    LOG(INFO) << GREEN << "ThrAdjustment will run on the " << RESET << BOLDYELLOW << frontEnd->name << RESET << GREEN << " FE, columns [" << RESET << BOLDYELLOW << colStart << ", " << colStop << RESET
              << GREEN << "]" << RESET;

    // ########################
    // # Custom channel group #
    // ########################
    for(auto row = PixelAlive::rowStart; row <= PixelAlive::rowStop; row++)
        for(auto col = PixelAlive::colStart; col <= PixelAlive::colStop; col++) PixelAlive::theChnGroupHandler->getRegionOfInterest().enableChannel(row, col);

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += ThrAdjustment::getNumberIterations();
}

void ThrAdjustment::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[ThrAdjustment::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(PixelAlive::saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_ThrAdjustment.raw", 'w');
        this->initializeWriteFileHandler();
    }

    ThrAdjustment::run();
    ThrAdjustment::analyze();
    ThrAdjustment::draw();
    ThrAdjustment::sendData();
    PixelAlive::sendData();
}

void ThrAdjustment::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("ThrAdjustmentThreshold");
        theContainerSerialization.streamByChipContainer(fDQMStreamer, theThrContainer);
    }
}

void ThrAdjustment::Stop()
{
    LOG(INFO) << GREEN << "[ThrAdjustment::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void ThrAdjustment::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos             = nullptr;
    PixelAlive::histos = nullptr;

    LOG(INFO) << GREEN << "[ThrAdjustment::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    ThrAdjustment::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "ThrAdjustment", histos, currentRun, PixelAlive::saveBinaryData);
    CalibBase::initializeFiles(histoFileName, "PixelAlive", PixelAlive::histos);
}

void ThrAdjustment::run()
{
    LOG(INFO) << RESET;
    LOG(INFO) << BOLDMAGENTA << ">>> Searching for threshold corresponding to " << std::setprecision(1) << BOLDYELLOW << TARGETEFF * 100 << BOLDMAGENTA << "% efficiency <<<" << RESET;
    ThrAdjustment::bitWiseScanGlobal(frontEnd->thresholdRegs, targetThreshold, startValue, stopValue);

    // ############################
    // # Fill threshold container #
    // ############################
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theThrContainer);
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                    theThrContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                        static_cast<RD53*>(cChip)->getReg(frontEnd->thresholdRegs[0]);

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void ThrAdjustment::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    ThrAdjustment::fillHisto();
    histos->process();

    PixelAlive::draw(false);

    if(doDisplay == true) myApp->Run(true);
#endif
}

void ThrAdjustment::analyze()
{
    for(const auto cBoard: theThrContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                    LOG(INFO) << GREEN << "Global threshold for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                              << +cChip->getId() << RESET << GREEN << "] is " << BOLDYELLOW << cChip->getSummary<uint16_t>() << RESET;
}

void ThrAdjustment::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fill(theThrContainer);
#endif
}

void ThrAdjustment::bitWiseScanGlobal(const std::vector<const char*>& regNames, float targetThreshold, uint16_t startValue, uint16_t stopValue)
{
    char           zeroC = 'L'; // 'H' and 'L' set direction
    uint16_t       init;
    const uint16_t numberOfBits = floor(log2(stopValue - startValue + 1) + 1);
    const float    goldenRatio  = (sqrt(5) - 1) / 2.;

    DetectorDataContainer outputMidH;
    DetectorDataContainer outputMidL;

    DetectorDataContainer directionContainer;
    DetectorDataContainer minDACcontainer;
    DetectorDataContainer midHDACcontainer;
    DetectorDataContainer downloadDACcontainer;
    DetectorDataContainer midLDACcontainer;
    DetectorDataContainer maxDACcontainer;
    DetectorDataContainer chargeContainer;

    ContainerFactory::copyAndInitChip<float>(*fDetectorContainer, outputMidH);
    ContainerFactory::copyAndInitChip<float>(*fDetectorContainer, outputMidL);

    ContainerFactory::copyAndInitChip<char>(*fDetectorContainer, directionContainer, zeroC);

    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, minDACcontainer, init = startValue);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, midHDACcontainer);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, downloadDACcontainer);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, midLDACcontainer);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, maxDACcontainer, init = (stopValue + 1));

    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, chargeContainer);

    // ##########################################
    // # Find VCAL_HIGH to get target threshold #
    // ##########################################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    uint16_t vcal_med_setting =
                        static_cast<RD53*>(fDetectorContainer->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()))
                            ->getReg("VCAL_MED");
                    uint16_t vcal_high_setting = round(RD53Shared::firstChip->Charge2VCal(targetThreshold)) + vcal_med_setting;
                    chargeContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = vcal_high_setting;

                    LOG(INFO) << GREEN << "The target threshold for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId()
                              << "/" << +cChip->getId() << RESET << GREEN "] is " << std::setprecision(1) << BOLDYELLOW << targetThreshold << RESET << GREEN << " electrons" << RESET;
                    LOG(INFO) << BOLDBLUE << "\t--> Closest charge setting is " << BOLDYELLOW << "VCAL_HIGH" << BOLDBLUE << " = " << BOLDYELLOW << vcal_high_setting << BOLDBLUE << " for "
                              << BOLDYELLOW << "VCAL_MED" << BOLDBLUE << " = " << BOLDYELLOW << vcal_med_setting << std::setprecision(-1) << RESET;
                }

    // #######################################################################
    // # Prepare query, disable all chips, and set weak check of data status #
    // #######################################################################
    CalibBase::prepareChipQueryForEnDis("chipSubset");
    CalibBase::setChipEnDis(false);
    RD53Event::weakCheckDataStatus = true;

    for(auto indx = 0u; indx < RD53FWconstants::NMAXCHIP_HYBRID; indx++)
    {
        if(CalibBase::shiftEnable(indx) == true) break;

        LOG(INFO) << RESET;
        LOG(INFO) << BOLDMAGENTA << ">>> Optimizing frontend chip #" << BOLDYELLOW << indx << BOLDMAGENTA << " <<<" << RESET;

        for(auto i = 0u; i <= numberOfBits + 1u; i++)
        {
            // ###########################
            // # Download new DAC values #
            // ###########################
            for(const auto cBoard: *fDetectorContainer)
                for(const auto cOpticalGroup: *cBoard)
                    for(const auto cHybrid: *cOpticalGroup)
                        for(const auto cChip: *cHybrid)
                        {
                            auto minDAC = minDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                            auto maxDAC = maxDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();

                            if(directionContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<char>() == 'L')
                            {
                                const uint16_t DACval = minDAC + (maxDAC - minDAC) * (1 - goldenRatio);

                                midLDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = DACval;
                                downloadDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    DACval;
                            }
                            else
                            {
                                const uint16_t DACval = minDAC + (maxDAC - minDAC) * goldenRatio;

                                midHDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = DACval;
                                downloadDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    DACval;
                            }
                        }

            // ################
            // # Run analysis #
            // ################
            ThrAdjustment::establishStartingPoint(chargeContainer);
            CalibBase::downloadNewDACvalues(downloadDACcontainer, regNames);
            PixelAlive::doSilentRunning = true;
            PixelAlive::run();
            PixelAlive::doSilentRunning = false;
            auto output                 = PixelAlive::analyze();

            // ##################
            // # Reset sequence #
            // ##################
            CalibBase::copyMaskFromDefault("en in");
            // ##############
            // # Full reset # // @TMP@
            // ##############
            // LOG(INFO) << BOLDMAGENTA << ">>> Resetting the system in case it got stuck due to high noise <<<" << RESET;
            // CalibBase::setChipEnDis(true);
            // CalibBase::ResetBoards();
            CalibBase::setChipEnDis(false);
            CalibBase::shiftEnable(indx);

            // ##############################################
            // # Send periodic data to monitor the progress #
            // ##############################################
            PixelAlive::sendData();

            // #####################
            // # Compute next step #
            // #####################
            for(const auto cBoard: *fDetectorContainer)
                for(const auto cOpticalGroup: *cBoard)
                    for(const auto cHybrid: *cOpticalGroup)
                        for(const auto cChip: *cHybrid)
                        {
                            // #######################
                            // # Build discriminator #
                            // #######################
                            auto value = output->getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<GenericDataVector, OccupancyAndPh>()
                                             .fOccupancy;

                            if(i == 0) // First iteration
                            {
                                outputMidL.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>()        = value;
                                directionContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<char>() = 'H';
                                continue;
                            }
                            else if(directionContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<char>() == 'L')
                                outputMidL.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>() = value;
                            else
                                outputMidH.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>() = value;

                            auto valueMidH = outputMidH.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>();
                            auto valueMidL = outputMidL.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>();

                            if(i == numberOfBits + 1u)
                            {
                                // #######################
                                // # Save best DAC value #
                                // #######################
                                if(fabs(valueMidL - TARGETEFF) < fabs(valueMidH - TARGETEFF))
                                {
                                    downloadDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                        midLDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                                }
                                else if(fabs(valueMidH - TARGETEFF) < fabs(valueMidL - TARGETEFF))
                                {
                                    downloadDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                        midHDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                                }
                                break; // Allows to compute last move
                            }

                            // #####################
                            // # Compute next move #
                            // #####################

                            // #########################
                            // # Set new window limits #
                            // #########################
                            if((valueMidL < valueMidH) || (valueMidL > TARGETEFF))
                                minDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    midLDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();

                            if(valueMidH > TARGETEFF)
                                minDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    midHDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();

                            if((valueMidL >= valueMidH) && (valueMidH < TARGETEFF))
                                maxDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    midHDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();

                            // #########################################
                            // # Set new internal values and direction #
                            // #########################################
                            if(valueMidL < valueMidH || valueMidH > TARGETEFF)
                            {
                                // If slope is positive or valueMidH is above TARGETEFF, Global Zero is to the right of midHDAC

                                midLDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    midHDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                                outputMidL.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>() = valueMidH;
                            }

                            if(valueMidL >= valueMidH && valueMidL < TARGETEFF)
                            {
                                // If slope is negative or zero and valueMidL is below TARGETEFF, Global Zero is to the left of midLDAC and we move left, otherwise we move right

                                midHDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                    midLDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                                outputMidH.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>() = valueMidL;

                                directionContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<char>() = 'L';
                            }
                            else
                                directionContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<char>() = 'H';
                        }
        }
    }

    // ########################################################################
    // # Restore query, enable all chips, and reset weak check of data status #
    // ########################################################################
    fDetectorContainer->resetReadoutChipQueryFunction();
    CalibBase::setChipEnDis(true);
    RD53Event::weakCheckDataStatus = false;

    // ###########################
    // # Download new DAC values #
    // ###########################
    LOG(INFO) << BOLDMAGENTA << ">>> Best values <<<" << RESET;
    CalibBase::downloadNewDACvalues(downloadDACcontainer, regNames, false, true, 0);

    // ################
    // # Run analysis #
    // ################
    PixelAlive::run();
    PixelAlive::analyze();

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");
}

void ThrAdjustment::establishStartingPoint(DetectorDataContainer& chargeContainer)
{
    CalibBase::downloadNewDACvalues(chargeContainer, {"VCAL_HIGH"}, true);
    PixelAlive::SetInjectionType();
}
