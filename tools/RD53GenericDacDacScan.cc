/*!
  \file                  RD53GenericDacDacScan.cc
  \brief                 Implementaion of a generic DAC-DAC scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/05/21
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53GenericDacDacScan.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void GenericDacDacScan::ConfigureCalibration()
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
    regNameDAC1    = this->findValueInSettings<std::string>("RegNameDAC1");
    startValueDAC1 = this->findValueInSettings<double>("StartValueDAC1");
    stopValueDAC1  = this->findValueInSettings<double>("StopValueDAC1");
    stepDAC1       = this->findValueInSettings<double>("StepDAC1", 1);
    regNameDAC2    = this->findValueInSettings<std::string>("RegNameDAC2");
    startValueDAC2 = this->findValueInSettings<double>("StartValueDAC2");
    stopValueDAC2  = this->findValueInSettings<double>("StopValueDAC2");
    stepDAC2       = this->findValueInSettings<double>("StepDAC2", 1);

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    size_t nSteps = (stopValueDAC1 - startValueDAC1) / stepDAC1 + 1;
    for(auto i = 0u; i < nSteps; i++) dac1List.push_back(startValueDAC1 + stepDAC1 * i);
    nSteps = (stopValueDAC2 - startValueDAC2) / stepDAC2 + 1;
    for(auto i = 0u; i < nSteps; i++) dac2List.push_back(startValueDAC2 + stepDAC2 * i);

    // ##################################
    // # Check if it's RD53 or FPGA reg #
    // ##################################
    isDAC1ChipReg = (regNameDAC1.find(".") == std::string::npos ? true : false);
    isDAC2ChipReg = (regNameDAC2.find(".") == std::string::npos ? true : false);

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += GenericDacDacScan::getNumberIterations();
}

void GenericDacDacScan::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[GenericDacDacScan::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(PixelAlive::saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_GenericDacDacScan.raw", 'w');
        this->initializeWriteFileHandler();
    }

    GenericDacDacScan::run();
    GenericDacDacScan::analyze();
    GenericDacDacScan::draw();
    GenericDacDacScan::sendData();
}

void GenericDacDacScan::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancySerialization("GenericDacDacScanOccupancy");
        theOccupancySerialization.streamByChipContainer(fDQMStreamer, theOccContainer);

        ContainerSerialization theDACDACSerialization("GenericDacDacDACDAC");
        theDACDACSerialization.streamByChipContainer(fDQMStreamer, theGenericDacDacContainer);
    }
}

void GenericDacDacScan::Stop()
{
    LOG(INFO) << GREEN << "[GenericDacDacScan::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void GenericDacDacScan::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos             = nullptr;
    PixelAlive::histos = nullptr;

    LOG(INFO) << GREEN << "[GenericDacDacScan::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    GenericDacDacScan::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "GenericDacDacScan", histos, currentRun, PixelAlive::saveBinaryData);
}

void GenericDacDacScan::run()
{
    CalibBase::showErrorReport = false;
    ContainerFactory::copyAndInitChip<std::vector<float>>(*fDetectorContainer, theOccContainer);
    CalibBase::fillVectorContainer<float>(theOccContainer, dac1List.size() * dac2List.size(), 0);
    GenericDacDacScan::scanDacDac(regNameDAC1, regNameDAC2, dac1List, dac2List, &theOccContainer);
    CalibBase::showErrorReport = true;

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void GenericDacDacScan::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(PixelAlive::doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(PixelAlive::doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    GenericDacDacScan::fillHisto();
    histos->process();

    if(PixelAlive::doDisplay == true) myApp->Run(true);
#endif
}

void GenericDacDacScan::analyze()
{
    ContainerFactory::copyAndInitChip<std::pair<uint16_t, uint16_t>>(*fDetectorContainer, theGenericDacDacContainer);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float  best    = 0u;
                    size_t regVal1 = 0u;
                    size_t regVal2 = 0u;

                    // #####################################################
                    // # Inverted loop (dac2 external and dac1 internal)   #
                    // # in order to find the lowest value in tornado plot #
                    // #####################################################
                    for(auto j = 0u; j < dac2List.size(); j++)
                        for(auto i = 0u; i < dac1List.size(); i++)
                        {
                            auto current = round(theOccContainer.getObject(cBoard->getId())
                                                     ->getObject(cOpticalGroup->getId())
                                                     ->getObject(cHybrid->getId())
                                                     ->getObject(cChip->getId())
                                                     ->getSummary<std::vector<float>>()
                                                     .at(i * dac2List.size() + j) /
                                                 RD53Shared::PRECISION) *
                                           RD53Shared::PRECISION;
                            if(current > best)
                            {
                                regVal1 = dac1List[i];
                                regVal2 = dac2List[j];
                                best    = current;
                            }
                        }

                    LOG(INFO) << BOLDMAGENTA << ">>> Best register values for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                              << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] are " << BOLDYELLOW << regVal1 << BOLDMAGENTA << " for " << BOLDYELLOW << regNameDAC1 << BOLDMAGENTA
                              << " and " << BOLDYELLOW << regVal2 << BOLDMAGENTA << " for " << BOLDYELLOW << regNameDAC2 << RESET;

                    // ######################################################
                    // # Fill latency container and download new DAC values #
                    // ######################################################
                    theGenericDacDacContainer.getObject(cBoard->getId())
                        ->getObject(cOpticalGroup->getId())
                        ->getObject(cHybrid->getId())
                        ->getObject(cChip->getId())
                        ->getSummary<std::pair<uint16_t, uint16_t>>() = std::pair<uint16_t, uint16_t>(regVal1, regVal2);
                }
}

void GenericDacDacScan::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillOccupancy(theOccContainer);
    histos->fillGenericDacDac(theGenericDacDacContainer);
#endif
}

void GenericDacDacScan::scanDacDac(const std::string&           regNameDAC1,
                                   const std::string&           regNameDAC2,
                                   const std::vector<uint16_t>& dac1List,
                                   const std::vector<uint16_t>& dac2List,
                                   DetectorDataContainer*       theContainer)
{
    for(auto i = 0u; i < dac1List.size(); i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regNameDAC1 << BOLDMAGENTA << " broadcast value = " << BOLDYELLOW << dac1List[i] << BOLDMAGENTA << " <<<" << RESET;
        if(isDAC1ChipReg == true)
            CalibBase::WriteBroadcastChipReg(regNameDAC1, dac1List[i]);
        else
            for(const auto cBoard: *fDetectorContainer)
                static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])
                    ->WriteArbitraryRegister(regNameDAC1, dac1List[i], cBoard, this->fReadoutChipInterface, (regNameDAC2.find("cdr") != std::string::npos ? true : false));

        for(auto j = 0u; j < dac2List.size(); j++)
        {
            // ###########################
            // # Download new DAC values #
            // ###########################
            LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regNameDAC2 << BOLDMAGENTA << " broadcast value = " << BOLDYELLOW << dac2List[j] << BOLDMAGENTA << " <<<" << RESET;
            if(isDAC2ChipReg == true)
                CalibBase::WriteBroadcastChipReg(regNameDAC2, dac2List[j]);
            else
                for(const auto cBoard: *fDetectorContainer)
                    static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])
                        ->WriteArbitraryRegister(regNameDAC2, dac2List[j], cBoard, this->fReadoutChipInterface, (regNameDAC2.find("cdr") != std::string::npos ? true : false));

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
                        {
                            float occ = cChip->getSummary<GenericDataVector, OccupancyAndPh>().fOccupancy;
                            theContainer->getObject(cBoard->getId())
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<std::vector<float>>()
                                .at(i * dac2List.size() + j) = occ;
                        }

            // ##############################################
            // # Send periodic data to monitor the progress #
            // ##############################################
            GenericDacDacScan::sendData();
        }
    }

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");
}
