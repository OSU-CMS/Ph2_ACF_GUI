/*!
  \file                  RD53DataReadbackOptimization.cc
  \brief                 Implementaion of data readback optimization scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53DataReadbackOptimization.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void DataReadbackOptimization::ConfigureCalibration()
{
    // ##############################
    // # Initialize sub-calibration #
    // ##############################
    BERtest::ConfigureCalibration();

    // #######################
    // # Retrieve parameters #
    // #######################
    startValueTAP0 = this->findValueInSettings<double>("TAP0Start");
    stopValueTAP0  = this->findValueInSettings<double>("TAP0Stop");
    startValueTAP1 = this->findValueInSettings<double>("TAP1Start");
    stopValueTAP1  = this->findValueInSettings<double>("TAP1Stop");
    invTAP1        = this->findValueInSettings<double>("InvTAP1");
    startValueTAP2 = this->findValueInSettings<double>("TAP2Start");
    stopValueTAP2  = this->findValueInSettings<double>("TAP2Stop");
    invTAP2        = this->findValueInSettings<double>("InvTAP2");
    doUpdateChip   = this->findValueInSettings<double>("UpdateChipCfg");

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    size_t nSteps = (stopValueTAP0 - startValueTAP0 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP0 - startValueTAP0 + 1);
    size_t step   = floor((stopValueTAP0 - startValueTAP0 + 1) / nSteps);
    for(auto i = 0u; i < nSteps; i++) dacListTAP0.push_back(startValueTAP0 + step * i);

    nSteps = (stopValueTAP1 - startValueTAP1 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP1 - startValueTAP1 + 1);
    step   = floor((stopValueTAP1 - startValueTAP1 + 1) / nSteps);
    for(auto i = 0u; i < nSteps; i++) dacListTAP1.push_back(startValueTAP1 + step * i);

    nSteps = (stopValueTAP2 - startValueTAP2 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP2 - startValueTAP2 + 1);
    step   = floor((stopValueTAP2 - startValueTAP2 + 1) / nSteps);
    for(auto i = 0u; i < nSteps; i++) dacListTAP2.push_back(startValueTAP2 + step * i);
}

void DataReadbackOptimization::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[DataReadbackOptimization::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    DataReadbackOptimization::run();
    DataReadbackOptimization::draw();
    DataReadbackOptimization::sendData();
}

void DataReadbackOptimization::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theTAP0scanSerialization("DataReadbackOptimizationTAP0scan");
        theTAP0scanSerialization.streamByChipContainer(fDQMStreamer, theTAP0scanContainer);

        ContainerSerialization theTAP0Serialization("DataReadbackOptimizationTAP0");
        theTAP0Serialization.streamByChipContainer(fDQMStreamer, theTAP0Container);

        ContainerSerialization theTAP1scanSerialization("DataReadbackOptimizationTAP1scan");
        theTAP1scanSerialization.streamByChipContainer(fDQMStreamer, theTAP1scanContainer);

        ContainerSerialization theTAP1Serialization("DataReadbackOptimizationTAP1");
        theTAP1Serialization.streamByChipContainer(fDQMStreamer, theTAP1Container);

        ContainerSerialization theTAP2scanSerialization("DataReadbackOptimizationTAP2scan");
        theTAP2scanSerialization.streamByChipContainer(fDQMStreamer, theTAP2scanContainer);

        ContainerSerialization theTAP2Serialization("DataReadbackOptimizationTAP2");
        theTAP2Serialization.streamByChipContainer(fDQMStreamer, theTAP2Container);
    }
}

void DataReadbackOptimization::Stop()
{
    LOG(INFO) << GREEN << "[DataReadbackOptimization::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void DataReadbackOptimization::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos          = nullptr;
    BERtest::histos = nullptr;

    LOG(INFO) << GREEN << "[DataReadbackOptimization::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    DataReadbackOptimization::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "DataReadbackOptimization", histos);
}

void DataReadbackOptimization::run()
{
    ContainerFactory::copyAndInitChip<std::vector<double>>(*fDetectorContainer, theTAP0scanContainer);
    CalibBase::fillVectorContainer<double>(theTAP0scanContainer, dacListTAP0.size(), 0);
    ContainerFactory::copyAndInitChip<std::vector<double>>(*fDetectorContainer, theTAP1scanContainer);
    CalibBase::fillVectorContainer<double>(theTAP1scanContainer, dacListTAP1.size(), 0);
    ContainerFactory::copyAndInitChip<std::vector<double>>(*fDetectorContainer, theTAP2scanContainer);
    CalibBase::fillVectorContainer<double>(theTAP2scanContainer, dacListTAP2.size(), 0);

    CalibBase::WriteBroadcastChipReg("CML_CONFIG_SER_EN_TAP", 0x0);
    DataReadbackOptimization::scanDac("DAC_CML_BIAS_0", dacListTAP0, &theTAP0scanContainer);
    CalibBase::ResetBoardsReadBkFIFO();
    DataReadbackOptimization::analyze("DAC_CML_BIAS_0", dacListTAP0, theTAP0scanContainer, theTAP0Container);

    CalibBase::WriteBroadcastChipReg("CML_CONFIG_SER_EN_TAP", 0x1);
    CalibBase::WriteBroadcastChipReg("CML_CONFIG_SER_INV_TAP", invTAP1);
    DataReadbackOptimization::scanDac("DAC_CML_BIAS_1", dacListTAP1, &theTAP1scanContainer);
    CalibBase::ResetBoardsReadBkFIFO();
    DataReadbackOptimization::analyze("DAC_CML_BIAS_1", dacListTAP1, theTAP1scanContainer, theTAP1Container);

    CalibBase::WriteBroadcastChipReg("CML_CONFIG_SER_EN_TAP", 0x3);
    CalibBase::WriteBroadcastChipReg("CML_CONFIG_SER_INV_TAP", bits::pack<1, 1>(invTAP2, invTAP1));
    DataReadbackOptimization::scanDac("DAC_CML_BIAS_2", dacListTAP2, &theTAP2scanContainer);
    CalibBase::ResetBoardsReadBkFIFO();
    DataReadbackOptimization::analyze("DAC_CML_BIAS_2", dacListTAP2, theTAP2scanContainer, theTAP2Container);

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void DataReadbackOptimization::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(BERtest::doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    DataReadbackOptimization::fillHisto();
    histos->process();

    if(BERtest::doDisplay == true) myApp->Run(true);
#endif
}

void DataReadbackOptimization::analyze(const std::string& regName, const std::vector<uint16_t>& dacListTAP, const DetectorDataContainer& theTAPscanContainer, DetectorDataContainer& theTAPContainer)
{
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theTAPContainer);

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    auto best = *std::max_element(theTAPscanContainer.getObject(cBoard->getId())
                                                      ->getObject(cOpticalGroup->getId())
                                                      ->getObject(cHybrid->getId())
                                                      ->getObject(cChip->getId())
                                                      ->getSummary<std::vector<double>>()
                                                      .begin(),
                                                  theTAPscanContainer.getObject(cBoard->getId())
                                                      ->getObject(cOpticalGroup->getId())
                                                      ->getObject(cHybrid->getId())
                                                      ->getObject(cChip->getId())
                                                      ->getSummary<std::vector<double>>()
                                                      .end());

                    int regVal = dacListTAP.at(0);
                    for(auto i = 1u; i < dacListTAP.size(); i++)
                    {
                        auto current = round(theTAPscanContainer.getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<std::vector<double>>()
                                                 .at(i) /
                                             RD53Shared::SUPERPRECISION) *
                                       RD53Shared::SUPERPRECISION;
                        if((current >= 0) && (current < best))
                        {
                            regVal = dacListTAP[i];
                            best   = current;
                        }
                    }

                    LOG(INFO) << BOLDMAGENTA << ">>> Best " << BOLDYELLOW << regName << BOLDMAGENTA << " for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/"
                              << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/" << +cChip->getId() << BOLDMAGENTA << "] is " << BOLDYELLOW << regVal << BOLDMAGENTA << " <<<" << RESET;

                    // ##################################################
                    // # Fill TAP container and download new DAC values #
                    // ##################################################
                    theTAPContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<double>() = regVal;
                    this->fReadoutChipInterface->WriteChipReg(static_cast<RD53*>(cChip), regName, regVal, false);
                }
}

void DataReadbackOptimization::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillScanTAP0(theTAP0scanContainer);
    histos->fillTAP0(theTAP0Container);

    histos->fillScanTAP1(theTAP1scanContainer);
    histos->fillTAP1(theTAP1Container);

    histos->fillScanTAP2(theTAP2scanContainer);
    histos->fillTAP2(theTAP2Container);
#endif
}

void DataReadbackOptimization::scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer)
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
        BERtest::run();

        // ######################
        // # Save BER test data #
        // ######################
        for(const auto cBoard: *theContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                        cChip->getSummary<std::vector<double>>().at(i) =
                            BERtest::theBERtestContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<double>();

        // ##############################################
        // # Send periodic data to monitor the progress #
        // ##############################################
        DataReadbackOptimization::sendData();
    }
}
