/*!
  \file                  RD53SCurve.cc
  \brief                 Implementaion of SCurve scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53SCurve.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void SCurve::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    CalibBase::ConfigureCalibration();
    injType        = static_cast<RD53Shared::INJtype>(this->findValueInSettings<double>("INJtype"));
    startValue     = this->findValueInSettings<double>("VCalHstart");
    stopValue      = this->findValueInSettings<double>("VCalHstop");
    nSteps         = this->findValueInSettings<double>("VCalHnsteps", 1);
    offset         = this->findValueInSettings<double>("VCalMED");
    nHITxCol       = this->findValueInSettings<double>("nHITxCol");
    doOnlyNGroups  = this->findValueInSettings<double>("DoOnlyNGroups");
    doDisplay      = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip   = this->findValueInSettings<double>("UpdateChipCfg");
    saveBinaryData = this->findValueInSettings<double>("SaveBinaryData");
    frontEnd       = RD53Shared::firstChip->getFEtype(colStart, colStop);

    // ########################
    // # Custom channel group #
    // ########################
    auto groupType = CalibBase::assignGroupType(injType);
    theChnGroupHandler =
        std::make_shared<RD53ChannelGroupHandler>(rowStart, rowStop, colStart, colStop, RD53Shared::firstChip->getNRows(), RD53Shared::firstChip->getNCols(), groupType, nHITxCol, doOnlyNGroups);
    this->setChannelGroupHandler(theChnGroupHandler);

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    const float step = (stopValue - startValue) / nSteps;
    for(auto i = 0u; i < nSteps; i++) dacList.push_back(startValue + step * i);

    // #################################
    // # Initialize container recycler #
    // #################################
    theRecyclingBin.setDetectorContainer(fDetectorContainer);

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += SCurve::getNumberIterations();
}

void SCurve::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[SCurve::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_SCurve.raw", 'w');
        this->initializeWriteFileHandler();
    }

    SCurve::run();
    SCurve::analyze();
    SCurve::draw();
    SCurve::sendData();
}

void SCurve::sendData()
{
    if(fDQMStreamerEnabled)
    {
        if(theThresholdAndNoiseContainer != nullptr)
        {
            ContainerSerialization theThresholdAndNoiseSerialization("SCurveThresholdAndNoise");
            theThresholdAndNoiseSerialization.streamByChipContainer(fDQMStreamer, *theThresholdAndNoiseContainer.get());
        }

        size_t                 index = 0;
        ContainerSerialization theOccupancySerialization("SCurveOccupancy");
        for(const auto theOccContainer: detectorContainerVector)
        {
            uint16_t deltaVcal = dacList[index++] - offset + (stopValue - startValue) / (2 * nSteps);
            theOccupancySerialization.streamByChipContainer(fDQMStreamer, *theOccContainer, deltaVcal);
        }
    }
}

void SCurve::Stop()
{
    LOG(INFO) << GREEN << "[SCurve::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void SCurve::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos = nullptr;

    LOG(INFO) << GREEN << "[SCurve::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    SCurve::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "SCurve", histos, currentRun, saveBinaryData);
}

void SCurve::run()
{
    // ##########################
    // # Set new VCAL_MED value #
    // ##########################
    CalibBase::WriteBroadcastChipReg("VCAL_MED", offset);

    for(auto container: detectorContainerVector) theRecyclingBin.free(container);
    detectorContainerVector.clear();
    for(auto i = 0u; i < dacList.size(); i++) detectorContainerVector.push_back(theRecyclingBin.get(&ContainerFactory::copyAndInitStructure<OccupancyAndPh>, OccupancyAndPh()));

    auto groupType = CalibBase::assignGroupType(injType);
    this->SetBoardBroadcast(true);
    this->SetTestPulse(groupType != RD53GroupType::AllPixels);
    this->fMaskChannelsFromOtherGroups = true;
    this->scanDac("VCAL_HIGH", dacList, nEvents, detectorContainerVector);

    // #########################
    // # Mark enabled channels #
    // #########################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
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
                                for(auto i = 0u; i < dacList.size(); i++)
                                    detectorContainerVector[i]
                                        ->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<OccupancyAndPh>(row, col)
                                        .fStatus = RD53Shared::ISDISABLED;

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void SCurve::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    SCurve::fillHisto();
    histos->process();
    doSaveData = saveData;

    if(doDisplay == true) myApp->Run(true);
#endif

    // #####################
    // # @TMP@ : CalibFile #
    // #####################
    if(saveBinaryData == true) CalibBase::saveSCurveOrGaindValues(detectorContainerVector, dacList, offset, nEvents, "SCurve");
}

std::shared_ptr<DetectorDataContainer> SCurve::analyze()
{
    float              nHits, mean, rms;
    std::vector<float> measurements(dacList.size(), 0);

    theThresholdAndNoiseContainer = std::make_shared<DetectorDataContainer>();
    ContainerFactory::copyAndInitStructure<ThresholdAndNoise>(*fDetectorContainer, *theThresholdAndNoiseContainer);
    DetectorDataContainer theMaxThresholdContainer;
    ContainerFactory::copyAndInitChip<float>(*fDetectorContainer, theMaxThresholdContainer, mean = 0);

    size_t index = 0;
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
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
                                for(auto i = 0u; i < dacList.size(); i++)
                                    measurements[i] = detectorContainerVector[i]
                                                          ->getObject(cBoard->getId())
                                                          ->getObject(cOpticalGroup->getId())
                                                          ->getObject(cHybrid->getId())
                                                          ->getObject(cChip->getId())
                                                          ->getChannel<OccupancyAndPh>(row, col)
                                                          .fOccupancy;

                                SCurve::computeStats(measurements, offset, nHits, mean, rms);

                                if((mean > 0) && (rms > 0) && (nHits > 0) && (std::isnormal(rms) == true))
                                {
                                    theThresholdAndNoiseContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<ThresholdAndNoise>(row, col)
                                        .fThreshold = mean;
                                    theThresholdAndNoiseContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<ThresholdAndNoise>(row, col)
                                        .fThresholdError = rms / sqrt(nHits);
                                    theThresholdAndNoiseContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<ThresholdAndNoise>(row, col)
                                        .fNoise = rms;
                                    theThresholdAndNoiseContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<ThresholdAndNoise>(row, col)
                                        .fNoiseError = (nHits > 1 ? rms / sqrt(nHits) * sqrt(sqrt(2 / (nHits - 1))) : 0);

                                    if(mean > theMaxThresholdContainer.getObject(cBoard->getId())
                                                  ->getObject(cOpticalGroup->getId())
                                                  ->getObject(cHybrid->getId())
                                                  ->getObject(cChip->getId())
                                                  ->getSummary<float>())
                                        theMaxThresholdContainer.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<float>() = mean;
                                }
                                else
                                    theThresholdAndNoiseContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<ThresholdAndNoise>(row, col)
                                        .fNoise = RD53Shared::ISFITERROR;
                            }

                    index++;
                }

    theThresholdAndNoiseContainer->resetNormalizationStatus();
    theThresholdAndNoiseContainer->normalizeAndAverageContainers(fDetectorContainer, this->getChannelGroupHandlerContainer(), 1);

    for(const auto cBoard: *theThresholdAndNoiseContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    LOG(INFO) << GREEN << "Average threshold for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                              << +cChip->getId() << RESET << GREEN << "] is " << BOLDYELLOW << std::fixed << std::setprecision(1)
                              << cChip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThreshold << RESET << GREEN << " (Delta_VCal)" << std::setprecision(-1) << RESET;
                    LOG(INFO) << BOLDBLUE << "\t--> Highest threshold: " << BOLDYELLOW << std::fixed << std::setprecision(1)
                              << theMaxThresholdContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<float>()
                              << BOLDBLUE << " (Delta_VCal)" << std::setprecision(-1) << RESET;
                    RD53Shared::resetDefaultFloat();
                }

    return theThresholdAndNoiseContainer;
}

void SCurve::fillHisto()
{
#ifdef __USE_ROOT__
    for(auto i = 0u; i < dacList.size(); i++) histos->fillOccupancy(*detectorContainerVector[i], dacList[i] - offset + (stopValue - startValue) / (2 * nSteps));
    histos->fillThrAndNoise(*theThresholdAndNoiseContainer);
#endif
}

void SCurve::computeStats(std::vector<float>& measurements, int offset, float& nHits, float& mean, float& rms)
{
    float mean2  = 0;
    float weight = 0;
    mean         = 0;

    std::for_each(measurements.begin(), measurements.end(), [](float& ele) { ele = (std::fabs(ele) > 1. ? 1. : std::fabs(ele)); });
    std::reverse(measurements.begin(), measurements.end());
    const auto itHigh = measurements.end() - std::max_element(measurements.begin(), measurements.end());

    std::reverse(measurements.begin(), measurements.end());
    const auto itLow = std::max_element(measurements.begin(), measurements.end()) - measurements.begin();

    const auto stop = std::min<int>((itHigh + itLow) / 2, dacList.size() - 1);

    for(auto i = 0; i < stop; i++)
    {
        auto measurement = std::fabs(measurements[i + 1] - measurements[i]);
        auto dacCenter   = (dacList[i] + dacList[i + 1]) / 2.;

        mean += measurement * (dacCenter - offset);
        weight += measurement;
        mean2 += measurement * (dacCenter - offset) * (dacCenter - offset);
    }

    nHits = weight * nEvents;

    if((weight > 0) && (mean > 0))
    {
        mean /= weight;
        rms = sqrt((mean2 / weight - mean * mean) * weight / (weight - 1. / nEvents));
    }
    else
    {
        mean = 0;
        rms  = 0;
    }
}
