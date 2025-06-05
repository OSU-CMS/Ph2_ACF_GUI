/*!
  \file                  RD53Latency.cc
  \brief                 Implementaion of Latency scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53Latency.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void Latency::ConfigureCalibration()
{
    // ##############################
    // # Initialize sub-calibration #
    // ##############################
    PixelAlive::ConfigureCalibration();
    PixelAlive::doSaveData = false;
    RD53RunProgress::total() -= PixelAlive::getNumberIterations();

    // #######################
    // # Retrieve parameters #
    // #######################
    startValue = this->findValueInSettings<double>("LatencyStart");
    stopValue  = this->findValueInSettings<double>("LatencyStop");

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    const size_t nSteps = ((stopValue - startValue) / PixelAlive::nTRIGxEvent + 1 <= RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1 ? (stopValue - startValue) / PixelAlive::nTRIGxEvent + 1
                                                                                                                                        : RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1);
    const size_t step   = PixelAlive::nTRIGxEvent;
    for(auto i = 0u; i < nSteps; i++) dacList.push_back(startValue + step * i);

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += Latency::getNumberIterations();
}

void Latency::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[Latency::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(PixelAlive::saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_Latency.raw", 'w');
        this->initializeWriteFileHandler();
    }

    Latency::run();
    Latency::analyze();
    Latency::draw();
    Latency::sendData();
}

void Latency::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancySerialization("LatencyOccupancy");
        theOccupancySerialization.streamByChipContainer(fDQMStreamer, theOccContainer);

        ContainerSerialization theLatencySerialization("LatencyLatency");
        theLatencySerialization.streamByChipContainer(fDQMStreamer, theLatencyContainer);
    }
}

void Latency::Stop()
{
    LOG(INFO) << GREEN << "[Latency::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void Latency::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos             = nullptr;
    PixelAlive::histos = nullptr;

    LOG(INFO) << GREEN << "[Latency::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    Latency::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "Latency", histos, currentRun, PixelAlive::saveBinaryData);
}

void Latency::run()
{
    ContainerFactory::copyAndInitChip<std::vector<float>>(*fDetectorContainer, theOccContainer);
    CalibBase::fillVectorContainer<float>(theOccContainer, dacList.size(), 0);
    Latency::scanDac(frontEnd->latencyReg, dacList, &theOccContainer);

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void Latency::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(PixelAlive::doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(PixelAlive::doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    Latency::fillHisto();
    histos->process();

    if(PixelAlive::doDisplay == true) myApp->Run(true);
#endif
}

void Latency::analyze()
{
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theLatencyContainer);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float best   = 0;
                    int   regVal = 0;

                    for(auto i = 0u; i < dacList.size(); i++)
                    {
                        auto current = theOccContainer.getObject(cBoard->getId())
                                           ->getObject(cOpticalGroup->getId())
                                           ->getObject(cHybrid->getId())
                                           ->getObject(cChip->getId())
                                           ->getSummary<std::vector<float>>()
                                           .at(i);
                        if(current > best)
                        {
                            regVal = dacList[i];
                            best   = current;
                        }
                    }

                    if(PixelAlive::nTRIGxEvent > 1)
                        LOG(INFO) << BOLDMAGENTA << ">>> Best latency for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                                  << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] is within [" << BOLDYELLOW
                                  << (regVal - (int)PixelAlive::nTRIGxEvent + 1 >= 0 ? std::to_string(regVal - (int)PixelAlive::nTRIGxEvent + 1) : "N.A.") << "," << regVal << BOLDMAGENTA
                                  << "] (n.bx) <<<" << RESET;
                    else
                        LOG(INFO) << BOLDMAGENTA << ">>> Best latency for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                                  << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] is " << BOLDYELLOW << regVal << BOLDMAGENTA << " (n.bx) <<<" << RESET;

                    // ######################################################
                    // # Fill latency container and download new DAC values #
                    // ######################################################
                    theLatencyContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = regVal;
                    this->fReadoutChipInterface->WriteChipReg(static_cast<RD53*>(cChip), frontEnd->latencyReg, regVal);
                }
}

void Latency::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillOccupancy(theOccContainer);
    histos->fillLatency(theLatencyContainer);
#endif
}

void Latency::scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer)
{
    for(auto i = 0u; i < dacList.size(); i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName << BOLDMAGENTA << " broadcast value = " << BOLDYELLOW << dacList[i] << BOLDMAGENTA << " <<<" << RESET;
        CalibBase::WriteBroadcastChipReg(regName, dacList[i]);

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
        Latency::sendData();
    }
}
