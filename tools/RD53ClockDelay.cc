/*!
  \file                  RD53ClockDelay.cc
  \brief                 Implementaion of Clock Delay scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53ClockDelay.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void ClockDelay::ConfigureCalibration()
{
    // ##############################
    // # Initialize sub-calibration #
    // ##############################
    PixelAlive::ConfigureCalibration();
    PixelAlive::doDisplay = false;
    RD53RunProgress::total() -= PixelAlive::getNumberIterations();

    // #######################
    // # Retrieve parameters #
    // #######################
    startValue = 0u;
    stopValue  = frontEnd->nLatencyBins2Span * (RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CLK_DATA_DELAY_CLK")) + 1) - 1;

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    const size_t nSteps = stopValue - startValue + 1;
    for(auto i = 0u; i < nSteps; i++) dacList.push_back(startValue + i);

    // ######################
    // # Initialize Latency #
    // ######################
    la.Inherit(this);
    la.ConfigureCalibration();

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += ClockDelay::getNumberIterations();
}

void ClockDelay::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[ClockDelay::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(PixelAlive::saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_ClockDelay.raw", 'w');
        this->initializeWriteFileHandler();
    }

    ClockDelay::run();
    ClockDelay::analyze();
    ClockDelay::draw();
    ClockDelay::sendData();
    la.sendData();
}

void ClockDelay::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancySerialization("ClockDelayOccupancy");
        theOccupancySerialization.streamByChipContainer(fDQMStreamer, theOccContainer);

        ContainerSerialization theClockDelaySerialization("ClockDelayClockDelay");
        theClockDelaySerialization.streamByChipContainer(fDQMStreamer, theClockDelayContainer);
    }
}

void ClockDelay::Stop()
{
    LOG(INFO) << GREEN << "[ClockDelay::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void ClockDelay::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos                = nullptr;
    la.histos             = nullptr;
    la.PixelAlive::histos = nullptr;
    PixelAlive::histos    = nullptr;

    LOG(INFO) << GREEN << "[ClockDelay::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    ClockDelay::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "ClockDelay", histos, currentRun, PixelAlive::saveBinaryData);

    // ######################
    // # Initialize Latency #
    // ######################
    std::string fileName = histoFileName;
    fileName.replace(fileName.find("_ClockDelay"), 15, "_Latency");
    la.CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);
    la.initializeFiles(fileName, "Latency", la.histos);
}

void ClockDelay::run()
{
    // ###############
    // # Run Latency #
    // ###############
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid) ClockDelay::writeClkDelaySequence(cBoard, cChip, 0);

    la.run();
    la.analyze();

    ContainerFactory::copyAndInitChip<std::vector<float>>(*fDetectorContainer, theOccContainer);
    CalibBase::fillVectorContainer<float>(theOccContainer, dacList.size(), 0);

    // #######################
    // # Set initial latency #
    // #######################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    auto latency = this->fReadoutChipInterface->ReadChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg);
                    this->fReadoutChipInterface->WriteChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg, latency - 1);
                }

    // ###############################
    // # Scan two adjacent latencies #
    // ###############################
    for(auto i = 0u; i < frontEnd->nLatencyBins2Span; i++)
    {
        std::vector<uint16_t> halfDacList(dacList.begin() + i * (dacList.end() - dacList.begin()) / frontEnd->nLatencyBins2Span,
                                          dacList.begin() + (i + 1) * (dacList.end() - dacList.begin()) / frontEnd->nLatencyBins2Span);

        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        auto latency = this->fReadoutChipInterface->ReadChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg);
                        this->fReadoutChipInterface->WriteChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg, latency + i);
                    }

        ClockDelay::scanDac("CLK_DATA_DELAY", halfDacList, &theOccContainer);
    }

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void ClockDelay::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(PixelAlive::doUpdateChip);
    la.draw(false);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(PixelAlive::doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    ClockDelay::fillHisto();
    histos->process();

    if(PixelAlive::doDisplay == true) myApp->Run(true);
#endif
}

void ClockDelay::analyze()
{
    const size_t maxRegValue = RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CLK_DATA_DELAY_CLK")) + 1;
    const auto unitTime = 1. / RD53Constants::ACCELERATOR_CLK * 1000 / ((RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CAL_EDGE_FINE_DELAY")) + 1) / (2. / frontEnd->nLatencyBins2Span));

    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theClockDelayContainer);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float  best   = 0u;
                    size_t regVal = 0u;

                    for(auto i = 0u; i < dacList.size(); i++)
                    {
                        auto current = round(theOccContainer.getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<std::vector<float>>()
                                                 .at(i) /
                                             RD53Shared::PRECISION) *
                                       RD53Shared::PRECISION;
                        if(current > best)
                        {
                            regVal = dacList[i];
                            best   = current;
                        }
                    }

                    LOG(INFO) << BOLDMAGENTA << ">>> Best clock delay for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                              << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] is " << BOLDYELLOW << regVal << BOLDMAGENTA << " (" << unitTime << " ns) computed over two bx <<<"
                              << RESET;
                    LOG(INFO) << BOLDMAGENTA << ">>> New clock delay dac value for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                              << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] is " << BOLDYELLOW << regVal % maxRegValue << BOLDMAGENTA << " <<<" << RESET;

                    // ####################################################
                    // # Fill delay container and download new DAC values #
                    // ####################################################
                    ClockDelay::writeClkDelaySequence(cBoard, cChip, regVal);

                    auto latency = this->fReadoutChipInterface->ReadChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg) - frontEnd->nLatencyBins2Span + regVal / maxRegValue + 1;
                    this->fReadoutChipInterface->WriteChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg, latency);

                    LOG(INFO) << BOLDMAGENTA << ">>> New latency dac value for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                              << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] is " << BOLDYELLOW << latency << BOLDMAGENTA << " <<<" << RESET;
                }
}

void ClockDelay::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillOccupancy(theOccContainer);
    histos->fillClockDelay(theClockDelayContainer);
#endif
}

void ClockDelay::scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer)
{
    for(auto i = 0u; i < dacList.size(); i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName << BOLDMAGENTA << " value = " << BOLDYELLOW << dacList[i] << BOLDMAGENTA << " <<<" << RESET;
        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid) ClockDelay::writeClkDelaySequence(cBoard, cChip, dacList[i]);

        // ################
        // # Run analysis #
        // ################
        PixelAlive::run();
        auto output = PixelAlive::analyze();

        // ###############
        // # Save output #
        // ###############
        for(const auto cBoard: *output)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                        theContainer->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::vector<float>>().at(i) =
                            cChip->getSummary<GenericDataVector, OccupancyAndPh>().fOccupancy;

        // ##############################################
        // # Send periodic data to monitor the progress #
        // ##############################################
        ClockDelay::sendData();
    }

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");
}

void ClockDelay::writeClkDelaySequence(const Ph2_HwDescription::BeBoard* pBoard, Ph2_HwDescription::ReadoutChip* pChip, uint16_t value)
{
    const size_t maxClkValue  = RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CLK_DATA_DELAY_CLK")) + 1;
    const size_t maxDataValue = RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CLK_DATA_DELAY_DATA")) + 1;

    // #################################################
    // # Move data and clock phases of the same amount #
    // #################################################
    auto clk_delay = pChip->getRegItem("CLK_DATA_DELAY_CLK").fValue;
    auto nameAndValue(static_cast<RD53Interface*>(this->fReadoutChipInterface)->SetSpecialRegister("CLK_DATA_DELAY_CLK", value % maxClkValue, pChip->getRegMap()));
    pChip->setReg("CLK_DATA_DELAY", nameAndValue.second);
    auto data_delay = (pChip->getRegItem("CLK_DATA_DELAY_DATA").fValue + (value % maxClkValue) - clk_delay) % maxDataValue; // Apply to data the same shift of the clock
    nameAndValue    = static_cast<RD53Interface*>(this->fReadoutChipInterface)->SetSpecialRegister("CLK_DATA_DELAY_DATA", data_delay, pChip->getRegMap());
    pChip->setReg("CLK_DATA_DELAY_CLK", value % maxClkValue);
    pChip->setReg("CLK_DATA_DELAY_DATA", data_delay);

    static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteClockDataDelay(pChip, nameAndValue.second);
}
