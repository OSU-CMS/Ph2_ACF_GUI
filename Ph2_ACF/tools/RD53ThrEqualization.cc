/*!
  \file                  RD53ThrEqualization.cc
  \brief                 Implementaion of threshold equalization
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53ThrEqualization.h"
#include "HWDescription/RD53A.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void ThrEqualization::ConfigureCalibration()
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
    resetTDAC          = this->findValueInSettings<double>("ResetTDAC");
    startValue         = this->findValueInSettings<double>("VCalHstart");
    stopValue          = this->findValueInSettings<double>("VCalHstop");
    startTDACGainValue = this->findValueInSettings<double>("TDACGainStart");
    stopTDACGainValue  = this->findValueInSettings<double>("TDACGainStop");
    TDACGainNSteps     = this->findValueInSettings<double>("TDACGainNSteps", 1);
    doNSteps           = this->findValueInSettings<double>("DoNSteps");
    doDisplay          = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip       = this->findValueInSettings<double>("UpdateChipCfg");

    if(frontEnd == &RD53A::SYNC)
    {
        LOG(ERROR) << BOLDRED << "ThrEqualization cannot be used on the Synchronous FE, please change the selected columns" << RESET;
        exit(EXIT_FAILURE);
    }
    PixelAlive::colStart = std::max(PixelAlive::colStart, frontEnd->colStart);
    PixelAlive::colStop  = std::min(PixelAlive::colStop, frontEnd->colStop);
    LOG(INFO) << GREEN << "ThrEqualization will run on the " << RESET << BOLDYELLOW << frontEnd->name << RESET << GREEN << " FE, columns [" << GREEN << BOLDYELLOW << PixelAlive::colStart << ", "
              << PixelAlive::colStop << RESET << GREEN << "]" << RESET;

    // ########################
    // # Custom channel group #
    // ########################
    for(auto row = PixelAlive::rowStart; row <= PixelAlive::rowStop; row++)
        for(auto col = PixelAlive::colStart; col <= PixelAlive::colStop; col++) PixelAlive::theChnGroupHandler->getRegionOfInterest().enableChannel(row, col);

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    const float step = (TDACGainNSteps != 0 ? (stopTDACGainValue - startTDACGainValue) / TDACGainNSteps : 0);
    for(auto i = 0u; i < TDACGainNSteps; i++) dacList.push_back(startTDACGainValue + step * i);

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += ThrEqualization::getNumberIterations();
}

void ThrEqualization::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[ThrEqualization::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(PixelAlive::saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_ThrEqualization.raw", 'w');
        this->initializeWriteFileHandler();
    }

    ThrEqualization::run();
    ThrEqualization::analyze();
    ThrEqualization::draw();
    ThrEqualization::sendData();
    PixelAlive::sendData();
}

void ThrEqualization::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancySerialization("ThrEqualizationOccupancy");
        theOccupancySerialization.streamByChipContainer(fDQMStreamer, *theOccContainer.get());

        ContainerSerialization theTDACSerialization("ThrEqualizationTDAC");
        theTDACSerialization.streamByChipContainer(fDQMStreamer, theTDACContainer);

        ContainerSerialization theOccupancyScanSerialization("ThrEqualizationOccupancyScan");
        theOccupancyScanSerialization.streamByChipContainer(fDQMStreamer, theContainer);

        ContainerSerialization theTDACGainSerialization("ThrEqualizationTDACGain");
        theTDACGainSerialization.streamByChipContainer(fDQMStreamer, theTDACGainContainer);
    }
}

void ThrEqualization::Stop()
{
    LOG(INFO) << GREEN << "[ThrEqualization::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void ThrEqualization::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos             = nullptr;
    PixelAlive::histos = nullptr;

    LOG(INFO) << GREEN << "[ThrEqualization::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    ThrEqualization::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "ThrEqualization", histos, currentRun, PixelAlive::saveBinaryData);
    CalibBase::initializeFiles(histoFileName, "PixelAlive", PixelAlive::histos);
}

void ThrEqualization::run()
{
    if(TDACGainNSteps != 0)
    {
        // ###########################################
        // # Scan DAC and run threshold equalization #
        // ###########################################
        ContainerFactory::copyAndInitChip<std::vector<float>>(*fDetectorContainer, theContainer);
        CalibBase::fillVectorContainer<float>(theContainer, dacList.size(), 0);
        ThrEqualization::scanDac(frontEnd->TDACGainReg, dacList, &theContainer);

        // #######################################
        // # Run analysis to find best DAC value #
        // #######################################
        ThrEqualization::analyzeDuringRun();
    }

    // #########################
    // # Find global threshold #
    // #########################
    ThrEqualization::bitWiseScanGlobal("VCAL_HIGH", TARGETEFF, startValue, stopValue);

    // ##############################
    // # Run threshold equalization #
    // ##############################
    ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, theTDACContainer);
    ThrEqualization::bitWiseScanLocal(TARGETEFF, true);

    // #################################################
    // # Fill TDAC container and mark enabled channels #
    // #################################################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    this->fReadoutChipInterface->ReadChipAllLocalReg(
                        static_cast<RD53*>(cChip),
                        "PIX_PORTAL",
                        *theTDACContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()));

                    for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                        for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                            if(!cChip->getChipOriginalMask()->isChannelEnabled(row, col) || !this->getChannelGroupHandlerContainer()
                                                                                                 ->getObject(cBoard->getId())
                                                                                                 ->getObject(cOpticalGroup->getId())
                                                                                                 ->getObject(cHybrid->getId())
                                                                                                 ->getObject(cChip->getId())
                                                                                                 ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                                                                                 ->allChannelGroup()
                                                                                                 ->isChannelEnabled(row, col))
                            {
                                theOccContainer->getObject(cBoard->getId())
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getChannel<OccupancyAndPh>(row, col)
                                    .fStatus = RD53Shared::ISDISABLED;
                                theTDACContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) =
                                    frontEnd->nTDACvalues;
                            }
                }

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void ThrEqualization::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    ThrEqualization::fillHisto();
    histos->process();

    PixelAlive::draw(false);

    if(doDisplay == true) myApp->Run(true);
#endif
}

void ThrEqualization::analyze()
{
    const size_t TDACcenter = (resetTDAC < 0 ? frontEnd->nTDACvalues / 2 : resetTDAC);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float avgTDAC       = 0;
                    int   counter       = 0;
                    int   counterMinBin = 0;
                    int   counterMaxBin = 0;

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
                                static_cast<RD53*>(cChip)->setTDAC(row,
                                                                   col,
                                                                   theTDACContainer.getObject(cBoard->getId())
                                                                       ->getObject(cOpticalGroup->getId())
                                                                       ->getObject(cHybrid->getId())
                                                                       ->getObject(cChip->getId())
                                                                       ->getChannel<uint16_t>(row, col));

                                avgTDAC += theTDACContainer.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getChannel<uint16_t>(row, col);
                                counter++;

                                counterMinBin += (theTDACContainer.getObject(cBoard->getId())
                                                              ->getObject(cOpticalGroup->getId())
                                                              ->getObject(cHybrid->getId())
                                                              ->getObject(cChip->getId())
                                                              ->getChannel<uint16_t>(row, col) == 0
                                                      ? 1
                                                      : 0);
                                counterMaxBin += (theTDACContainer.getObject(cBoard->getId())
                                                              ->getObject(cOpticalGroup->getId())
                                                              ->getObject(cHybrid->getId())
                                                              ->getObject(cChip->getId())
                                                              ->getChannel<uint16_t>(row, col) == frontEnd->nTDACvalues - 1
                                                      ? 1
                                                      : 0);
                            }

                    avgTDAC /= counter;

                    // ###########################
                    // # Check TDAC distribution #
                    // ###########################
                    if(fabs(avgTDAC - TDACcenter) > MAXtdacDISTANCE)
                    {
                        LOG(WARNING) << BOLDRED << "Average TDAC distribution not centered around " << BOLDYELLOW << TDACcenter << BOLDRED << " (i.e. " << std::setprecision(1) << BOLDYELLOW << avgTDAC
                                     << BOLDRED << " - center > " << BOLDYELLOW << MAXtdacDISTANCE << BOLDRED << ") for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/"
                                     << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/" << +cChip->getId() << BOLDRED << "]" << std::setprecision(-1) << RESET;
                    }
                    else if((counterMaxBin == 0) && (counterMinBin == 0))
                        LOG(WARNING) << BOLDRED << "TDAC distribution is most likely empty for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId()
                                     << "/" << cHybrid->getId() << "/" << +cChip->getId() << BOLDRED << "]" << RESET;
                    else if(((frontEnd->nTDACvalues * counterMaxBin / (counterMinBin + counterMaxBin)) - TDACcenter) > MAXtdacDISTANCE)
                    {
                        LOG(WARNING) << BOLDRED << "Min and Max TDAC bins are not balanced (i.e. low TDAC value with " << std::setprecision(1) << BOLDYELLOW << counterMinBin << BOLDRED
                                     << " entries and high TDAC value with " << BOLDYELLOW << counterMaxBin << BOLDRED << " entries) for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW
                                     << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/" << +cChip->getId() << BOLDRED << "]" << std::setprecision(-1) << RESET;
                    }
                }
}

void ThrEqualization::analyzeDuringRun()
{
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theTDACGainContainer);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float best   = 1;
                    int   regVal = 0;

                    for(auto i = 0u; i < dacList.size(); i++)
                    {
                        auto current = round(theContainer.getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<std::vector<float>>()
                                                 .at(i) /
                                             RD53Shared::PRECISION) *
                                       RD53Shared::PRECISION;
                        if(current < best)
                        {
                            regVal = dacList[i];
                            best   = current;
                        }
                    }

                    LOG(INFO) << BOLDMAGENTA << ">>> Best TDAC gain for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId()
                              << "/" << +cChip->getId() << BOLDMAGENTA << "] is " << BOLDYELLOW << regVal << BOLDMAGENTA << " <<<" << RESET;

                    // ########################################################
                    // # Fill TDAC gain container and download new DAC values #
                    // ########################################################
                    theTDACGainContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = regVal;
                    this->fReadoutChipInterface->WriteChipReg(static_cast<RD53*>(cChip), frontEnd->TDACGainReg, regVal);
                }
}

void ThrEqualization::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillOccupancy(*theOccContainer.get());
    histos->fillTDAC(theTDACContainer);
    histos->fillOccupancyScan(theContainer);
    histos->fillTDACGain(theTDACGainContainer);
#endif
}

void ThrEqualization::scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer)
{
    for(auto i = 0u; i < dacList.size(); i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName << BOLDMAGENTA << " broadcast value = " << BOLDYELLOW << dacList[i] << BOLDMAGENTA << " <<<" << RESET;
        CalibBase::WriteBroadcastChipReg(regName, dacList[i]);

        // #########################
        // # Find global threshold #
        // #########################
        ThrEqualization::bitWiseScanGlobal("VCAL_HIGH", TARGETEFF, startValue, stopValue);

        // ##############################
        // # Run threshold equalization #
        // ##############################
        ThrEqualization::bitWiseScanLocal(TARGETEFF, false);

        // #####################
        // # Compute next step #
        // #####################
        for(const auto cBoard: *theOccContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        // #######################
                        // # Build discriminator #
                        // #######################
                        float  stdDev = 0;
                        size_t cnt    = 0;
                        for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                            for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                                if(fDetectorContainer->getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getChipOriginalMask()
                                       ->isChannelEnabled(row, col) &&
                                   this->getChannelGroupHandlerContainer()
                                       ->getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                       ->allChannelGroup()
                                       ->isChannelEnabled(row, col) &&
                                   cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy >= 0)
                                {
                                    auto value = cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy;
                                    stdDev += (value - TARGETEFF) * (value - TARGETEFF);
                                    cnt++;
                                }
                        stdDev = (cnt != 0 ? sqrt(stdDev / cnt) : 0);

                        // ###############
                        // # Save output #
                        // ###############
                        theContainer->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::vector<float>>().at(i) =
                            stdDev;

                        // ##############################################
                        // # Send periodic data to monitor the progress #
                        // ##############################################
                        ThrEqualization::sendData();
                    }
    }
}

void ThrEqualization::bitWiseScanGlobal(const std::string& regName, float target, uint16_t startValue, uint16_t stopValue)
{
    float          tmp = 0;
    uint16_t       init;
    const uint16_t numberOfBits = floor(log2(stopValue - startValue + 1) + 1);

    DetectorDataContainer minDACcontainer;
    DetectorDataContainer midDACcontainer;
    DetectorDataContainer maxDACcontainer;

    DetectorDataContainer bestDACcontainer;
    DetectorDataContainer bestContainer;

    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, minDACcontainer, init = startValue);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, midDACcontainer);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, maxDACcontainer, init = (stopValue + 1));

    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, bestDACcontainer, init = 0);
    ContainerFactory::copyAndInitChip<float>(*fDetectorContainer, bestContainer, tmp);

    for(auto i = 0u; i <= numberOfBits; i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                        midDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                            (minDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() +
                             maxDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>()) /
                            2;
        CalibBase::downloadNewDACvalues(midDACcontainer, {regName.c_str()});

        // ################
        // # Run analysis #
        // ################
        PixelAlive::run();
        auto output = PixelAlive::analyze();

        // ##############################################
        // # Send periodic data to monitor the progress #
        // ##############################################
        PixelAlive::sendData();

        // #####################
        // # Compute next step #
        // #####################
        for(const auto cBoard: *output)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        // #######################
                        // # Build discriminator #
                        // #######################
                        float newValue = cChip->getSummary<GenericDataVector, OccupancyAndPh>().fOccupancy;
                        // float newValue = cChip->getSummary<GenericDataVector, OccupancyAndPh>().fOccupancyMedian; // @TMP@

                        // ########################
                        // # Save best DAC values #
                        // ########################
                        float oldValue = bestContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>();

                        if(fabs(newValue - target) < fabs(oldValue - target))
                        {
                            bestContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>() = newValue;

                            bestDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                midDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                        }

                        if(newValue > target)

                            maxDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                midDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();

                        else

                            minDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                midDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                    }
    }

    // ###########################
    // # Download new DAC values #
    // ###########################
    LOG(INFO) << BOLDMAGENTA << ">>> Best values <<<" << RESET;
    CalibBase::downloadNewDACvalues(bestDACcontainer, {regName.c_str()}, false, true, 0);

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");
}

void ThrEqualization::bitWiseScanLocal(float target, bool updateDACs)
{
    float          tmp;
    uint16_t       init;
    const uint16_t numberOfBits = floor(log2(frontEnd->nTDACvalues) + 1);

    DetectorDataContainer minDACcontainer;
    DetectorDataContainer midDACcontainer;
    DetectorDataContainer maxDACcontainer;

    DetectorDataContainer bestDACcontainer;
    DetectorDataContainer bestContainer;

    ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, minDACcontainer, init = 0);
    ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, midDACcontainer);
    ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, maxDACcontainer, init = frontEnd->nTDACvalues);

    ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, bestDACcontainer);
    ContainerFactory::copyAndInitChannel<float>(*fDetectorContainer, bestContainer, tmp = (target < 0.5 ? 1 : 0));

    // ############################
    // # Read DAC starting values #
    // ############################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    this->fReadoutChipInterface->ReadChipAllLocalReg(
                        static_cast<RD53*>(cChip), "", *midDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()));
                    this->fReadoutChipInterface->ReadChipAllLocalReg(
                        static_cast<RD53*>(cChip), "", *bestDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()));
                }

    for(auto i = 0u; i <= (doNSteps != 0 ? doNSteps : numberOfBits); i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                        this->fReadoutChipInterface->WriteChipAllLocalReg(
                            static_cast<RD53*>(cChip), "", *midDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()));

        // ################
        // # Run analysis #
        // ################
        PixelAlive::run();
        auto output = PixelAlive::analyze();

        // ##############################################
        // # Send periodic data to monitor the progress #
        // ##############################################
        PixelAlive::sendData();

        // #####################
        // # Compute next step #
        // #####################
        for(const auto cBoard: *output)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                        for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                            for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                                if(fDetectorContainer->getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getChipOriginalMask()
                                       ->isChannelEnabled(row, col) &&
                                   this->getChannelGroupHandlerContainer()
                                       ->getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                       ->allChannelGroup()
                                       ->isChannelEnabled(row, col))
                                {
                                    // #######################
                                    // # Build discriminator #
                                    // #######################
                                    float newValue = cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy;

                                    // ########################
                                    // # Save best DAC values #
                                    // ########################
                                    float oldValue = bestContainer.getObject(cBoard->getId())
                                                         ->getObject(cOpticalGroup->getId())
                                                         ->getObject(cHybrid->getId())
                                                         ->getObject(cChip->getId())
                                                         ->getChannel<float>(row, col);

                                    if(fabs(newValue - target) <= fabs(oldValue - target))
                                    {
                                        bestContainer.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<float>(row, col) = newValue;
                                        bestDACcontainer.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<uint16_t>(row, col) = midDACcontainer.getObject(cBoard->getId())
                                                                                   ->getObject(cOpticalGroup->getId())
                                                                                   ->getObject(cHybrid->getId())
                                                                                   ->getObject(cChip->getId())
                                                                                   ->getChannel<uint16_t>(row, col);
                                    }

                                    if(newValue < target)

                                        minDACcontainer.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<uint16_t>(row, col) = midDACcontainer.getObject(cBoard->getId())
                                                                                   ->getObject(cOpticalGroup->getId())
                                                                                   ->getObject(cHybrid->getId())
                                                                                   ->getObject(cChip->getId())
                                                                                   ->getChannel<uint16_t>(row, col);

                                    else

                                        maxDACcontainer.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<uint16_t>(row, col) = midDACcontainer.getObject(cBoard->getId())
                                                                                   ->getObject(cOpticalGroup->getId())
                                                                                   ->getObject(cHybrid->getId())
                                                                                   ->getObject(cChip->getId())
                                                                                   ->getChannel<uint16_t>(row, col);

                                    if(doNSteps == 0)
                                        midDACcontainer.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<uint16_t>(row, col) = (minDACcontainer.getObject(cBoard->getId())
                                                                                    ->getObject(cOpticalGroup->getId())
                                                                                    ->getObject(cHybrid->getId())
                                                                                    ->getObject(cChip->getId())
                                                                                    ->getChannel<uint16_t>(row, col) +
                                                                                maxDACcontainer.getObject(cBoard->getId())
                                                                                    ->getObject(cOpticalGroup->getId())
                                                                                    ->getObject(cHybrid->getId())
                                                                                    ->getObject(cChip->getId())
                                                                                    ->getChannel<uint16_t>(row, col)) /
                                                                               2;
                                    else
                                    {
                                        auto& midDAC = midDACcontainer.getObject(cBoard->getId())
                                                           ->getObject(cOpticalGroup->getId())
                                                           ->getObject(cHybrid->getId())
                                                           ->getObject(cChip->getId())
                                                           ->getChannel<uint16_t>(row, col);
                                        if(newValue < target)
                                        {
                                            midDAC += 1;
                                            if(midDAC > (frontEnd->nTDACvalues - 1)) midDAC = frontEnd->nTDACvalues - 1;
                                        }
                                        else if(midDAC - 1 < 0)
                                            midDAC = 0;
                                        else
                                            midDAC -= 1;
                                    }
                                }
    }

    // ###########################
    // # Download new DAC values #
    // ###########################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    this->fReadoutChipInterface->WriteChipAllLocalReg(
                        static_cast<RD53*>(cChip), "", *bestDACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()));
                    if(updateDACs == true) static_cast<RD53*>(cChip)->copyMaskToDefault("td");
                }

    // ################
    // # Run analysis #
    // ################
    PixelAlive::run();
    theOccContainer = PixelAlive::analyze();

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in td");
}
